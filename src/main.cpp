/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    10.11.2024
 * @brief   Entry point of the application.
 ********************************************************************************
 */

#include <Arduino.h>
#include "uhf_receiver.h"

#define MAKE_VERSION(bMajor, bMinor, bPatch) ((bMajor << 16) | (bMinor << 8) | (bPatch))
#define GET_VERSION_MAJOR(dwVersion) (((dwVersion) >> 16) & 0xFF)
#define GET_VERSION_MINOR(dwVersion) (((dwVersion) >> 8) & 0xFF)
#define GET_VERSION_PATCH(dwVersion) ((dwVersion) & 0xFF)

constexpr uint32_t FIRMWARE_VERSION = MAKE_VERSION(1, 0, 0);
constexpr const char *FIRMWARE_BUILD_DATE = __DATE__;
constexpr const char *REPO_URL = "https://github.com/bankoViktor";

constexpr uint32_t UART_BAUDRATE = 115200;
static const char *TAG = "app";

UhfReceiver g_uhfReceiver;

void setup()
{
    Serial.begin(UART_BAUDRATE);
    // Serial2.begin(UART_BAUDRATE, SERIAL_8N1, -1, 14);

    esp_log_level_set("*", ESP_LOG_INFO);
    //esp_log_level_set("uhf", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "Tire-pressure monitoring system");
    ESP_LOGI(TAG, "Firmware   : v%i.%i.%i",
             GET_VERSION_MAJOR(FIRMWARE_VERSION),
             GET_VERSION_MINOR(FIRMWARE_VERSION),
             GET_VERSION_PATCH(FIRMWARE_VERSION));
    ESP_LOGI(TAG, "Build date : %s", FIRMWARE_BUILD_DATE);
    ESP_LOGI(TAG, "Author     : Viktor Banko S. (bankviktor14@gmail.com)");
    ESP_LOGI(TAG, "URL        : %s", REPO_URL);
    ESP_LOGI(TAG, "Chip       : %s Rev.%i", ESP.getChipModel(), ESP.getChipRevision());
    ESP_LOGI(TAG, "Flash Size : %i KB", ESP.getFlashChipSize() / 1024);

    g_uhfReceiver.begin();
}

void loop()
{
}
