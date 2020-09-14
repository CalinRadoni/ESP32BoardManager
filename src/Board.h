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

#ifndef Board_H
#define Board_H

#include "freertos/FreeRTOS.h"
#include "BoardEvents.h"
#include "pax_http_server.h"

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
     * - Initialize NVS
     * - Initialize the TCP/IP adapter
     * - calls CriticalInit()
     * - reads configuration from NVS
     * - calls BoardInit()
     *
     * The execution is transfered to GoodBye function when:
     * - NVS Initialization fails
     * - CriticalInit does not return ESP_OK
     */
    esp_err_t Initialize(PaxHttpServer *theHttpServer);

    virtual esp_err_t EarlyInit(void) = 0;
    virtual esp_err_t CriticalInit(void) = 0;
    virtual esp_err_t BoardInit(void) = 0;

    /**
     * @brief Executes vTaskDelay every 100ms
     */
    void DoNothingForever(void);

    /**
     * @brief Enters deep sleep for one hour
     */
    void GoodBye(void);

    /**
     * @brief Start the board in APSTA mode for configuration purposes
     */
    esp_err_t StartConfigurationAP(void);

    /**
     * @brief Start the board in AP mode
     */
    esp_err_t StartAP(void);
    /**
     * @brief Stop the AP mode
     */
    void StopAP(void);

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

    esp_err_t InitializeWiFi(void);
    esp_err_t CleanWiFi(void);

    /**
     * @brief Connect to the saved AP
     *
     * Tries to connect and wait for connection to complete or timeout.
     */
    esp_err_t Connect(void);

    /**
     * @brief Disconnect
     */
    void Disconnect(void);

    // TODO: In the Board class add functions for reading ALL board related data, default_mac, chip_info_ app_info, ...

protected:
    EventGroupHandler events;

    PaxHttpServer *httpServer;

    bool initialized;

    /**
     * @brief Placeholder for base MAC address
     */
    uint8_t MAC[6];
};

#endif
