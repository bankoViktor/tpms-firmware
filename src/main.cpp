/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    10.11.2024
 * @brief   Entry point of the application.
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include <Arduino.h>

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

constexpr float FIRMWARE_VERSION = 0.1;
constexpr uint32_t UART_BAUDRATE = 115200;

constexpr uint8_t PWM_PIN = 32;
constexpr uint8_t PWM_CHANNEL = 0;
constexpr uint32_t PWM_FREQ = 125000;

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void setup()
{
  Serial.begin(UART_BAUDRATE);

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
  Serial.printf("Run\n");

  // Start PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, 8);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 128);
}

void loop()
{
}
