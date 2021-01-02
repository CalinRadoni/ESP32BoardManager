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

#ifndef Configuration_H
#define Configuration_H

#include "freertos/FreeRTOS.h"
#include "cJSON.h"
#include "WiFiConfig.h"

const uint8_t NameBufLen = 64;
const uint8_t WiFiConfigCnt = 2;
const uint8_t ipv4BufLen = 16;

class Configuration
{
public:
    Configuration(void);
    virtual ~Configuration();

    uint32_t version;
    std::string name;
    std::string pass;
    WiFiConfig ap[WiFiConfigCnt];
    char ipAddr[ipv4BufLen];
    char ipMask[ipv4BufLen];
    char ipGateway[ipv4BufLen];
    char ipDNS[ipv4BufLen];

    void InitData(void);

    esp_err_t InitializeNVS(void);

    esp_err_t ReadFromNVS(void);
    esp_err_t WriteToNVS(bool eraseAll);

    /**
     * @warning Delete returned string with 'free' !
     */
    char* CreateJSONConfigString(bool addWhitespaces);

    bool SetFromJSONString(char*);

protected:
    virtual bool CreateJSON_CustomData(cJSON*);

    virtual bool SetFromJSONString_CustomData(cJSON*);

    bool SetIntFromJSON(int&, const char *id, cJSON *jstr);
    bool SetStringFromJSON(std::string& str, const char *id, cJSON *jstr);
    bool SetStringFromJSON(char *str, uint8_t len, const char *id, cJSON *jstr);

private:
};

#endif
