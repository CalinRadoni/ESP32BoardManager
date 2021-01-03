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

#ifndef Board_H
#define Board_H

#include "freertos/FreeRTOS.h"
#include "BoardEvents.h"
#include "pax_http_server.h"
#include "WiFiManager.h"
#include "BoardInfo.h"

#include "esp32_hal_cpu.h"
#include "esp32_hal_gpio.h"
#include "esp32_hal_adc.h"
#include "esp32_hal_i2c.h"
#include "esp32_hal_spi.h"

class Board
{
public:
    Board(void);
    virtual ~Board(void);

    /**
     * @brief Initialize entire board
     *
     * This functions does, in order:
     * - calls EarlyInit()
     * - initialize NVS and read configuration from NVS
     * - initialize the underlying TCP/IP stack
     * - calls CriticalInit()
     * - calls BoardInit()
     * - calls PostInit()
     *
     * The execution is transfered to the GoodBye function when any step until
     * BoardInit fails with one exception: if configuration cannot be read from NVS
     * or is not valid a default one is created.
     *
     * If BoardInit fails the Initialize will return.
     *
     * For more and detailed information see the workflow file in docs.
     */
    esp_err_t Initialize(void);

    virtual esp_err_t EarlyInit(void) = 0;
    virtual esp_err_t CriticalInit(void) = 0;
    virtual esp_err_t BoardInit(void) = 0;
    virtual esp_err_t PostInit(void) = 0;

    /**
     * @brief Executes vTaskDelay every 100ms
     */
    void DoNothingForever(void);

    /**
     * @brief Enters deep sleep for one hour
     */
    void GoodBye(void);

    /**
     * @brief Returns the base MAC address which is factory-programmed by Espressif in BLK0 of EFUSE.
     *
     * The MAC address is readed using the esp_efuse_mac_get_default function.
     *
     * Returns a pointer to the private variable MAC.
     */
    uint8_t* GetMAC(void);

    /**
     * @brief Restart the board
     *
     * Uses the esp_restart function so this function does not return.
     */
    void Restart(void);

    /**
     * @brief Initialize the WiFi
     *
     * This function can be called from the overridden BoardInit function
     */
    esp_err_t InitializeWiFi(void);

    /**
     * @brief Call this function if WiFi is not needed any more
     */
    esp_err_t CleanWiFi(void);

    /**
     * @brief Start the board in AP mode
     *
     * This function can be called from the overridden PostInit function.
     */
    esp_err_t StartAP(void);

    /**
     * @brief Connect to one of the saved APs
     *
     * Tries to connect and wait for connection to complete or timeout.
     * This function can be called from the overridden PostInit function.
     *
     * @returns ESP_ERR_INVALID_ARG if configuration is nullptr
     */
    esp_err_t StartStation(void);

    /**
     * @brief Stop the AP or station mode
     */
    void StopWiFiMode(void);

    /**
     * @brief returns ESP32SimpleOTA::CheckApplicationImage()
     */
    esp_err_t CheckApplicationImage(void);

    void BuildDefaultDeviceName(std::string& str);

protected:
    EventGroupHandler events;

    WiFiManager theWiFiManager;
    ESP32SimpleOTA simpleOTA;
    Configuration *configuration;
    esp32hal::CPU cpu;

    BoardInfo boardInfo;
    void SetBoardInfo(void);

    bool initialized;
    bool netInitialized;

    /**
     * @brief Placeholder for base MAC address
     */
    uint8_t MAC[6];

    /**
     * @brief Connect to one of the saved APs
     *
     * Tries to connect and wait for connection to complete or timeout.
     *
     * @returns ESP_ERR_INVALID_ARG if configuration is nullptr
     * @returns ESP_ERR_INVALID_ARG if AP index is >= WiFiConfigCnt
     */
    esp_err_t ConnectToAP(uint8_t);

    esp_err_t InitializeMDNS(void);
    void CleanupMDNS(void);
};

#endif
