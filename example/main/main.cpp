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
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ExampleBoard.h"

const char* TAG = "main.cpp";

ExampleBoard board;
bool stationMode;

static TaskHandle_t xHTTPHandlerTask = NULL;
static TaskHandle_t xLoopTask = NULL;

HTTPCommand cmd;

extern "C" {

    static void LoopTask(void *taskParameter) {
        for(;;) {
            if (cmd.command != 0) {
                ESP_LOGI(TAG, "Received command %d with data 0x%x", cmd.command, cmd.data);
                cmd.command = 0;
                cmd.data = 0;
            }

            if (stationMode) {
                if (!board.IsConnectedToAP()) {
                    // the board has lost the WiFi connectivity
                    board.StopTheServers();
                    if (board.RestartStationMode(3) == ESP_OK) {
                        board.StartTheServers();
                        board.ConfigureMDNS();
                    }
                    else {
                        // failed to connect to WiFi, wait two minutes
                        vTaskDelay(120000 / portTICK_PERIOD_MS);
                    }
                }
            }

            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        // the next lines are here only for "completion"
        vTaskDelete(NULL);
    }

    static void HTTPTask(void *taskParameter) {
        HTTPCommand httpCmd;

        QueueHandle_t serverQueue;
        serverQueue = board.GetHttpServerQueue();

        for(;;) {
            if (xQueueReceive(serverQueue, &httpCmd, portMAX_DELAY) == pdPASS) {
                cmd.command = httpCmd.command;
                cmd.data = httpCmd.data;

                if (cmd.command == 0xFE) {
                    vTaskDelay (2000 / portTICK_PERIOD_MS);
                    esp_restart();
                }
            }
        }

        // the next lines are here only for "completion"
        vTaskDelete(NULL);
    }

    void app_main()
    {
        board.CheckApplicationImage();

        esp_err_t err = board.Initialize();
        if (err != ESP_OK) {
            uint8_t initFailSeverity = board.InitFailSeverity();

            board.PowerPeripherals(false);

            ESP_LOGE(TAG, "Initialization failed with severity level %d !", initFailSeverity);
            if (initFailSeverity == 5) {
                // critical system error
                board.EnterDeepSleep(60);
            }

            // retry in a few minutes, maybe it will recover ?
            board.Restart(120 + (esp_random() & 0x3F));
        }
        ESP_LOGI(TAG, "Board initialized OK");

        stationMode = board.IsConnectedToAP();

        xTaskCreate(HTTPTask, "HTTP command handling task", 2048, NULL, uxTaskPriorityGet(NULL) + 1, &xHTTPHandlerTask);
        if (xHTTPHandlerTask != NULL) {
            ESP_LOGI(TAG, "HTTP command handling task created.");
        }
        else {
            ESP_LOGE(TAG, "Failed to create the HTTP command handling task !");
            board.PowerPeripherals(false);
            board.EnterDeepSleep(10);
        }

        xTaskCreate(LoopTask, "Loop task", 4096, NULL, uxTaskPriorityGet(NULL) + 1, &xLoopTask);
        if (xLoopTask != NULL) {
            ESP_LOGI(TAG, "Loop task created.");
        }
        else {
            ESP_LOGE(TAG, "Failed to create the Loop task !");
            board.PowerPeripherals(false);
            board.EnterDeepSleep(10);
        }
    }
}
