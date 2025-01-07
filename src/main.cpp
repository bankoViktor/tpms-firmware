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

constexpr float FIRMWARE_VERSION = 0.1;
constexpr uint32_t UART_BAUDRATE = 115200;

UhfReceiver g_uhfReceiver;

void setup()
{
    Serial.begin(UART_BAUDRATE);
    Serial2.begin(UART_BAUDRATE, SERIAL_8N1, -1, 14);

    Serial.printf("\n\n");
    Serial.printf("--------------------------------------------\n");
    Serial.printf("Tire-pressure monitoring system on ESP32\n");
    Serial.printf("Version %.1f\n", FIRMWARE_VERSION);
    Serial.printf("from Viktor Banko S. (bankviktor14@gmail.com)\n");
    Serial.printf("Thank you for your interest in my TPMS project\n");
    Serial.printf("See more to https://github.com/merbanan/rtl_433\n");
    Serial.printf("--------------------------------------------\n");
    Serial.printf("Chip Model  : %s Rev.%i (%i cores)\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores());
    Serial.printf("CPU Freq    : %i MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size  : %i KB\n", ESP.getFlashChipSize() / 1024);
    Serial.printf("--------------------------------------------\n");

    g_uhfReceiver.begin();

    Serial.printf("Run\n");

    Serial.println();
}

void loop()
{
    g_uhfReceiver.loopHandle();
}
