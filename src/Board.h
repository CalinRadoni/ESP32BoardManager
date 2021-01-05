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
     * - calls PowerPeripherals(true)
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

    /**
     * @brief Returns the severity level of failure for Initialize function
     *
     * If Initialize() fails, use this function to determine the level of severity of the failure
     *
     * - 0 means no failure
     * - 1 means PostInit failed
     * - 2 means BoardInit failed
     * - 3 reserved, not used
     * - 4 reserved, not used
     * - 5 means that one of the actions from start to CriticalInit, inclusive, failed
     *
     * A failure level greater then 2 should be equivalent to a critical system failure.
     * The severity of Levels 1 and 2 depends strictly of the functionality you've added in PostInit function.
     *
     * @return uint8_t
     */
    uint8_t InitFailSeverity(void);

    virtual esp_err_t EarlyInit(void) = 0;
    virtual esp_err_t CriticalInit(void) = 0;
    virtual esp_err_t BoardInit(void) = 0;
    virtual esp_err_t PostInit(void) = 0;

    /**
     * @brief Turn on or off the power for peripherals
     *
     * If the board have this functionality override this function in your dedicated class.
     */
    virtual bool PowerPeripherals(bool);

    /**
     * @brief Enters deep sleep for specified number of minutes
     *
     * This function does not return !
     *
     * It only calls `esp_deep_sleep`.
     * It does not shut down WiFi, BT, or any other higher level protocol connections gracefully.
     */
    void EnterDeepSleep(uint32_t minutes);

    /**
     * @brief Restarts the board after specified number of seconds
     *
     * This function does not return !
     *
     * It waits using `vTaskDelay` so it is not really accurate.
     * After waiting it only calls `esp_restart`.
     * It does not shut down WiFi, BT, or any other higher level protocol connections gracefully.
     */
    void Restart(uint32_t seconds);

    /**
     * @brief Returns the base MAC address which is factory-programmed by Espressif in BLK0 of EFUSE.
     *
     * The MAC address is readed using the esp_efuse_mac_get_default function.
     *
     * Returns a pointer to the private variable MAC.
     */
    uint8_t* GetMAC(void);

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
     * After each failed retry the function waits a random time between 100ms and 600ms.
     *
     * @param apIdx the index of the AP configuration to use
     *
     * @returns ESP_ERR_INVALID_ARG if configuration is nullptr
     */
    esp_err_t StartStation(uint8_t maxRetries);

    /**
     * @brief Returns true if connected to an AP
     */
    bool IsConnectedToAP(void);

    esp_err_t RestartStationMode(uint8_t maxRetries);

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

    uint8_t initFailSeverity;

    /**
     * @brief Placeholder for base MAC address
     */
    uint8_t MAC[6];

    /**
     * @brief Connect to one of the saved APs
     *
     * Tries to connect and wait for connection to complete or timeout.
     *
     * @param apIdx the index of the AP configuration to use
     *
     * @return ESP_ERR_INVALID_ARG if configuration is nullptr
     * @return ESP_ERR_INVALID_ARG if AP index is >= WiFiConfigCnt
     */
    esp_err_t ConnectToAP(uint8_t apIdx);

    esp_err_t InitializeMDNS(void);
    void CleanupMDNS(void);
};

#endif
