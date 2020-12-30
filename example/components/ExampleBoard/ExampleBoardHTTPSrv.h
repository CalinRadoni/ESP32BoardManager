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

#ifndef ExampleBoardHTTPSrv_H
#define ExampleBoardHTTPSrv_H

#include "pax_http_server.h"

class ExampleBoardHTTPSrv : public PaxHttpServer
{
public:
    ExampleBoardHTTPSrv(void);
    virtual ~ExampleBoardHTTPSrv();

    std::string tagline;

protected:
    /**
     * @warning Delete returned string with 'free' !
     */
    virtual char* CreateJSONInfoString(bool addWhitespaces);

    /**
     * @warning Delete returned string with 'free' !
     */
    virtual char* CreateJSONStatusString(bool addWhitespaces);
};

#endif
