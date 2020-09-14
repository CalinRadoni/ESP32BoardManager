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
#include "Configuration.h"
#include "WiFiManager.h"
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
    httpServer = nullptr;
}

Board::~Board(void)
{
    //
}

esp_err_t Board::Initialize(PaxHttpServer *theHttpServer)
{
    if (initialized)
        return ESP_ERR_INVALID_STATE;

    if (theHttpServer == nullptr)
        return ESP_ERR_INVALID_ARG;

    httpServer = theHttpServer;

    esp_err_t err = EarlyInit();
    if (err != ESP_OK) {
        GoodBye();
        return err;
    }

    err = configuration.InitializeNVS();
    if (err != ESP_OK) {
        GoodBye();
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        GoodBye();
        return err;
    }

    if (!events.Create()) {
        GoodBye();
        return err;
    }
    theWiFiManager.events = &events;

    tcpip_adapter_init();

    err = CriticalInit();
    if (err != ESP_OK) {
        GoodBye();
        return err;
    }

    err = configuration.ReadFromNVS();
    if (err != ESP_OK) {
        GoodBye();
        return err;
    }

    err = BoardInit();
    if (err != ESP_OK)
        return err;

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

esp_err_t Board::StartConfigurationAP(void)
{
    const char* apName = "DevX";
    WiFiConfig apCfg, staCfg;
    esp_err_t err;
    EventBits_t bits;

    err = theWiFiManager.Initialize();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x theWiFiManager.Initialize", err);
        return err;
    }

    // perform an AP scan
    err = theWiFiManager.Start(WiFiManagerMode::scan, nullptr, nullptr);
    if (err == ESP_OK) {
        bits = events.WaitForAllBits(xBitScanDone, msToWaitForScan);
        if ((bits & xBitScanDone) == 0) {
            ESP_LOGE(TAG, "Scan timeout");
            return ESP_ERR_TIMEOUT;
        }
    }
    theWiFiManager.Stop(true);

    // connect as apsta
    GetMAC();
    apCfg.SetFromNameAndMAC(apName, MAC);
    staCfg.SetFromNameAndMAC(apName, MAC);
    err = theWiFiManager.Start(WiFiManagerMode::apsta, &staCfg, &apCfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x Start APSTA", err);
        return err;
    }

    err = httpServer->StartServer();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%d StartServer", err);
        return err;
    }

/*
    bool done = false;

    while (!done) {
        // wait for credentials
        bits = events.WaitForAllBits(xBitCredentialsReceived, portMAX_DELAY);

        if (theHTTPConfigServer.checkThem) {
            theHTTPConfigServer.checkThem = false;

            theHTTPConfigServer.StopServer();
            theWiFiManager.Stop(true);

            staCfg.SetFromStrings(theHTTPConfigServer.SSID, theHTTPConfigServer.PASS);
            err = theWiFiManager.Start(WiFiManagerMode::apsta, &staCfg, &apCfg);
            if (err == ESP_OK) {
                bits = events.WaitForAnyBit(xBitStaConnected | xBitStaDisconnected, msToWaitToConnect);
                if ((bits & xBitStaDisconnected) != 0) {
                    theHTTPConfigServer.credentialsState = CredentialsState::checkFail;
                    ESP_LOGW(TAG, "Credentials test fail, reason %d", theWiFiManager.GetDisconnectReason());
                }
                if ((bits & xBitStaConnected) != 0) {
                    theHTTPConfigServer.credentialsState = CredentialsState::checkSuccess;
                    ESP_LOGI(TAG, "Credentials OK");
                }
                if ((bits & (xBitStaConnected | xBitStaDisconnected)) == 0) {
                    theHTTPConfigServer.credentialsState = CredentialsState::checkTimeout;
                    ESP_LOGE(TAG, "Credentials test timeout");
                }
            }
            else {
                theHTTPConfigServer.credentialsState = CredentialsState::processingError;
                ESP_LOGE(TAG, "%d Credentials test error", err);

                staCfg.SetFromNameAndMAC(apName, MAC);
                theWiFiManager.Stop(true);
                theWiFiManager.Start(WiFiManagerMode::apsta, &staCfg, &apCfg);
            }

            theHTTPConfigServer.StartServer();
        }

        if (theHTTPConfigServer.saveThem) {
            theHTTPConfigServer.saveThem = false;
            configuration.wcfg[0].SetFromStrings(theHTTPConfigServer.SSID, theHTTPConfigServer.PASS);

            err = configuration.WriteConfiguration(false);

            if (err == ESP_OK) {
                done = true;
                theHTTPConfigServer.credentialsSaveState = CredentialsSaveState::saved;
            }
            else {
                theHTTPConfigServer.credentialsSaveState = CredentialsSaveState::saveError;
            }
        }
    }

    if (theHTTPConfigServer.credentialsSaveState == CredentialsSaveState::saved) {
        // wait for user to see the status
        uint8_t i = 20;
        while (i > 0) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            i--;
        }

        theHTTPConfigServer.credentialsSaveState = CredentialsSaveState::saveRestart;

        // wait for user to see the status
        i = 50;
        while (i > 0) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            i--;
        }
    }
*/

    httpServer->StopServer();
    theWiFiManager.Stop(true);
    theWiFiManager.Clean();

    Restart();

    return ESP_OK;
}

esp_err_t Board::StartAP(void)
{
    const char* apName = "DevX";
    WiFiConfig apCfg;
    esp_err_t err;

    err = theWiFiManager.Initialize();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x theWiFiManager.Initialize", err);
        return err;
    }

    // connect as AP
    GetMAC();
    apCfg.SetFromNameAndMAC(apName, MAC);
    err = theWiFiManager.Start(WiFiManagerMode::ap, nullptr, &apCfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x Start AP", err);
        return err;
    }

    return ESP_OK;
}

void Board::StopAP(void)
{
    theWiFiManager.Stop(true);
    theWiFiManager.Clean();
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
    return theWiFiManager.Initialize();
}

esp_err_t Board::CleanWiFi(void)
{
    return theWiFiManager.Clean();
}

esp_err_t Board::Connect(void)
{
    WiFiConfig staCfg;

    theWiFiManager.Stop(true);

    staCfg.SetFromStrings((const char*)configuration.apCfg[0].SSID, (const char*)configuration.apCfg[0].Pass);
    esp_err_t err = theWiFiManager.Start(WiFiManagerMode::station, &staCfg, nullptr);
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

void Board::Disconnect(void)
{
    theWiFiManager.Stop(true);
}
