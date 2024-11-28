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
#include <RadioLib.h>
#include <manchester_codec.h>

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

constexpr float FIRMWARE_VERSION = 0.1;
constexpr uint32_t UART_BAUDRATE = 115200;
constexpr size_t RX_BUFFER_LENGTH = 1024;
constexpr size_t PACKET_LENGTH = 23;

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

// CC1101 has the following connections:
// CS pin:    5
// GDO0 pin:  15
// RST pin:   unused (optional)
// GDO2 pin:  unused (optional)
CC1101 radio = new Module(5, 15, RADIOLIB_NC);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// flag to indicate that a packet was received
volatile bool g_xReceivedFlag = false;

// RF message counter
uint32_t g_dwMsgCount = 0;

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

static void getSensorId(const uint8_t *pbaBuffer, uint32_t *pdwId)
{
  *pdwId = 0;
  *pdwId |= (pbaBuffer[0] & 0x0F) << 28;
  *pdwId |= pbaBuffer[1] << 20;
  *pdwId |= pbaBuffer[2] << 12;
  *pdwId |= pbaBuffer[3] << 4;
  *pdwId |= (pbaBuffer[4] & 0xF0) >> 4;
}

static void getPressureKPa(const uint8_t *pbaBuffer, float *pfPressure, uint8_t *pOut)
{
  uint8_t bValue;
  bValue |= (pbaBuffer[4] & 0x0F) << 4;
  bValue |= (pbaBuffer[5] & 0xF0) >> 4;
  *pOut = bValue;
  *pfPressure = ((float)bValue) * 3;
}

static float convert_KPa_to_Bar(float kPa_pressure)
{
  return kPa_pressure / 100;
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void packetReceivedCallback(void)
{
  // we got a packet, set the flag
  g_xReceivedFlag = true;
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
  Serial.printf("Run\n");

  Serial.println();

#pragma region Initializing
  Serial.print(F("[CC1101] Initializing ... "));
  int16_t wState = radio.begin(
      433.92, // Carrier frequency in MHz
      19.200, // Bit rate in kbps
      10,     // Frequency deviation from carrier frequency in kHz
      135.0,  // Receiver bandwidth in kHz
      10,     // Output power in dBm
      16      // Preamble Length in bits
  );
  if (wState == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

#pragma region Version
  Serial.print(F("[CC1101] Version: "));
  wState = radio.getChipVersion();
  if (wState >= RADIOLIB_ERR_NONE)
  {
    Serial.println(wState);
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

#pragma region Sync word
  Serial.print(F("[CC1101] Set Sync word ... "));
  wState = radio.setSyncWord(0x56, 0x65);
  if (wState == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

#pragma region NRZ Encode
  Serial.print(F("[CC1101] Set Encode ... "));
  wState = radio.setEncoding(RADIOLIB_ENCODING_NRZ);
  if (wState == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

#pragma region CRC disable
  Serial.print(F("[CC1101] Disable CRC filtering ... "));
  wState = radio.setCrcFiltering(false);
  if (wState == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

#pragma region Fixed Pkt Len Mode
  Serial.print(F("[CC1101] Fixed Packet Length Mode ("));
  Serial.print(PACKET_LENGTH);
  Serial.print(F(" bytes) ... "));
  wState = radio.fixedPacketLengthMode(PACKET_LENGTH);
  if (wState == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

  radio.setPacketReceivedAction(packetReceivedCallback);

#pragma region Start Receiving
  Serial.print(F("[CC1101] Starting to listen ... "));
  wState = radio.startReceive();
  if (wState == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(wState);
    while (true)
    {
      delay(10);
    }
  }
#pragma endregion

  // set the function that will be called
  // when new packet is received

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.readData();

  g_xReceivedFlag = false;
}

void loop()
{
  // check if the flag is set
  if (g_xReceivedFlag)
  {
    // reset flag
    g_xReceivedFlag = false;

    g_dwMsgCount++;

    // you can read received data as an Arduino String
    // String str;
    // int state = radio.readData(str);

    // you can also read received data as byte array
    Serial.print(F("[CC1101] Received #"));
    Serial.print(g_dwMsgCount);
    Serial.print(F(" ... "));
    size_t nNumBytes = radio.getPacketLength();

    // packet was successfully received
    Serial.print(nNumBytes);
    Serial.print(F("B, "));

    if (nNumBytes > 0)
    {
      if (nNumBytes > RX_BUFFER_LENGTH)
      {
        Serial.println(F("Buffer overflow"));
      }
      else
      {
        uint8_t abBuffer[RX_BUFFER_LENGTH] = {0};

        int16_t wState = radio.readData(abBuffer, nNumBytes);
        if (wState == RADIOLIB_ERR_NONE)
        {
          // print RSSI (Received Signal Strength Indicator)
          // of the last received packet
          // Serial.print(F("RSSI:"));
          // Serial.print(radio.getRSSI());
          // Serial.print(F("dBm"));

          // print LQI (Link Quality Indicator)
          // of the last received packet, lower is better
          // Serial.print(F(", LQI:"));
          // Serial.print(radio.getLQI());

          // print data of the packet
          // Serial.println(F("Data:"));
          // for (size_t i = 0; i < nNumBytes; i++)
          // {
          //     Serial.print(abBuffer[i], HEX);
          // }
          // Serial.println();

          uint8_t abDecodedData[32] = {0};
          size_t nDecodeDataLen = 0;

          // Serial.print(F("Data ... "));
          wState = manchester_decode(abBuffer, nNumBytes * 8, abDecodedData, &nDecodeDataLen, 0);
          if (wState == MANCHESTER_SUCCESS)
          {
            if (nDecodeDataLen % 8 != 0)
            {
              nDecodeDataLen += 4;
            }
            nDecodeDataLen /= 8;

            // Pring decode buffer
            // for (size_t i = 0; i < nDecodeDataLen; i++)
            // {
            //     Serial.print(abDecodedData[i], HEX);
            // }
            // Serial.println();

            // ID
            uint32_t dwSensorId;
            getSensorId(abDecodedData, &dwSensorId);
            Serial.print(F("ID: "));
            Serial.print(dwSensorId, HEX);

            // Pressure
            float fPressure_KPa;
            uint8_t fPressure;
            getPressureKPa(abDecodedData, &fPressure_KPa, &fPressure);
            Serial.print(F(", "));
            // Serial.print(fPressure_KPa);
            // Serial.print(F("KPa "));
            Serial.print(convert_KPa_to_Bar(fPressure_KPa));
            Serial.print(F(" Bar"));
            // Serial.print(F("(0x"));
            // Serial.print(fPressure, HEX);
            // Serial.print(F(")"));

            Serial.println();
          }
          else
          {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(wState);
          }
        }
        else if (wState == RADIOLIB_ERR_CRC_MISMATCH)
        {
          // packet was received, but is malformed
          Serial.println(F("CRC error!"));
        }
        else
        {
          // some other error occurred
          Serial.print(F("failed, code "));
          Serial.println(wState);
        }
      }
    }
    else
    {
      Serial.println("Empty");
    }

    // put module back to listen mode
    radio.startReceive();
  }
}
