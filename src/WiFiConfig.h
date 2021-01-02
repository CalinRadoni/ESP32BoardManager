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

private:
};

#endif
