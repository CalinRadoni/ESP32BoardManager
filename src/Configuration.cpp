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

#include <cstring>
#include <algorithm>

#include "nvs_flash.h"
#include "nvs.h"

#include "Configuration.h"

// -----------------------------------------------------------------------------

static const char* TAG = "Configuration";

const uint32_t ConfigurationVersion = 2;
const char* ConfigNVS = "pax-config";

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
    std::fill(name, name + NameBufLen, static_cast<char>(0));

    for (uint8_t i = 0; i < WiFiConfigCnt; ++i)
        apCfg->Initialize();

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
            ESP_LOGE(TAG, "%d nvs_flash_erase", err);
        }
        else {
            err = nvs_flash_init();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "%d nvs_flash_init", err);
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
    if (cJSON_AddStringToObject(cfg, "name", name) == NULL) { cJSON_Delete(cfg); return str; }

    if (cJSON_AddStringToObject(cfg, "ap1s", (const char*)apCfg[0].SSID) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ap1p", (const char*)apCfg[0].Pass) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ap2s", (const char*)apCfg[1].SSID) == NULL) { cJSON_Delete(cfg); return str; }
    if (cJSON_AddStringToObject(cfg, "ap2p", (const char*)apCfg[1].Pass) == NULL) { cJSON_Delete(cfg); return str; }

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

    ESP_LOGI(TAG, "SetStringFromJSON %s = \"%s\"", id, str);

    return true;
}

bool Configuration::SetFromJSONString(char *jsonStr)
{
    InitData();

    cJSON *cfg = cJSON_Parse(jsonStr);
    if (cfg == NULL) {
        const char *errStr = cJSON_GetErrorPtr();
        if (errStr != NULL) {
            ESP_LOGE(TAG, "JSON error before: %s", errStr);
        }
        return false;
    }

    cJSON *item = NULL;

    // write the version
    item = cJSON_GetObjectItemCaseSensitive(cfg, "version");
    if (item == NULL) {
        cJSON_Delete(cfg);
        return false;
    }
    if (cJSON_IsNumber(item)) {
        version = item->valueint;
        if (version != ConfigurationVersion) {
            version = ConfigurationVersion;
            cJSON_Delete(cfg);
            return false;
        }
    }
    else {
        cJSON_Delete(cfg);
        return false;
    }

    bool res = true;
    res = res & SetStringFromJSON(name, NameBufLen, "name", cfg);
    res = res & SetStringFromJSON((char*)apCfg[0].SSID, SSIDBufLen, "ap1s", cfg);
    res = res & SetStringFromJSON((char*)apCfg[0].Pass, PassBufLen, "ap1p", cfg);
    res = res & SetStringFromJSON((char*)apCfg[1].SSID, SSIDBufLen, "ap2s", cfg);
    res = res & SetStringFromJSON((char*)apCfg[1].Pass, PassBufLen, "ap2p", cfg);
    res = res & SetStringFromJSON(ipAddr, ipv4BufLen, "ipAddr", cfg);
    res = res & SetStringFromJSON(ipMask, ipv4BufLen, "ipMask", cfg);
    res = res & SetStringFromJSON(ipGateway, ipv4BufLen, "ipGateway", cfg);
    res = res & SetStringFromJSON(ipDNS, ipv4BufLen, "ipDNS", cfg);

    res = res & SetFromJSONString_CustomData(cfg);

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
        ESP_LOGW(TAG, "%d nvs_open. Creating the namespace and default config.", err);
        nvs_close(nvsHandle);
        return WriteToNVS(false);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%d nvs_open NVS_READONLY", err);
        nvs_close(nvsHandle);
        return err;
    }

    size_t strBufLen = 0;
    err = nvs_get_str(nvsHandle, "json", NULL, &strBufLen);
    if (strBufLen == 0) {
        ESP_LOGE(TAG, "%d nvs_get_str required length", err);
        nvs_close(nvsHandle);
        return err;
    }
    char *str = (char*)malloc(strBufLen);
    if (str == NULL) {
        ESP_LOGE(TAG, "malloc str");
        nvs_close(nvsHandle);
        return err;
    }
    err = nvs_get_str(nvsHandle, "json", str, &strBufLen);
    if (err != ESP_OK) {
        free(str);
        ESP_LOGE(TAG, "%d nvs_get_str", err);
        nvs_close(nvsHandle);
        return err;
    }

    if (!SetFromJSONString(str)) {
        free(str);
        ESP_LOGE(TAG, "SetFromJSONString failed");
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }

    free(str);

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
        ESP_LOGE(TAG, "%d Failed to open NVS in readwrite mode", err);
        return err;
    }

    if (eraseAll) {
        err = nvs_erase_all(nvsHandle);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            ESP_LOGE(TAG, "%d Failed to erase", err);
            return err;
        }
        err = nvs_commit(nvsHandle);
        if (err != ESP_OK) {
            nvs_close(nvsHandle);
            ESP_LOGE(TAG, "%d Failed to commit", err);
            return err;
        }
    }

    char *str = CreateJSONConfigString(false);
    if (str == nullptr) {
        nvs_close(nvsHandle);
        return ESP_FAIL;
    }

    err = nvs_set_str(nvsHandle, "json", str);
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
