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
#include "esp_err.h"
#include "esp_log.h"

#include <string>
#include <cstring>
#include <vector>

#include "esp_system.h"
#include "esp_event.h"
#include "esp_sleep.h"

#include "Board.h"
#include "pax_http_server.h"

#include "esp_netif.h"
#include "mdns.h"

// -----------------------------------------------------------------------------

static const char* TAG = "Board";

const uint64_t usInOneHour       = 3600000000ULL;

const uint32_t msToWaitForScan   = 120000UL;
const uint32_t msToWaitToConnect =  60000UL;

const char* defaultDeviceName = "pax-device";
const char* defaultDevicePass = "paxxword";

// -----------------------------------------------------------------------------

Board::Board(void)
{
    initialized = false;
    netInitialized = false;
    memset(MAC, 0, 6);
    configuration = nullptr;
}

Board::~Board(void)
{
    if (netInitialized) {
        netInitialized = false;
        esp_netif_deinit();
    }
}

esp_err_t Board::Initialize(void)
{
    if (initialized)
        return ESP_ERR_INVALID_STATE;

    cpu.ReadChipInfo();

    esp_err_t err = EarlyInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x EarlyInit", err);
        GoodBye();
        return err;
    }

    if (configuration == nullptr) {
        ESP_LOGE(TAG, "configuration is null");
        GoodBye();
        return ESP_FAIL;
    }
    err = configuration->InitializeNVS();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x configuration->InitializeNVS", err);
        GoodBye();
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x esp_event_loop_create_default", err);
        GoodBye();
        return err;
    }

    if (!events.Create()) {
        ESP_LOGE(TAG, "0x%x events.Create", err);
        GoodBye();
        return err;
    }
    theWiFiManager.events = &events;

    if (netInitialized) {
        netInitialized = false;
        esp_netif_deinit();
    }
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x esp_netif_init", err);
        GoodBye();
        return err;
    }
    netInitialized = true;

    err = CriticalInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x CriticalInit", err);
        GoodBye();
        return err;
    }

    err = configuration->ReadFromNVS();
    if (err != ESP_OK) {
        // the stored configuration is not valid ...
        ESP_LOGE(TAG, "0x%x configuration->ReadFromNVS", err);
        // ... build an empty one
        configuration->InitData();
        ESP_LOGW(TAG, "Configuration initialized to default values");
    }

    err = BoardInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x BoardInit", err);
        return err;
    }

    // set WiFi power mode to WIFI_PS_NONE, otherwise it will not respond to ARP requests !
    wifi_ps_type_t psType = WIFI_PS_MIN_MODEM;
    err = esp_wifi_get_ps(&psType);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "0x%x esp_wifi_get_ps", err);
        psType = WIFI_PS_MIN_MODEM;
    }
    if (psType != WIFI_PS_NONE) {
        err = esp_wifi_set_ps(WIFI_PS_NONE);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Current WiFi power save set to WIFI_PS_NONE");
        }
        else {
            ESP_LOGE(TAG, "0x%x esp_wifi_set_ps", err);
        }
    }

    SetBoardInfo();

    err = PostInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x PostInit", err);
        return err;
    }

    initialized = true;
    return ESP_OK;
}

void Board::SetBoardInfo(void)
{
    const esp_app_desc_t *appDescription;
    appDescription = esp_ota_get_app_description();
    if (appDescription->magic_word != 0xabcd5432) {
        ESP_LOGE(TAG, "esp_ota_get_app_description");
    }
    else {
        boardInfo.appName = appDescription->project_name;
        boardInfo.appVersion = appDescription->version;
        boardInfo.compileTime = appDescription->date;
        boardInfo.compileTime += ' ';
        boardInfo.compileTime += appDescription->time;
        boardInfo.idfVersion = appDescription->idf_ver;
        ESP_LOGI(TAG, "%s %s compiled with ESP_IDF %s on %s",
            boardInfo.appName.c_str(), boardInfo.appVersion.c_str(),
            boardInfo.idfVersion.c_str(), boardInfo.compileTime.c_str());

        const char hex[17] = "0123456789abcdef";
        const uint8_t *sha256 = appDescription->app_elf_sha256;
        boardInfo.elfSHA256.clear();
        boardInfo.elfSHA256.reserve(64);
        for (uint8_t i = 32; i > 0; --i) {
            boardInfo.elfSHA256 += hex[(*sha256 >> 4) & 0x0F];
            boardInfo.elfSHA256 += hex[*sha256 & 0x0F];
            ++sha256;
        }
        ESP_LOGI(TAG, "SHA256 of elf file: %s", boardInfo.elfSHA256.c_str());
    }

    boardInfo.link = "https://github.com/CalinRadoni/ESP32BoardManager";
    boardInfo.tagline = "";

    std::string formatString;
    int size;

    // create a string for processor type
    boardInfo.hwInfo = cpu.chipModel;
    if (cpu.numberOfCores == 1) {
        boardInfo.hwInfo += " single-core";
    }
    else {
        if (cpu.numberOfCores == 2) {
            boardInfo.hwInfo += " dual-core";
        }
        else {
            boardInfo.hwInfo += " with ";
            formatString = " with %d cores";
            size = std::snprintf(nullptr, 0, formatString.c_str(), cpu.numberOfCores);
            if (size > 0){
                std::vector<char> bc(size + 1);
                std::snprintf(bc.data(), bc.size(), formatString.c_str(), cpu.numberOfCores);
                boardInfo.hwInfo += bc.data();
            }
            else { ESP_LOGE(TAG, "snprintf hwInfo"); }
        }
    }

    // add memory info
    formatString = ", Flash chip ID 0x%x, size %dMB";
    size = std::snprintf(nullptr, 0, formatString.c_str(), cpu.espFlashID, cpu.espFlashSize / (1024 * 1024));
    if (size > 0) {
        std::vector<char> buffer(size + 1);
        std::snprintf(buffer.data(), buffer.size(), formatString.c_str(), cpu.espFlashID, cpu.espFlashSize / (1024 * 1024));
        boardInfo.hwInfo += buffer.data();
    }
    else { ESP_LOGE(TAG, "snprintf hwInfo"); }
}

void Board::DoNothingForever(void)
{
    while (true) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void Board::GoodBye(void)
{
    uint64_t sleepTimeUS = usInOneHour;
    esp_deep_sleep(sleepTimeUS);
}

uint8_t* Board::GetMAC(void)
{
    bool isZero = true;

    for (uint8_t i = 0; isZero && (i < 6); i++) {
        if (MAC[i] != 0)
            isZero = false;
    }
    if (isZero){
        esp_efuse_mac_get_default(MAC);
    }

    return MAC;
}

void Board::Restart(void)
{
    esp_restart();
}

esp_err_t Board::InitializeWiFi(void)
{
    esp_err_t res = theWiFiManager.Initialize();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "0x%x theWiFiManager.Initialize", res);
    }
    return res;
}

esp_err_t Board::CleanWiFi(void)
{
    return theWiFiManager.Clean();
}

esp_err_t Board::StartAP(void)
{
    WiFiConfig ap;

    ap.Initialize();
    if (configuration != nullptr) {
        if (!configuration->name.empty()) {
            ap.ssid = configuration->name;
        }
        if (configuration->pass.length() > 7) {
            ap.pass = configuration->pass;
        }
    }
    if (ap.ssid.empty()) {
        BuildDefaultDeviceName(ap.ssid);
    }
    if (ap.pass.empty()) {
        ap.pass = defaultDevicePass;
    }

    esp_err_t err = theWiFiManager.Start(WiFiManagerMode::ap, nullptr, &ap);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x Start AP", err);
        return err;
    }

    return ESP_OK;
}

esp_err_t Board::StartStation()
{
    uint8_t idx = 0;
    bool done = false;
    esp_err_t res = ESP_FAIL;

    if (configuration == nullptr) return ESP_ERR_INVALID_ARG;

    while (!done && (idx < WiFiConfigCnt)) {
        if (configuration->ap[idx].CheckData()) {
            res = ConnectToAP(idx);
            if (res == ESP_OK) {
                // connected to an AP
                ESP_LOGI(TAG, "Connected to \"%s\"", configuration->ap[idx].ssid.c_str());
                done = true;
            }
            else {
                ESP_LOGE(TAG, "Failed to connect to \"%s\"", configuration->ap[idx].ssid.c_str());
                ++idx;
            }
        }
        else {
            ESP_LOGW(TAG, "AP configuration %d skipped", (idx + 1));
            ++idx;
        }
    }

    return done ? ESP_OK : res;
}

esp_err_t Board::ConnectToAP(uint8_t apIdx)
{
    theWiFiManager.Stop(true);

    if (configuration == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (apIdx >= WiFiConfigCnt) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = theWiFiManager.Start(WiFiManagerMode::station, &(configuration->ap[apIdx]), nullptr);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Connect error");
    }
    else {
        EventBits_t bits = events.WaitForAnyBit(xBitStaConnected | xBitStaDisconnected, msToWaitToConnect);
        if ((bits & xBitStaDisconnected) != 0) {
            ESP_LOGW(TAG, "Disconnected, reason %d", theWiFiManager.GetDisconnectReason());
            err = ESP_FAIL;
        }
        if ((bits & xBitStaConnected) != 0) {
            ESP_LOGI(TAG, "Connected");
            err = ESP_OK;
        }
        if ((bits & (xBitStaConnected | xBitStaDisconnected)) == 0) {
            ESP_LOGE(TAG, "Connect timeout");
            err = ESP_ERR_WIFI_TIMEOUT;
        }
    }

    return err;
}

void Board::StopWiFiMode(void)
{
    theWiFiManager.Stop(true);
}

esp_err_t Board::CheckApplicationImage(void)
{
    return simpleOTA.CheckApplicationImage();
}

void Board::BuildDefaultDeviceName(std::string& str)
{
    str.clear();

    GetMAC();

    std::string formatString = "%s-%02X%02X%02X";
    int size = std::snprintf(nullptr, 0, formatString.c_str(), defaultDeviceName, MAC[3], MAC[4], MAC[5]);
    if (size <= 0) {
        str = defaultDeviceName;
        return;
    }

    std::vector<char> buffer(size + 1);
    std::snprintf(buffer.data(), buffer.size(), formatString.c_str(), defaultDeviceName, MAC[3], MAC[4], MAC[5]);
    str = buffer.data();
}

esp_err_t Board::InitializeMDNS(void)
{
    esp_err_t res = mdns_init();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "0x%x mdns_init", res);
        return res;
    }

    std::string name;
    if (configuration != nullptr) {
        if (!configuration->name.empty()) {
            name = configuration->name;
        }
    }
    if (name.empty()) {
        BuildDefaultDeviceName(name);
    }

    ESP_LOGI(TAG, "Setting the mDNS hostname to %s.local", name.c_str());

    res = mdns_hostname_set(name.c_str());
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "0x%x mdns_hostname_set", res);
        return res;
    }

    res = mdns_instance_name_set(name.c_str());
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "0x%x mdns_instance_name_set", res);
        return res;
    }

    return ESP_OK;
}

void Board::CleanupMDNS(void)
{
    mdns_service_remove_all();
    mdns_free();
}
