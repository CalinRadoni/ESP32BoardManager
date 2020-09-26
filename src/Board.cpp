/**
This file is part of BoardManager esp-idf component from
pax-devices (https://github.com/CalinRadoni/pax-devices)
Copyright (C) 2019+ by Calin Radoni

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

#include <string.h>

#include "esp_system.h"
#include "esp_event.h"
#include "esp_sleep.h"

#include "Board.h"
#include "pax_http_server.h"

// -----------------------------------------------------------------------------

static const char* TAG = "Board";

const uint64_t usInOneHour       = 3600000000ULL;

const uint32_t msToWaitForScan   = 120000UL;
const uint32_t msToWaitToConnect =  60000UL;

// -----------------------------------------------------------------------------

Board::Board(void)
{
    initialized = false;
    memset(MAC, 0, 6);
    configuration = nullptr;
}

Board::~Board(void)
{
    //
}

esp_err_t Board::Initialize(void)
{
    if (initialized)
        return ESP_ERR_INVALID_STATE;

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

    tcpip_adapter_init();

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

    err = PostInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x PostInit", err);
        return err;
    }

    initialized = true;
    return ESP_OK;
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
    const char* apName = "DevX";
    WiFiConfig ap;

    // connect as AP
    GetMAC();
    ap.SetFromNameAndMAC(apName, MAC);
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
