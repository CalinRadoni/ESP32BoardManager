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

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include <string>
#include <cstring>

#include "sdkconfig.h"
#include "pax_http_server.h"
#include "Configuration.h"
#include "cJSON.h"

// -----------------------------------------------------------------------------

static const char* TAG = "PaxHttpSrv";

const uint8_t queueLength = 8;

#ifdef CONFIG_ESP32BM_WEB_Compressed_index
extern const uint8_t index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_gz_end[]   asm("_binary_index_html_gz_end");
#else
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
#endif

#ifdef CONFIG_ESP32BM_WEB_USE_favicon
extern const uint8_t index_html_start[] asm("_binary_favicon_ico_start");
extern const uint8_t index_html_end[]   asm("_binary_favicon_ico_end");
#endif

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
    simpleOTA = nullptr;
    configuration = nullptr;
}

PaxHttpServer::~PaxHttpServer()
{
    StopServer();
    DestroyQueue();
}

bool PaxHttpServer::Initialize(void) {
    if (serverQueue != 0) {
        DestroyQueue();
    }
    return CreateQueue();
}

QueueHandle_t PaxHttpServer::GetQueueHandle(void) {
    if (serverQueue == 0) {
        CreateQueue();
    }
    return serverQueue;
}

bool PaxHttpServer::CreateQueue(void)
{
    if (serverQueue != 0) {
        xQueueReset(serverQueue);
        return true;
    }

    serverQueue = xQueueCreate(queueLength, sizeof(HTTPCommand));
    if (serverQueue == 0) {
        ESP_LOGE(TAG, "xQueueCreate");
        return false;
    }
    return true;
}
void PaxHttpServer::DestroyQueue(void)
{
    if (serverQueue == 0) return;

    vQueueDelete(serverQueue);
    serverQueue = 0;
}

esp_err_t PaxHttpServer::StartServer(ESP32SimpleOTA *sOTA, Configuration *boardConfiguration, BoardInfo *boardInfoIn)
{
    if (serverHandle != nullptr)
        StopServer();

    simpleOTA = sOTA;
    if (simpleOTA == nullptr) {
        ESP_LOGE(TAG, "simpleOTA is nullptr");
        return ESP_ERR_INVALID_ARG;
    }

    configuration = boardConfiguration;
    if (configuration == nullptr) {
        ESP_LOGE(TAG, "configuration is nullptr");
        return ESP_ERR_INVALID_ARG;
    }

    boardInfo = boardInfoIn;
    if (boardInfo == nullptr) {
        ESP_LOGE(TAG, "boardInfo is nullptr");
        return ESP_ERR_INVALID_ARG;
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
    esp_err_t res;

    if (str == "/info.json") {
        return HandleGet_InfoJson(req);
    }

    if (str == "/status.json") {
        return HandleGet_StatusJson(req);
    }

    if (str == "/config.json") {
        return HandleGet_ConfigJson(req);
    }

    if ((str == "/") || (str == "/index.html")){
        res = httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
        if (res != ESP_OK) return res;

    #ifdef CONFIG_ESP32BM_WEB_Compressed_index
        res = httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        if (res != ESP_OK) return res;

        return httpd_resp_send(req, (const char *)index_html_gz_start, index_html_gz_end - index_html_gz_start);
    #else
        return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    #endif
    }

    #ifdef CONFIG_ESP32BM_WEB_USE_favicon
    if (str == "/favicon.ico"){
        esp_err_t res = httpd_resp_set_type(req, "image/x-icon");
        if (res != ESP_OK) return res;

        return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    }
    #endif

    if (HandleGET_Custom(req, &res)) {
        return res;
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

// -----------------------------------------------------------------------------

char* PaxHttpServer::CreateJSONInfoString(bool addWhitespaces)
{
    char *str = nullptr;

    if (configuration == nullptr) { return str; }
    if (boardInfo == nullptr) { return str; }

    cJSON *cfg = cJSON_CreateObject();

    if (cJSON_AddStringToObject(cfg, "title", configuration->name.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "tagline", boardInfo->tagline.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }

    if (cJSON_AddStringToObject(cfg, "appName", boardInfo->appName.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "appVersion", boardInfo->appVersion.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "link", boardInfo->link.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "compileTime", boardInfo->compileTime.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "idfVersion", boardInfo->idfVersion.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "elfSHA256", boardInfo->elfSHA256.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }
    if (cJSON_AddStringToObject(cfg, "hwInfo", boardInfo->hwInfo.c_str()) == NULL) {
        cJSON_Delete(cfg);
        return str;
    }

    if (addWhitespaces) { str = cJSON_Print(cfg); }
    else                { str = cJSON_PrintUnformatted(cfg); }

    cJSON_Delete(cfg);
    return str;
}

esp_err_t PaxHttpServer::HandleGet_InfoJson(httpd_req_t* req)
{
    char *str = CreateJSONInfoString(true);
    if (str == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "info.json");
        return ESP_FAIL;
    }

    esp_err_t res = SetJsonHeader(req);
    if(res != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "info.json");
        free(str);
        return res;
    }

    res = httpd_resp_sendstr(req, str);
    free(str);
    return res;
}

// -----------------------------------------------------------------------------

char* PaxHttpServer::CreateJSONStatusString(bool addWhitespaces)
{
    return nullptr;
}

esp_err_t PaxHttpServer::HandleGet_StatusJson(httpd_req_t* req)
{
    char *str = CreateJSONStatusString(true);
    if (str == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "status.json");
        return ESP_FAIL;
    }

    esp_err_t res = SetJsonHeader(req);
    if(res != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "status.json");
        free(str);
        return res;
    }

    res = httpd_resp_sendstr(req, str);
    free(str);
    return res;
}

// -----------------------------------------------------------------------------

esp_err_t PaxHttpServer::HandleGet_ConfigJson(httpd_req_t* req)
{
    if (configuration == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "configuration is null");
        return ESP_FAIL;
    }

    char *str = configuration->CreateJSONConfigString(true);
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

// -----------------------------------------------------------------------------

esp_err_t PaxHttpServer::HandlePostRequest(httpd_req_t* req)
{
    if (req == nullptr) return ESP_FAIL;

    std::string str = req->uri;
    esp_err_t res;

    if (str == "/cmd.json") {
        return HandlePost_CmdJson(req);
    }

    if (str == "/config.json") {
        return HandlePost_ConfigJson(req);
    }

    if (str == "/update") {
        res = HandleOTA(req);
        if (res == ESP_OK) {
            httpd_resp_sendstr(req, "OTA OK.");
        }
        else {
            httpd_resp_sendstr(req, "OTA Failed !");
        }
        return res;
    }

    if (HandlePOST_Custom(req, &res)) {
        return res;
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
    if (root == nullptr) {
        const char *errStr = cJSON_GetErrorPtr();
        if (errStr != nullptr) {
            ESP_LOGE(TAG, "JSON error before: \"%s\"", errStr);
        }
    }
    else {
        cJSON *item;

        item = cJSON_GetObjectItem(root, "cmd");
        if (item != nullptr) {
            if (cJSON_IsNumber(item)) {
                cmd.command = (uint8_t)item->valueint;
            }
        }

        item = cJSON_GetObjectItem(root, "data");
        if (item != nullptr) {
            if (cJSON_IsNumber(item)) {
                cmd.data = (uint32_t)item->valuedouble;
            }
            else {
                if (cJSON_IsString(item)) {
                    if (item->valuestring != nullptr)
                        cmd.data = (uint32_t)strtoul(item->valuestring, nullptr, 10);
                }
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
    size_t totalLen = req->content_len;
    size_t curLen = 0;
    size_t recLen = 0;

    if (totalLen >= workBufferSize) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "config content too long");
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

    if (configuration == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "configuration is null");
        return ESP_FAIL;
    }

    bool res = configuration->SetFromJSONString(workBuffer);
    if (!res) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to process data");
        return ESP_FAIL;
    }

    esp_err_t err = configuration->WriteToNVS(false);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to process data");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_sendstr(req, "Command processed");
    return ESP_OK;
}

// -----------------------------------------------------------------------------

bool PaxHttpServer::HandleGET_Custom(httpd_req_t *req, esp_err_t *res)
{
    return false;
}

bool PaxHttpServer::HandlePOST_Custom(httpd_req_t *req, esp_err_t *res)
{
    return false;
}

// -----------------------------------------------------------------------------

esp_err_t PaxHttpServer::HandleOTA(httpd_req_t* req)
{
    const size_t buffSize = 1024;
    char buff[buffSize];
    int pageLen = req->content_len;
    int rxLen;
    bool headerReceived = false;
    bool done = false;
    esp_err_t result = ESP_OK;

    if (simpleOTA == nullptr) {
        ESP_LOGE(TAG, "OTA is null !");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "OTA is not available !");
        return ESP_FAIL;
    }

    if (pageLen > simpleOTA->GetMaxImageSize()) {
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

                result = simpleOTA->Begin();
                if (result != ESP_OK) { return result; }
            }

            result = simpleOTA->Write(buff, rxLen);
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

    result = simpleOTA->End();
    if (result != ESP_OK) { return result; }

    ESP_LOGI(TAG, "OTA done, you should restart");

    return result;
}
