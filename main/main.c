/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.02.2025
 * @brief   Source file of the application.
 ********************************************************************************
 * Espressif ESP32-S3-DevKitC-1 (ESP32-S3-WROOM-1-N16R8)
 * https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/index.html
 *
 *                                 +----------+
 *                                 | UUUUUUUU |
 *                              +--+----------+--+
 *                            x | 3V3        GND | x
 *                            x | 3V3         43 | U0TXD |----> ESP32-Prog
 *         ESP32-Prog <---- RST | EN          44 | U0RXD |
 *                              | 4            1 |
 *                              | 5            2 |
 *                              | 6           42 | JTAG_TMS |
 *                              | 7           41 | JTAG_DTI |----> ESP32-Prog
 *                              | 15          40 | JTAG_DTO |
 *                              | 16          39 | JTAG_TCK |
 *                      | CANTX | 17          38 | RGB LED (built-in RGB led)
 *     SN65HVD230 <---- | CANRX | 18          37 |
 *      WIFI PW SUPPRESS button | 8           36 |
 *                              | 3           35 |
 *                              | 46           0 | BOOT ----> ESP32-Prog
 *                   |     GDO0 | 9           45 |
 *                   |   SPI_CS | 10          48 |
 *       CC1101 <----| SPI_MOSI | 11          47 |
 *                   |  SPI_CLK | 12          21 |
 *                   | SPI_MISO | 13          20 |
 *                              | 14          19 |
 *                            x | 5V         GND | x
 *                            x | GND        GND | x
 *                              +-+----+--+----+-+
 *                                +----+  +----+
 *
 * Espressif ESP32-DevKitC V4 (ESP32-WROOM-32/ESP32-WROOM-32D)
 * https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/index.html
 *
 *                                 +----------+
 *                                 | UUUUUUUU |
 *                              +--+----------+--+
 *                            x | 3V3        GND | x
 *         ESP32-Prog <---- RST | EN          23 |
 *                              > 36          22 |
 *                              > 39           1 | U0TXD |----> ESP32-Prog
 *                              > 34           3 | U0RXD |
 *                              > 35          21 | CANTX    |
 *                              | 32         GND | x        |----> SN65HVD230
 *                              | 33          19 | CANRX    |
 *                              | 25          18 |
 *                              | 26           5 |
 *                              | 27          17 |
 *                   | JTAG_TMS | 14          16 | JTAG_TDO ----> ESP32-Prog
 *   ESP32-Prog <----| JTAG_TDI | 12           4 |
 *                   |        x | GND          0 | BOOT ----> ESP32-Prog
 *                   | JTAG_TCK | 13           2 |
 *                            x | 9           15 |
 *                            x | 10           8 | x
 *                            x | 11           7 | x
 *                            x | 5V           6 | x
 *                              +-----+----+-----+
 *                                    +----+
 */

// Run command 'ESP-IDF: Add vscode Configuration Folder' for fix IDE
// IntellSence errors.
#include "app_config.h"
#include "srvc_can_tx.h"
#include "srvc_uhf_rx.h"
#include "srvc_wifi_softap.h"
#include "tpms_core.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <nvs_flash.h>

static app_config_t s_app_config;

void app_main(void) {
  // Log configuration
  // esp_log_level_set("cc1101", ESP_LOG_INFO);
  // esp_log_level_set("uhf_srv", ESP_LOG_INFO);
  // esp_log_level_set("can_srv", ESP_LOG_INFO);
  // esp_log_level_set("tpms_core", ESP_LOG_INFO);
  // esp_log_level_set("app_cfg", ESP_LOG_DEBUG);
  // esp_log_level_set("wifi_ap", ESP_LOG_DEBUG);

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));

  // App configuration
  ESP_ERROR_CHECK(app_config_restore(&s_app_config));

  tpms_core_t *tpms_core;
  ESP_ERROR_CHECK(tpms_core_init(&s_app_config.tpms_config, &tpms_core));

  app_wifi_config_t *wifi_cfg = &s_app_config.wifi_config;

  // Services
  ESP_ERROR_CHECK(srvc_wifi_softap(wifi_cfg));
  ESP_ERROR_CHECK(srvc_uhf_rx(tpms_core));
  ESP_ERROR_CHECK(srvc_can_tx(tpms_core));

  vTaskSuspend(NULL);
}
