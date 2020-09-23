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

#include <string>

class WiFiConfig
{
public:
    WiFiConfig(void);
    ~WiFiConfig();

    std::string ssid;
    std::string pass;

    void Initialize(void);

    /**
     * @brief Check validity of data
     *
     * For now it only checks:
     * - 1 <= {@code ssid}.length() <= 31
     * - 8 <= {@code pass}.length() <= 63
     */
    bool CheckData(void);

    void SetStationConfig(wifi_config_t*);
    void SetAPConfig(wifi_config_t*);

    void SetFromStrings(const char* strSSID, const char* strPASS);

    /**
     * @brief Builds the name and password for configuration AP
     *
     * Sets {@code ssid} to "name-hex(MAC[3]MAC[4]MAC[5])"
     * If {@code name} or {@code MAC} are nullptr, {@code ssid} will be "pax-device"
     *
     * Sets {@code pass} to "paxxword"
     */
    void SetFromNameAndMAC(const char* name, const uint8_t* MAC);

private:
};

#endif
