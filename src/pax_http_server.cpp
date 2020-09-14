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

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include <string>
#include <cstring>

#include "pax_http_server.h"
#include "Configuration.h"
#include "cJSON.h"
#include "ESP32SimpleOTA.h"

// -----------------------------------------------------------------------------

static const char* TAG = "PaxHttpSrv";

const uint8_t queueLength = 8;

extern const uint8_t index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_gz_end[]   asm("_binary_index_html_gz_end");

// -----------------------------------------------------------------------------

static esp_err_t request_handler(httpd_req_t *req)
{
    if (req == nullptr) return ESP_FAIL;

    PaxHttpServer* server = (PaxHttpServer *) req->user_ctx;
    if (server == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "user_ctx is nullptr");
        return ESP_FAIL;
    }

    return server->HandleRequest(req);
}

// -----------------------------------------------------------------------------

PaxHttpServer::PaxHttpServer(void)
{
    serverQueue = 0;
    serverHandle = nullptr;
    working = false;
}

PaxHttpServer::~PaxHttpServer()
{
    StopServer();
}

esp_err_t PaxHttpServer::StartServer(void)
{
    if (serverHandle != nullptr)
        StopServer();

    serverQueue = xQueueCreate(queueLength, sizeof(HTTPCommand));
    if (serverQueue == 0) {
        ESP_LOGE(TAG, "xQueueCreate");
        return ESP_FAIL;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t err = httpd_start(&serverHandle, &config);
    if (err != ESP_OK) {
        serverHandle = nullptr;
        ESP_LOGE(TAG, "%d httpd_start", err);
        return err;
    }

    httpd_uri_t uri_get = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = request_handler,
        .user_ctx = nullptr
    };

    httpd_uri_t uri_post = {
        .uri      = "/*",
        .method   = HTTP_POST,
        .handler  = request_handler,
        .user_ctx = nullptr
    };

    uri_get.user_ctx = this;
    httpd_register_uri_handler(serverHandle, &uri_get);
    uri_post.user_ctx = this;
    httpd_register_uri_handler(serverHandle, &uri_post);

    working = true;

    return err;
}

void PaxHttpServer::StopServer(void)
{
    if (serverHandle == nullptr) return;

    working = false;

    httpd_stop(serverHandle);
    serverHandle = nullptr;

    if (serverQueue != 0) {
        vQueueDelete(serverQueue);
        serverQueue = 0;
    }
}

esp_err_t PaxHttpServer::HandleRequest(httpd_req_t* req)
{
    if (req == nullptr) return ESP_FAIL;

    if (!working) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not working");
        return ESP_FAIL;
    }

    switch (req->method) {
    case HTTP_GET:
        return HandleGetRequest(req);
    case HTTP_POST:
        return HandlePostRequest(req);
    default:
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Method not implemented");
        return ESP_FAIL;
    }
}

esp_err_t PaxHttpServer::HandleGetRequest(httpd_req_t* req)
{
    if (req == nullptr) return ESP_FAIL;

    std::string str = req->uri;

    if (str == "/status.json") {
        return HandleGet_StatusJson(req);
    }

    if (str == "/config.json") {
        return HandleGet_ConfigJson(req);
    }

    if ((str == "/") || (str == "/index.html")){
        esp_err_t res = httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
        if (res != ESP_OK) return res;

        res = httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        if (res != ESP_OK) return res;

        return httpd_resp_send(req, (const char *)index_html_gz_start, index_html_gz_end - index_html_gz_start);
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 :)");
    return ESP_FAIL;
}

esp_err_t PaxHttpServer::SetJsonHeader(httpd_req_t* req)
{
    esp_err_t res = httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    if (res != ESP_OK) return res;

    res = httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    if (res != ESP_OK) return res;

    return httpd_resp_set_hdr(req, "Pragma", "no-cache");
}

esp_err_t PaxHttpServer::HandleGet_StatusJson(httpd_req_t* req)
{
    esp_err_t res = SetJsonHeader(req);
    if(res != ESP_OK) return res;

    uint8_t statusVal = 0;
    snprintf(workBuffer, workBufferSize, "{\"status\":%d}\n", statusVal);

    return httpd_resp_sendstr(req, workBuffer);
}

esp_err_t PaxHttpServer::HandleGet_ConfigJson(httpd_req_t* req)
{
    char *str = configuration.CreateJSONConfigString(true);
    if (str == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "config.json");
        return ESP_FAIL;
    }

    esp_err_t res = SetJsonHeader(req);
    if(res != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "config.json");
        free(str);
        return res;
    }

    res = httpd_resp_sendstr(req, str);
    free(str);
    return res;
}

esp_err_t PaxHttpServer::HandlePostRequest(httpd_req_t* req)
{
    if (req == nullptr) return ESP_FAIL;

    std::string str = req->uri;

    if (str == "/cmd.json") {
        return HandlePost_CmdJson(req);
    }

    if (str == "/config.json") {
        return HandlePost_ConfigJson(req);
    }

    if (str == "/update") {
        esp_err_t err = HandleOTA(req);
        if (err == ESP_OK) {
            httpd_resp_sendstr(req, "OTA OK.");
        }
        else {
            httpd_resp_sendstr(req, "OTA Failed !");
        }
        return err;
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 :)");
    return ESP_FAIL;
}

esp_err_t PaxHttpServer::HandlePost_CmdJson(httpd_req_t* req)
{
    size_t totalLen = req->content_len;
    size_t curLen = 0;
    size_t recLen = 0;

    if (totalLen >= workBufferSize) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "cmd content too long");
        return ESP_FAIL;
    }
    while (curLen < totalLen) {
        recLen = httpd_req_recv(req, &workBuffer[curLen], totalLen - curLen);
        if (recLen <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to receive data");
            return ESP_FAIL;
        }
        curLen += recLen;
    }
    workBuffer[totalLen] = '\0';

    HTTPCommand cmd;

    cJSON *root = cJSON_Parse(workBuffer);
    if (root != nullptr) {
        cJSON *item;
        item = cJSON_GetObjectItem(root, "cmd");
        if (item != nullptr) {
            cmd.command = (uint8_t)item->valueint;
        }
        item = cJSON_GetObjectItem(root, "data");
        if (item != nullptr) {
            if (item->type == cJSON_Number) {
                cmd.data = (uint32_t)item->valuedouble;
            }
            else {
                cmd.data = (uint32_t)strtoul(item->valuestring, nullptr, 10);
            }
        }

        cJSON_Delete(root);
        root = nullptr;
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");

    if (cmd.command == 0) {
        httpd_resp_sendstr(req, "Command ignored");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Command processed");

    if (serverQueue != 0) {
        xQueueSendToBack(serverQueue, &cmd, (TickType_t)0);
    }

    return ESP_OK;
}

esp_err_t PaxHttpServer::HandlePost_ConfigJson(httpd_req_t* req)
{
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "config.json");
    return ESP_FAIL;
}

esp_err_t PaxHttpServer::HandleOTA(httpd_req_t* req)
{
    const size_t buffSize = 1024;
    char buff[buffSize];
    int pageLen = req->content_len;
    int rxLen;
    bool headerReceived = false;
    bool done = false;
    esp_err_t result = ESP_OK;

    if (pageLen > simpleOTA.GetMaxImageSize()) {
        ESP_LOGE(TAG, "OTA content is too big (%d bytes) !", pageLen);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "OTA content is too big !");
        return ESP_FAIL;
    }

    while (!done) {
        size_t reqLen = pageLen > buffSize ? buffSize : pageLen;
        rxLen = httpd_req_recv(req, buff, reqLen);
        if (rxLen < 0) {
            if (rxLen != HTTPD_SOCK_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "httpd_req_recv: %d", rxLen);
                return ESP_FAIL;
            }
        }
        else if (rxLen == 0) {
            done = true;
        }
        else { /* rxLen > 0 */
            if (!headerReceived) {
                headerReceived = true;

                result = simpleOTA.Begin();
                if (result != ESP_OK) { return result; }
            }

            result = simpleOTA.Write(buff, rxLen);
            if (result != ESP_OK) { return result; }

            if (pageLen > rxLen) {
                pageLen -= rxLen;
            }
            else done = true;
        }
    }

    if (!headerReceived) {
        ESP_LOGE(TAG, "Firmware file - no data received");
        return ESP_FAIL;
    }

    result = simpleOTA.End();
    if (result != ESP_OK) { return result; }

    ESP_LOGI(TAG, "OTA done, you should restart");

    return result;
}
