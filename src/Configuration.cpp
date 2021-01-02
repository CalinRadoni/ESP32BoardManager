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

#include <vector>
#include <cstring>
#include <algorithm>

#include "nvs_flash.h"
#include "nvs.h"

#include "Configuration.h"

// -----------------------------------------------------------------------------

static const char* TAG = "Configuration";

const uint32_t ConfigurationVersion = 3;
const char* ConfigNVS = "pax-config";
const char* ConfigJSON = "cfgJSON";

// -----------------------------------------------------------------------------

Configuration::Configuration(void)
{
    InitData();
}

Configuration::~Configuration()
{
    //
}

void Configuration::InitData(void)
{
    version = ConfigurationVersion;
    name.clear();
    pass.clear();

    for (uint8_t i = 0; i < WiFiConfigCnt; ++i)
        ap->Initialize();

    std::fill(ipAddr, ipAddr + ipv4BufLen, static_cast<char>(0));
    std::fill(ipMask, ipMask + ipv4BufLen, static_cast<char>(0));
    std::fill(ipGateway, ipGateway + ipv4BufLen, static_cast<char>(0));
    std::fill(ipDNS, ipDNS + ipv4BufLen, static_cast<char>(0));
}

esp_err_t Configuration::InitializeNVS(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "0x%x nvs_flash_erase", err);
        }
        else {
            err = nvs_flash_init();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "0x%x nvs_flash_init", err);
            }
        }
    }
    return err;
}

bool Configuration::CreateJSON_CustomData(cJSON*)
{
    return true;
}

char* Configuration::CreateJSONConfigString(bool addWhitespaces)
{
    char *str = nullptr;

    cJSON *cfg = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(cfg, "version", version) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "name", name.c_str()) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "pass", name.c_str()) == NULL) { cJSON_Delete(cfg); return str; }

    if (cJSON_AddStringToObject(cfg, "ap1s", ap[0].ssid.c_str()) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ap1p", ap[0].pass.c_str()) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ap2s", ap[1].ssid.c_str()) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ap2p", ap[1].pass.c_str()) == NULL) { cJSON_Delete(cfg); return str; }

    if (cJSON_AddStringToObject(cfg, "ipAddr", ipAddr) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ipMask", ipMask) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ipGateway", ipGateway) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ipDNS", ipDNS) == NULL) { cJSON_Delete(cfg); return str; }

    if (!CreateJSON_CustomData(cfg)) { cJSON_Delete(cfg); return str; }

    if (addWhitespaces) { str = cJSON_Print(cfg); }
    else                { str = cJSON_PrintUnformatted(cfg); }

    cJSON_Delete(cfg);
    return str;
}

bool Configuration::SetFromJSONString_CustomData(cJSON*)
{
    return true;
}

bool Configuration::SetIntFromJSON(int& value, const char *id, cJSON *jstr)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(jstr, id);
    if (item == NULL) return false;
    if (!cJSON_IsNumber(item)) return false;

    value = item->valueint;
    return true;
}

bool Configuration::SetStringFromJSON(std::string& str, const char *id, cJSON *jstr)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(jstr, id);
    if (item == NULL) return false;
    if (!cJSON_IsString(item)) return false;

    if (item->valuestring == NULL){
        str.clear();
    }
    else {
        str = item->valuestring;
    }
    return true;
}

bool Configuration::SetStringFromJSON(char *str, uint8_t len, const char *id, cJSON *jstr)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(jstr, id);
    if (item == NULL) return false;
    if (!cJSON_IsString(item)) return false;

    std::fill(str, str + len, static_cast<char>(0));
    if (item->valuestring != NULL) {
        strncpy(str, item->valuestring, len);
        str[len - 1] = 0;
    }

    return true;
}

bool Configuration::SetFromJSONString(char *jsonStr)
{
    cJSON *cfg = cJSON_Parse(jsonStr);
    if (cfg == NULL) {
        const char *errStr = cJSON_GetErrorPtr();
        if (errStr != NULL) {
            ESP_LOGE(TAG, "JSON error before: \"%s\"", errStr);
        }
        return false;
    }

    // we have a valid JSON string, check if the version is OK
    int value = 0;
    bool res = SetIntFromJSON(value, "version", cfg);
    if (!res){
        cJSON_Delete(cfg);
        return false;
    }
    version = value;
    if (version != ConfigurationVersion) {
        // if the version is not the one I know exit with error
        cJSON_Delete(cfg);
        return false;
    }

    // set everything to default
    InitData();

    // these are NOT critical configuration values ...
    // ... so the exit code is not checked, any of these may be missing
    // the default values are set by the previous call to InitData
    SetStringFromJSON(name, "name", cfg);
    SetStringFromJSON(pass, "pass", cfg);
    SetStringFromJSON(ap[0].ssid, "ap1s", cfg);
    SetStringFromJSON(ap[0].pass, "ap1p", cfg);
    SetStringFromJSON(ap[1].ssid, "ap2s", cfg);
    SetStringFromJSON(ap[1].pass, "ap2p", cfg);

    SetStringFromJSON(ipAddr, ipv4BufLen, "ipAddr", cfg);
    SetStringFromJSON(ipMask, ipv4BufLen, "ipMask", cfg);
    SetStringFromJSON(ipGateway, ipv4BufLen, "ipGateway", cfg);
    SetStringFromJSON(ipDNS, ipv4BufLen, "ipDNS", cfg);

    SetFromJSONString_CustomData(cfg);

    cJSON_Delete(cfg);

    return res;
}

esp_err_t Configuration::ReadFromNVS(void)
{
    nvs_handle_t nvsHandle;

    InitData();

    esp_err_t err = nvs_open(ConfigNVS, NVS_READONLY, &nvsHandle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // The namespace does not exists yet
        // Write a default configuration, no erase needed
        ESP_LOGW(TAG, "0x%x nvs_open. Creating the namespace and default config.", err);
        nvs_close(nvsHandle);
        return WriteToNVS(false);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x nvs_open NVS_READONLY", err);
        nvs_close(nvsHandle);
        return err;
    }

    size_t strBufLen = 0;
    err = nvs_get_str(nvsHandle, ConfigJSON, NULL, &strBufLen);
    if (strBufLen == 0) {
        ESP_LOGE(TAG, "0x%x nvs_get_str required length", err);
        nvs_close(nvsHandle);
        return err;
    }

    char *str = new (std::nothrow) char[strBufLen];
    if (str == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate char buffer");
        nvs_close(nvsHandle);
        return err;
    }
    err = nvs_get_str(nvsHandle, ConfigJSON, str, &strBufLen);
    if (err != ESP_OK) {
        delete[] str;
        ESP_LOGE(TAG, "0x%x nvs_get_str", err);
        nvs_close(nvsHandle);
        return err;
    }
    if (!SetFromJSONString(str)) {
        delete[] str;
        ESP_LOGE(TAG, "SetFromJSONString failed");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }
    delete[] str;

    if (version != ConfigurationVersion){
        nvs_close(nvsHandle);
        return WriteToNVS(true);
    }

    nvs_close(nvsHandle);

    return ESP_OK;
}

esp_err_t Configuration::WriteToNVS(bool eraseAll)
{
    nvs_handle_t nvsHandle;

    esp_err_t err = nvs_open(ConfigNVS, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "0x%x Failed to open NVS in readwrite mode", err);
        return err;
    }

    if (eraseAll) {
        err = nvs_erase_all(nvsHandle);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            ESP_LOGE(TAG, "0x%x Failed to erase", err);
            return err;
        }
        err = nvs_commit(nvsHandle);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            ESP_LOGE(TAG, "0x%x Failed to commit", err);
            return err;
        }
    }

    char *str = CreateJSONConfigString(false);
    if (str == nullptr) {
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }

    err = nvs_set_str(nvsHandle, ConfigJSON, str);
    free(str);
    if (err != ESP_OK) {
        nvs_close(nvsHandle);
        return err;
    }
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        nvs_close(nvsHandle);
        return err;
    }

    nvs_close(nvsHandle);
    return ESP_OK;
}
