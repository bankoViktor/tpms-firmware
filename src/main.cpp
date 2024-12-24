/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    10.11.2024
 * @brief   Entry point of the application.
 ********************************************************************************
 */

#include <Arduino.h>
#include "tpms.h"

constexpr float FIRMWARE_VERSION = 0.1;
constexpr uint32_t UART_BAUDRATE = 115200;

Tpms g_tpms;

void setup()
{
    Serial.begin(UART_BAUDRATE);

    esp_log_level_set("TireSensor", ESP_LOG_DEBUG);
    esp_log_level_set("RandPusher", ESP_LOG_DEBUG);
    esp_log_level_set("CanPuller", ESP_LOG_DEBUG);
    esp_log_level_set("UhfTransiver", ESP_LOG_DEBUG);
    esp_log_level_set("Config", ESP_LOG_DEBUG);
    esp_log_level_set("TPMS", ESP_LOG_DEBUG);

    g_tpms.init();

    ESP_LOGI("App", "Setup complete");
}

void loop()
{
}
