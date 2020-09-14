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

#ifndef WiFiConfig_H
#define WiFiConfig_H

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"

const uint8_t SSIDBufLen = 33;
const uint8_t PassBufLen = 65;

class WiFiConfig
{
public:
    WiFiConfig(void);
    ~WiFiConfig();

    uint8_t SSID[SSIDBufLen]; // minimum length for a valid SSID is 1
    uint8_t Pass[PassBufLen]; // minimum length for a valid WPA2 password is 8

    void Initialize(void);

    bool CheckData(void);

    void SetStationConfig(wifi_config_t*);
    void SetAPConfig(wifi_config_t*);

    void SetFromStrings(const char* strSSID, const char* strPASS);

    /**
     * @brief Builds the name and password for configuration AP
     *
     * Creates the SSID and PASS as "name-hex(MAC[3]MAC[4]MAC[5])"
     */
    void SetFromNameAndMAC(const char* name, const uint8_t* MAC);

private:
};

#endif
