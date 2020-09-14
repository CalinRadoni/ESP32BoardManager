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

#include "WiFiConfig.h"

#include "esp_err.h"
#include "esp_wifi.h"

#include <cstring>
#include <algorithm>

WiFiConfig::WiFiConfig(void)
{
    Initialize();
}

WiFiConfig::~WiFiConfig(void)
{
    //
}

void WiFiConfig::Initialize(void)
{
    std::fill(SSID, SSID + SSIDBufLen, static_cast<uint8_t>(0));
    std::fill(Pass, Pass + PassBufLen, static_cast<uint8_t>(0));
}

bool WiFiConfig::CheckData(void)
{
    if (strlen((char*)SSID) < 1) return false;
    if (strlen((char*)Pass) < 8)  return false;

    return true;
}

void WiFiConfig::SetStationConfig(wifi_config_t* cfg)
{
    if (cfg == nullptr) return;

    memset(cfg, 0, sizeof(wifi_config_t));
    memcpy(cfg->sta.ssid, SSID, 31 * sizeof(uint8_t));
           cfg->sta.ssid[31] = 0;
    memcpy(cfg->sta.password, Pass, 63 * sizeof(uint8_t));
           cfg->sta.password[63] = 0;
}

void WiFiConfig::SetAPConfig(wifi_config_t* cfg)
{
    if (cfg == nullptr) return;

    memset(cfg, 0, sizeof(wifi_config_t));
    memcpy(cfg->ap.ssid, SSID, 31 * sizeof(uint8_t));
           cfg->ap.ssid[31] = 0;
           cfg->ap.ssid_len = 0;
    memcpy(cfg->ap.password, Pass, 63 * sizeof(uint8_t));
           cfg->ap.password[63] = 0;
}

void WiFiConfig::SetFromStrings(const char* strSSID, const char* strPASS)
{
    Initialize();

    if (strSSID != nullptr) {
        strncpy((char*)SSID, strSSID, 31);
        SSID[31] = 0;
    }

    if (strPASS != nullptr) {
        strncpy((char*)Pass, strPASS, 63);
        Pass[63] = 0;
    }
}

void WiFiConfig::SetFromNameAndMAC(const char* name, const uint8_t* MAC)
{
    Initialize();

    if ((name == nullptr) || (MAC == nullptr)) return;

    snprintf((char*)SSID, 32, "%s-%02X%02X%02X", name, MAC[3], MAC[4], MAC[5]);
    memcpy(Pass, SSID, 32);
}
