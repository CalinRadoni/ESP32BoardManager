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

#ifndef pax_http_server_H
#define pax_http_server_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

struct HTTPCommand
{
    uint8_t command;
    uint32_t data;

    HTTPCommand() {
        command = 0;
        data = 0;
    }
};

const size_t workBufferSize = 1000;

class PaxHttpServer
{
public:
    PaxHttpServer(void);
    virtual ~PaxHttpServer();

    /**
     * @brief The queue for timer events
     *
     * The queue is created by the StartServer function and destroyed by the StopServer function or on the destructor
     */
    QueueHandle_t serverQueue;

    esp_err_t StartServer(void);
    void StopServer(void);

    esp_err_t HandleRequest(httpd_req_t*);

protected:
    httpd_handle_t serverHandle;
    bool working;

    char workBuffer[workBufferSize];

    esp_err_t HandleGetRequest(httpd_req_t*);
    esp_err_t HandlePostRequest(httpd_req_t*);

    virtual esp_err_t SetJsonHeader(httpd_req_t*);
    virtual esp_err_t HandleGet_StatusJson(httpd_req_t*);
    virtual esp_err_t HandleGet_ConfigJson(httpd_req_t*);

    virtual esp_err_t HandlePost_CmdJson(httpd_req_t*);
    virtual esp_err_t HandlePost_ConfigJson(httpd_req_t*);
    esp_err_t HandleOTA(httpd_req_t*);
};

#endif
