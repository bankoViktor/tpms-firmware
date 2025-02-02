/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    10.11.2024
 * @brief   Entry point of the application.
 ********************************************************************************
 * ESP32-S3-DevKitC-1: (ESP32-S3-WROOM-1-N16R8)
 *
 * https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html
 * https://randomnerdtutorials.com/esp32-s3-devkitc-pinout-guide/
 *
 *                                 +----------+
 *                                 | UUUUUUUU |
 *                              +--+----------+--+
 *                            x | 3V3        GND | x
 *                            x | 3V3         43 | U0TXD |----> ESP32-Prog
 *         ESP32-Prog <---- RST | RST         44 | U0RXD |
 *                              | 4            1 |
 *                              | 5            2 |
 *                              | 6           42 | JTAG_TMS |
 *                              | 7           41 | JTAG_DTI |----> ESP32-Prog
 *                              | 15          40 | JTAG_DTO |
 *                              | 16          39 | JTAG_TCK |
 *         CC1101 <----|   GDO0 | 17          38 | RGB LED (built-in RGB led)
 *           # 2       | SPI_CS | 18          37 |
 *                              | 8           36 |
 *                              | 3           35 |
 *                              | 46           0 | BOOT ----> ESP32-Prog
 *         CC1101 <----|   GDO0 | 9           45 |
 *           # 1       | SPI_CS | 10          48 |
 *                   | SPI_MOSI | 11          47 |
 *       CC1101 <----|  SPI_CLK | 12          21 |
 *      #1 & #2      | SPI_MISO | 13          20 |
 *                              | 14          19 |
 *                            x | 5V         GND | x
 *                            x | GND        GND | x
 *                              +-+----+--+----+-+
 *                                +----+  +----+
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
constexpr const char *TAG = "app";

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
