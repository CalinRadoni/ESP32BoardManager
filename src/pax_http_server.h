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

#ifndef pax_http_server_H
#define pax_http_server_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"

#include "ESP32SimpleOTA.h"
#include "Configuration.h"
#include "BoardInfo.h"

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
     * Currently only creates the serverQueue.
     * If serverQueue exists, is destroyed and created again.
     */
    bool Initialize(void);

    /**
     * If serverQueue is not created tries to create one.
     * Returns the serverQueue.
     */
    QueueHandle_t GetQueueHandle(void);

    esp_err_t StartServer(ESP32SimpleOTA*, Configuration*, BoardInfo*);
    void StopServer(void);

    esp_err_t HandleRequest(httpd_req_t*);

protected:
    /**
     * The queue for http server events.
     * Is created by Initialize or GetQueueHandle functions.
     * Is destroyed by destructor
     */
    QueueHandle_t serverQueue;

    /**
     * If queue is allready created, is emptied otherwise is created
     */
    bool CreateQueue(void);
    void DestroyQueue(void);

    httpd_handle_t serverHandle;
    bool working;

    char workBuffer[workBufferSize];

    esp_err_t HandleGetRequest(httpd_req_t*);
    esp_err_t HandlePostRequest(httpd_req_t*);

    virtual esp_err_t SetJsonHeader(httpd_req_t*);
    virtual esp_err_t HandleGet_InfoJson(httpd_req_t*);
    virtual esp_err_t HandleGet_StatusJson(httpd_req_t*);
    virtual esp_err_t HandleGet_ConfigJson(httpd_req_t*);

    virtual esp_err_t HandlePost_CmdJson(httpd_req_t*);
    virtual esp_err_t HandlePost_ConfigJson(httpd_req_t*);

    /**
     * @brief Handles custom GET paths
     *
     * If this function returns false, a HTTPD_404_NOT_FOUND will be returned.
     *
     * You should handle all the cases, errors included, and return true.
     */
    virtual bool HandleGET_Custom(httpd_req_t*, esp_err_t*);

    /**
     * @brief Handles custom POST paths
     *
     * If this function returns false, a HTTPD_404_NOT_FOUND will be returned.
     *
     * You should handle all the cases, errors included, and return true.
     */
    virtual bool HandlePOST_Custom(httpd_req_t*, esp_err_t*);

    ESP32SimpleOTA *simpleOTA;
    esp_err_t HandleOTA(httpd_req_t*);

    Configuration *configuration;

    BoardInfo *boardInfo;

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
