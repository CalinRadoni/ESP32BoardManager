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

#include "WiFiConfig.h"

#include "esp_err.h"
#include "esp_wifi.h"

#include <string>
#include <cstring>

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
    ssid.clear();
    pass.clear();
}

bool WiFiConfig::CheckData(void)
{
    if (ssid.length() < 1) return false;
    if (ssid.length() > 31) return false;
    if (pass.length() < 1) return false;
    if (pass.length() > 63) return false;

    return true;
}

void WiFiConfig::SetStationConfig(wifi_config_t* cfg)
{
    if (cfg == nullptr) return;

    std::memset(cfg, 0, sizeof(wifi_config_t));

    std::size_t dstLen = (sizeof cfg->sta.ssid) - 1;
    std::size_t copyLen = ssid.length();
    if (copyLen > dstLen)
        copyLen = dstLen;
    std::memcpy(cfg->sta.ssid, ssid.c_str(), copyLen);
    cfg->sta.ssid[copyLen] = 0;

    dstLen = (sizeof cfg->sta.password) - 1;
    copyLen = pass.length();
    if (copyLen > dstLen)
        copyLen = dstLen;
    std::memcpy(cfg->sta.password, pass.c_str(), copyLen);
    cfg->sta.password[copyLen] = 0;
}

void WiFiConfig::SetAPConfig(wifi_config_t* cfg)
{
    if (cfg == nullptr) return;

    std::memset(cfg, 0, sizeof(wifi_config_t));

    std::size_t dstLen = (sizeof cfg->ap.ssid) - 1;
    std::size_t copyLen = ssid.length();
    if (copyLen > dstLen)
        copyLen = dstLen;
    std::memcpy(cfg->ap.ssid, ssid.c_str(), copyLen);
    cfg->ap.ssid[copyLen] = 0;

    cfg->ap.ssid_len = 0;

    dstLen = (sizeof cfg->ap.password) - 1;
    copyLen = pass.length();
    if (copyLen > dstLen)
        copyLen = dstLen;
    std::memcpy(cfg->ap.password, pass.c_str(), copyLen);
    cfg->ap.password[copyLen] = 0;
}

void WiFiConfig::SetFromStrings(const char* strSSID, const char* strPASS)
{
    Initialize();

    ssid.clear();
    if (strSSID != nullptr)
        ssid = strSSID;

    pass.clear();
    if (strPASS != nullptr)
        pass = strPASS;
}
