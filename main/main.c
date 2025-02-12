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
 *                      | CANTX | 4            1 |
 *     SN65HVD230 <---- | CANRX | 5            2 |
 *                              | 6           42 | JTAG_TMS |
 *                              | 7           41 | JTAG_DTI |----> ESP32-Prog
 *                              | 15          40 | JTAG_DTO |
 *                              | 16          39 | JTAG_TCK |
 *       * CC1101 <----|   GDO0 | 17          38 | RGB LED (built-in RGB led)
 *       *   # 2       | SPI_CS | 18          37 |
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
 *                            x | 5V           6 | x   CC1101 #1 <----|   GDO0*
 *                              +-----+----+-----+
 *                                    +----+
 */

#include <stdio.h>

void app_main() {}
