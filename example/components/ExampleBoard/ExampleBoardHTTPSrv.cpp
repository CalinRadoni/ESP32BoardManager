/**
This file is part of ESP32BoardManager esp-idf component
(https://github.com/CalinRadoni/ESP32BoardManager)
Copyright (C) 2020 by Calin Radoni

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

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include <string.h>

#include "ExampleBoardHTTPSrv.h"

// -----------------------------------------------------------------------------

ExampleBoardHTTPSrv::ExampleBoardHTTPSrv() : PaxHttpServer()
{
    exampleStatusData = 0;
}

ExampleBoardHTTPSrv::~ExampleBoardHTTPSrv(void)
{
    //
}

// -----------------------------------------------------------------------------

char* ExampleBoardHTTPSrv::CreateJSONStatusString(bool addWhitespaces)
{
    char *str = nullptr;

    if (configuration == nullptr) { return str; }
    if (boardInfo == nullptr) { return str; }

    ++exampleStatusData;

    cJSON *cfg = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(cfg, "exampleStatusData", exampleStatusData) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }

    if (addWhitespaces) { str = cJSON_Print(cfg); }
    else                { str = cJSON_PrintUnformatted(cfg); }

    cJSON_Delete(cfg);
    return str;
}
