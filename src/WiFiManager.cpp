/**
This file is part of ESP32BoardManager esp-idf component
(https://github.com/CalinRadoni/ESP32BoardManager)
Copyright (C) 2019 by Calin Radoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"

#include <cstring>
#include <new>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "WiFiManager.h"

// -----------------------------------------------------------------------------

static const char* TAG = "WiFiManager";

const uint32_t msToWaitAfterStop = 30000;

// -----------------------------------------------------------------------------

static void default_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    WiFiManager* theWiFiManager = static_cast<WiFiManager*>(arg);
    if (theWiFiManager != nullptr)
        theWiFiManager->EventHandler(event_base, event_id, event_data);
}

// -----------------------------------------------------------------------------

WiFiManager::WiFiManager(void)
{
    events = nullptr;

    workStatus = WiFiManagerStatus::idle;
    workMode   = WiFiManagerMode::none;

    defaultAP = nullptr;
    defaultSTA = nullptr;

    foundAPInfo = nullptr;
    foundAPcnt = 0;
}

WiFiManager::~WiFiManager(void)
{
    Stop(true);
    Clean();

    if (foundAPInfo != nullptr) {
        delete[] foundAPInfo;
        foundAPInfo = nullptr;
    }
    foundAPcnt = 0;
}

esp_err_t WiFiManager::Initialize(void)
{
    workStatus = WiFiManagerStatus::idle;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    /**
     * defaultAP and defaultSTA are BOTH created here because
     * those have to be created before calling esp_wifi_init
     * so on-demand creation is not possible for now
     */
    CleanDefaultAPSTA();
    defaultAP = esp_netif_create_default_wifi_ap();
    defaultSTA = esp_netif_create_default_wifi_sta();

    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        return err;
    }

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        return err;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &default_event_handler, this);
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        return err;
    }

    err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &default_event_handler, this);
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        return err;
    }

    esp_wifi_set_mode(WIFI_MODE_NULL);

    return ESP_OK;
}

void WiFiManager::CleanDefaultAPSTA(void)
{
    if (defaultSTA != nullptr) {
        esp_netif_destroy(defaultSTA);
        defaultSTA = nullptr;
    }
    if (defaultAP != nullptr) {
        esp_netif_destroy(defaultAP);
        defaultAP = nullptr;
    }
}

esp_err_t WiFiManager::Clean(void)
{
    esp_wifi_set_mode(WIFI_MODE_NULL);

    esp_err_t err = esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &default_event_handler);
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        // do not return. continue ...
    }

    err = esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &default_event_handler);
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        // do not return. continue ...
    }

    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        workStatus = WiFiManagerStatus::error;
        return err;
    }

    CleanDefaultAPSTA();

    workStatus = WiFiManagerStatus::idle;
    return ESP_OK;
}

esp_err_t WiFiManager::ConfigStation(WiFiConfig *cfg)
{
    if (cfg == nullptr) return ESP_ERR_INVALID_ARG;
    if (!cfg->CheckData()) return ESP_ERR_INVALID_ARG;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) {
        wifi_config_t wifi_config;
        cfg->SetStationConfig(&wifi_config);
        err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
    return err;
}

esp_err_t WiFiManager::ConfigAP(WiFiConfig *cfg)
{
    if (cfg == nullptr) return ESP_ERR_INVALID_ARG;
    if (!cfg->CheckData()) return ESP_ERR_INVALID_ARG;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err == ESP_OK) {
        wifi_config_t wifi_config;
        cfg->SetAPConfig(&wifi_config);
        wifi_config.ap.ssid_hidden     = 0;
        wifi_config.ap.authmode        = WIFI_AUTH_WPA2_PSK;
        wifi_config.ap.channel         = 0;
        wifi_config.ap.max_connection  = 4;
        wifi_config.ap.beacon_interval = 100;
        err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    }
    return err;
}

esp_err_t WiFiManager::ConfigAPStation(WiFiConfig* staCfg, WiFiConfig* apCfg)
{
    if (staCfg == nullptr) return ESP_ERR_INVALID_ARG;
    if (!staCfg->CheckData()) return ESP_ERR_INVALID_ARG;
    if (apCfg == nullptr) return ESP_ERR_INVALID_ARG;
    if (!apCfg->CheckData()) return ESP_ERR_INVALID_ARG;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) return err;

    wifi_config_t wifi_config;

    staCfg->SetStationConfig(&wifi_config);
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) return err;

    apCfg->SetAPConfig(&wifi_config);
    wifi_config.ap.ssid_hidden     = 0;
    wifi_config.ap.authmode        = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.channel         = 0;
    wifi_config.ap.max_connection  = 4;
    wifi_config.ap.beacon_interval = 100;
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) return err;

    return err;
}

esp_err_t WiFiManager::ConfigStationScan(void)
{
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) {
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));
        err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
    return err;
}

esp_err_t WiFiManager::StartActiveScan(void)
{
    wifi_scan_config_t scan_config;
    scan_config.ssid        = nullptr;
    scan_config.bssid       = nullptr;
    scan_config.channel     = 0;
    scan_config.show_hidden = true;
    scan_config.scan_type   = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min = 0;
    scan_config.scan_time.active.max = 500;

    return esp_wifi_scan_start(&scan_config, false);
}

esp_err_t WiFiManager::Start(WiFiManagerMode initMode, WiFiConfig* staCfg, WiFiConfig* apCfg)
{
    esp_err_t err;

    switch (initMode) {
        case WiFiManagerMode::station:
            workStatus = WiFiManagerStatus::staConnecting;
            err = ConfigStation(staCfg);
            break;
        case WiFiManagerMode::ap:
            workStatus = WiFiManagerStatus::apCreated;
            err = ConfigAP(apCfg);
            break;
        case WiFiManagerMode::apsta:
            workStatus = WiFiManagerStatus::apCreated;
            err = ConfigAPStation(staCfg, apCfg);
            break;
        case WiFiManagerMode::scan:
            workStatus = WiFiManagerStatus::scanInitiated;
            err = ConfigStationScan();
            break;

        default:
            err = ESP_ERR_INVALID_ARG;
            break;
    }
    if (err != ESP_OK) {
        esp_wifi_set_mode(WIFI_MODE_NULL);
        workStatus = WiFiManagerStatus::error;
        workMode   = WiFiManagerMode::none;
        return err;
    }
    else {
        workMode = initMode;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        esp_wifi_set_mode(WIFI_MODE_NULL);
        workStatus = WiFiManagerStatus::error;
        workMode   = WiFiManagerMode::none;
        return err;
    }

    if (initMode == WiFiManagerMode::scan) {
        err = StartActiveScan();
        if (err != ESP_OK) {
            workStatus = WiFiManagerStatus::error;
            workMode   = WiFiManagerMode::none;
        }
    }

    return err;
}

esp_err_t WiFiManager::Stop(bool wait)
{
    esp_err_t err = ESP_OK;
    EventBits_t waitBits = 0;
    wifi_mode_t mode;

    if (wait) {
        err =  esp_wifi_get_mode(&mode);
        if (err == ESP_OK) {
            switch (mode) {
                case WIFI_MODE_STA:
                    if (GetAPInfo() != nullptr)
                        waitBits |= xBitStaDisconnected;
                    break;
                case WIFI_MODE_AP:
                    waitBits |= xBitAPStopped;
                    break;
                case WIFI_MODE_APSTA:
                    waitBits |= xBitAPStopped;
                    if (GetAPInfo() != nullptr)
                        waitBits |= xBitStaDisconnected;
                    break;
                default:
                    break;
            }
        }

        err = esp_wifi_stop();
        if (waitBits != 0) {
            if (err == ESP_OK && (events != nullptr)) {
                EventBits_t bits = events->WaitForAllBits(waitBits, msToWaitAfterStop);
                if ((bits & waitBits) == 0) {
                    err = ESP_ERR_TIMEOUT;
                }
            }
        }
    }
    else {
        err = esp_wifi_stop();
    }

    workStatus = (err == ESP_OK) ? WiFiManagerStatus::idle : WiFiManagerStatus::error;
    workMode   = WiFiManagerMode::none;
    esp_wifi_set_mode(WIFI_MODE_NULL);
    return err;
}

uint8_t WiFiManager::GetDisconnectReason(void)
{
    return disconnectReason;
}

void WiFiManager::EventHandler(esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    esp_err_t err = ESP_OK;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                err = esp_wifi_connect();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "0x%04x esp_wifi_connect", err);
                }
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                {
                wifi_event_sta_disconnected_t* data = (wifi_event_sta_disconnected_t*)event_data;
                disconnectReason = data->reason;
                workStatus = WiFiManagerStatus::staDisconnected;
                // Do not reconnect automatically !
                // If connection is closed, after reconnection the sockets should be reinitialized !
                ESP_LOGW(TAG, "Disconnected from %s, reason %d", data->ssid, disconnectReason);
                if (events != nullptr)
                    events->SetBits(xBitStaDisconnected);
                }
                break;

            case WIFI_EVENT_AP_START:
                if (events != nullptr)
                    events->SetBits(xBitAPStarted);
                break;
            case WIFI_EVENT_AP_STOP:
                if (events != nullptr)
                    events->SetBits(xBitAPStopped);
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                {
                wifi_event_ap_staconnected_t* data = (wifi_event_ap_staconnected_t*)event_data;
                ESP_LOGI(TAG, "Station %02X:%02X:%02X:%02X:%02X:%02X join, AID=%d",
                            data->mac[0], data->mac[1], data->mac[2], data->mac[3], data->mac[4], data->mac[5], data->aid);
                }
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                {
                wifi_event_ap_stadisconnected_t* data = (wifi_event_ap_stadisconnected_t*)event_data;
                ESP_LOGI(TAG, "Station %02X:%02X:%02X:%02X:%02X:%02X leave, AID=%d",
                            data->mac[0], data->mac[1], data->mac[2], data->mac[3], data->mac[4], data->mac[5], data->aid);
                }
                break;

            case WIFI_EVENT_SCAN_DONE:
                ExtractAPScanResults();
                ESP_LOGI(TAG, "Scanning done");
                workStatus = WiFiManagerStatus::scanDone;
                if (events != nullptr)
                    events->SetBits(xBitScanDone);
                break;

            default:
                ESP_LOGD(TAG, "received WiFi event %x", event_id);
                break;
        }
        return;
    }

    if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                {
                ip_event_got_ip_t* data = (ip_event_got_ip_t*)event_data;
                ESP_LOGI(TAG, "Got IP:" IPSTR "\n", IP2STR(&data->ip_info.ip));
                workStatus = WiFiManagerStatus::staConnected;
                if (events != nullptr)
                    events->SetBits(xBitStaConnected);
                }
                break;

            case IP_EVENT_STA_LOST_IP:
                // before using this event read the ESP-IDF Programming Guide
                break;

            default:
                ESP_LOGD(TAG, "received IP event %x", event_id);
                break;
        }
        return;
    }
}

wifi_ap_record_t* WiFiManager::GetAPInfo(void)
{
    if (esp_wifi_sta_get_ap_info(&apRecord) == ESP_OK) {
        return &apRecord;
    }

    return nullptr;
}

WiFiManagerStatus WiFiManager::Status(void)
{
    return workStatus;
}

void WiFiManager::ExtractAPScanResults(void)
{
    if (foundAPInfo != nullptr) {
        delete[] foundAPInfo;
        foundAPInfo = nullptr;
    }
    foundAPcnt = 0;

    esp_err_t err = esp_wifi_scan_get_ap_num(&foundAPcnt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%04x esp_wifi_scan_get_ap_num", err);
        return;
    }
    if (foundAPcnt == 0)
        return;

    foundAPInfo = new (std::nothrow) wifi_ap_record_t[foundAPcnt];
    if (foundAPInfo == nullptr) {
        ESP_LOGE(TAG, "foundAPInfo[%d] alloc failed", foundAPcnt);
        return;
    }

    err = esp_wifi_scan_get_ap_records(&foundAPcnt, foundAPInfo);
    if (foundAPcnt == 0) {
        delete[] foundAPInfo;
        foundAPInfo = nullptr;
        ESP_LOGE(TAG, "0x%04x esp_wifi_scan_get_ap_records", err);
        return;
    }

    for (uint16_t i = 0; i < foundAPcnt; i++){
        ESP_LOGI(TAG, "%32s %2d %4d %d", foundAPInfo[i].ssid, foundAPInfo[i].primary, foundAPInfo[i].rssi, foundAPInfo[i].authmode);
    }
}

uint16_t WiFiManager::GetScanRecordCount(void)
{
    return foundAPcnt;
}

wifi_ap_record_t* WiFiManager::GetScanRecord(uint16_t index)
{
    if (foundAPInfo == nullptr)
        return nullptr;
    if (index >= foundAPcnt)
        return nullptr;
    return &foundAPInfo[index];
}

void WiFiManager::FreeScanRecords(void)
{
    if (foundAPInfo != nullptr) {
        delete[] foundAPInfo;
        foundAPInfo = nullptr;
    }
    foundAPcnt = 0;
}
