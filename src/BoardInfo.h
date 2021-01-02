/**
This file is part of ESP32BoardManager esp-idf component
(https://github.com/CalinRadoni/ESP32BoardManager)
Copyright (C) 2021 by Calin Radoni

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

#ifndef BoardInfo_H
#define BoardInfo_H

#include "freertos/FreeRTOS.h"

#include <string>

class BoardInfo
{
public:
    BoardInfo(void);
    virtual ~BoardInfo();

    std::string tagline;
    std::string link;

    std::string appName;
    std::string appVersion;
    std::string compileTime;
    std::string idfVersion;
    std::string elfSHA256;

    std::string hwInfo;
};

#endif
