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

#ifndef WiFiManager_H
#define WiFiManager_H

#include "freertos/FreeRTOS.h"
#include "BoardEvents.h"

#include "WiFiConfig.h"

enum class WiFiManagerStatus {
    idle,
    staConnecting, staConnected, staDisconnected,
    apCreated,
    scanInitiated, scanDone,
    error
};

enum class WiFiManagerMode {
    none, station, ap, apsta, scan
};

class WiFiManager {
public:
    WiFiManager(void);
    ~WiFiManager(void);

    EventGroupHandler *events;

    /**
     * @brief Initialize WiFi driver
     */
    esp_err_t Initialize(void);

    /**
     * @brief Clean after the Initialize function
     */
    esp_err_t Clean(void);

    /**
     * @brief Starts the WiFi in the requested mode
     *
     * Starts the WiFi in one of the modes:
     *    * station
     *    * ap
     *    * ap + station
     *    * scan
     *
     * NO checks are made to stop the current mode !
     * Call the functions in order !
     *
     * @example Order example
     * @code{.cpp}
     * theWiFiManager.Initialize();
     *
     * err = theWiFiManager.Start(a_mode);
     * ...
     * theWiFiManager.Stop(true);
     *
     * ...
     *
     * err = theWiFiManager.Start(another_mode);
     * ...
     * theWiFiManager.Stop(true);
     *
     * theWiFiManager.CleanUp();
     * @endcode
     *
     * @param  initMode one of WiFiManagerMode types
     * @param  SSID     should be uint8_t[32]
     * @param  PASS     should be uint8_t[64]
     * @return ESP_OK on success
     *
     * @warning Scanning stops any other mode !
     * Scanning can be done in Station or Station+AP modes without stop/disconnect but this function does not implement that behaviour
     *
     * @note 1. Connection in station mode is done when status changes to staConnected.
     * Wait for that value then start sockets, etc.
     * @code{.cpp}
     * if (theWiFiManager.Status() == WiFiManagerStatus::staConnected) {
     *     // open your sockets, create servers, etc.
     * }
     *
     * @note 2. Reconnection in station mode must be done manually.
     * Check for staDisconnected status value then close your sockets, etc.
     *
     * @note 3. Scanning is implemented as Active scan.
     *
     * @note 4. After scanning foundAPInfo remains with data.
     * Free it with FreeScanRecords when / if you do not need it.
     */
    esp_err_t Start(WiFiManagerMode initMode, WiFiConfig* staCfg, WiFiConfig* apCfg);

    /**
     * @brief Stop current WiFi mode
     *
     * This function stops the current WiFi mode.
     * If requested and STA or APSTA is connected to an Access Point it will wait
     * for it to disconnect but no more then msToWaitForDisconnect miliseconds.
     */
    esp_err_t Stop(bool wait);

    /**
     * @brief Get some informations about connected AP
     *
     * If in station or ap+station mode and the station is connected to an AP
     * returns a pointer to the private variable apRecord.
     *
     * Otherways will return nullptr.
     */
    wifi_ap_record_t* GetAPInfo(void);

    /**
     * @brief Return the current status of WiFiManager
     */
    WiFiManagerStatus Status(void);

    /**
     * @brief Returns the number of Access Points found on last scan
     */
    uint16_t GetScanRecordCount(void);

    /**
     * @brief Return informations about one of the Access Points found on last scan
     */
    wifi_ap_record_t* GetScanRecord(uint16_t);

    /**
     * @brief Release the memory used to store informations about Access Points found on last scan
     */
    void FreeScanRecords(void);

    /**
     * @brief Returns the disconnect reason
     *
     * The disconnect reason is set when the WIFI_EVENT_STA_DISCONNECTED event is received
     */
    uint8_t GetDisconnectReason(void);

    /**
     * @brief Event handler for default event loop
     *
     * This function handles the events for the event loop created with esp_event_loop_create_default function.
     *
     * This function need not to be called directly !
     * This function is called automatically from the default event loop.
     */
    void EventHandler(esp_event_base_t event_base, int32_t event_id, void* event_data);

private:
    WiFiManagerStatus workStatus;
    WiFiManagerMode workMode;

    esp_netif_t* defaultAP;
    esp_netif_t* defaultSTA;
    void CleanDefaultAPSTA(void);

    esp_err_t ConfigStation(WiFiConfig *cfg);
    esp_err_t ConfigAP(WiFiConfig *cfg);
    esp_err_t ConfigAPStation(WiFiConfig* staCfg, WiFiConfig* apCfg);
    esp_err_t ConfigStationScan(void);
    esp_err_t StartActiveScan(void);

    wifi_ap_record_t apRecord;
    uint8_t disconnectReason;

    wifi_ap_record_t *foundAPInfo;
    uint16_t foundAPcnt;
    void ExtractAPScanResults(void);
};

#endif
