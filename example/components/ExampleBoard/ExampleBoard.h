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

#ifndef ExampleBoard_H
#define ExampleBoard_H

#include "Board.h"
#include "ExampleBoardHTTPSrv.h"

/**
 * Hardware
 *
 * Hardware description can be put here
 *
 * Boot button IO0 to ground (needs pull-up)
 */

class ExampleBoard : public Board
{
public:
    ExampleBoard(void);
    virtual ~ExampleBoard(void);

    /**
     * @brief Perform basic hardware initialization
     *
     * This function initialize the pins for hardware connections
     */
    virtual esp_err_t EarlyInit(void);

    virtual esp_err_t CriticalInit(void);

    virtual esp_err_t BoardInit(void);

    virtual esp_err_t PostInit(void);


    esp_err_t StartTheServers(void);
    void StopTheServers(void);
    esp_err_t ConfigureMDNS(void);

    /**
     * @brief Retuns true if the onboard button is pressed
     */
    bool OnboardButtonPressed(void);

    /**
     * @brief The server queue is used to read the commands received by the HTTP server
     */
    QueueHandle_t GetHttpServerQueue(void);

protected:
    ExampleBoardHTTPSrv httpServer;

private:
};

#endif
