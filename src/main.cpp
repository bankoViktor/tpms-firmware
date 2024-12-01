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
#include <CAN.h>
#include <Ticker.h>
#include <CAN_CheryTiggo2.h>

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

constexpr float FIRMWARE_VERSION = 0.1;
constexpr uint32_t UART_BAUDRATE = 115200;

constexpr int CAN_RX_PIN = 4;
constexpr int CAN_TX_PIN = 5;

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

void tickerCallback();

static Ticker g_ticker(tickerCallback, 500, 0, MILLIS);

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

int CAN_send(int msgId, const uint8_t *buffer, size_t size)
{
  if (!CAN.beginPacket(msgId))
  {
    return 0;
  }

  if (!CAN.write(buffer, size))
  {
    return 0;
  }

  if (!CAN.endPacket())
  {
    return 0;
  }

  return 1;
}

void tickerCallback()
{
  can_msg_tpms_t msgTpms;

  msgTpms.setPressureFL(1.1);
  msgTpms.setPressureFR(1.2);
  msgTpms.setPressureRL(2.1);
  msgTpms.setPressureRR(2.2);

  if (!CAN_send(CAN_MSGID_TPMS, (uint8_t *)&msgTpms, 8))
  {
    Serial.println(F("[CAN] Send failed"));
  }
}

void canReceiveCallback(int nPacketSize)
{
  Serial.print("Received ");
  Serial.print(CAN.packetExtended() ? "EXT " : "STD ");

  if (CAN.packetRtr())
  {
    Serial.print("RTR ");
  }

  Serial.print("0x");
  Serial.print(CAN.packetId(), HEX);

  if (CAN.packetRtr())
  {
    Serial.print(" ");
    Serial.print(CAN.packetDlc());
    Serial.print(" bytes");
  }
  else
  {
    Serial.print(" ");
    Serial.print(nPacketSize);
    Serial.print(" bytes ");

    // only print packet data for non-RTR packets
    while (CAN.available())
    {
      Serial.printf("%02X ", (char)CAN.read());
    }
  }

  Serial.println();
}

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

  // CAN Init

  CAN.setPins(CAN_RX_PIN, CAN_TX_PIN);
  if (!CAN.begin(CAN2_BITRATE))
  {
    Serial.println("Starting CAN failed!");
    while (true)
    {
      delay(10);
    }
  }
  g_ticker.start();

  Serial.printf("Run\n");
}

void loop()
{
  g_ticker.update();

  int nPacketSize = CAN.available();
  if (nPacketSize)
  {
    canReceiveCallback(nPacketSize);
  }
}
