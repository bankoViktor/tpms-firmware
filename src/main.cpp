/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    10.11.2024
 * @brief   Entry point of the application.
 ********************************************************************************
 */

#pragma region Includes

#include <Arduino.h>
#include <RadioLib.h>
#include <manchester_codec.h>
#include "sensor_packet_reader.h"

#pragma endregion

constexpr float FIRMWARE_VERSION = 0.1;

constexpr uint32_t UART_BAUDRATE = 115200;

constexpr size_t RADIOLIB_PACKET_LENGTH = 18;
constexpr uint8_t RADIOLIB_SYNC_WORD_HIGH = 0xA9;
constexpr uint8_t RADIOLIB_SYNC_WORD_LOW = 0x55;

// CC1101 has the following connections:
//                               +---+---+
//                           GND | 1 | 2 | VCC
//          D15 <----       GDO0 | 3 | 4 | CSN   ----> D5 (VSPI)
//   (VSPI) D18 <----        SCK | 5 | 6 | MOSI  ----> D23 (VSPI)
//   (VSPI) D19 <----  MISO/GDO1 | 7 | 8 | GDO2
//                               +---+---+
CC1101 g_radio = new Module(5, 15, RADIOLIB_NC);

// flag to indicate that a packet was received
volatile bool g_xReceivedFlag = false;

// RF message counter
uint32_t g_dwMsgCount = 0;

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
    Serial.printf("Run\n");

    Serial.println();

#pragma region Initializing
    Serial.print(F("[CC1101] Initializing ... "));
    int16_t wState = g_radio.begin(
        433.92, // Carrier frequency in MHz
        19.200, // Bit rate in kbps
        10,     // Frequency deviation from carrier frequency in kHz
        135.0,  // Receiver bandwidth in kHz
        5,      // Output power in dBm
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
    wState = g_radio.getChipVersion();
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
    wState = g_radio.setSyncWord(RADIOLIB_SYNC_WORD_HIGH, RADIOLIB_SYNC_WORD_LOW);
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
    wState = g_radio.setEncoding(RADIOLIB_ENCODING_NRZ);
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
    wState = g_radio.setCrcFiltering(false);
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
    Serial.print(RADIOLIB_PACKET_LENGTH);
    Serial.print(F(" bytes) ... "));
    wState = g_radio.fixedPacketLengthMode(RADIOLIB_PACKET_LENGTH);
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

    g_radio.setPacketReceivedAction(packetReceivedCallback);

#pragma region Start Receiving
    Serial.print(F("[CC1101] Starting to listen ... "));
    wState = g_radio.startReceive();
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
        Serial.print(F(" - "));
        size_t nNumBytes = g_radio.getPacketLength();

        // packet was successfully received
        // Serial.print(nNumBytes);
        // Serial.print(F("B, "));

        if (nNumBytes > 0)
        {
            if (nNumBytes > RADIOLIB_PACKET_LENGTH)
            {
                Serial.println(F("Buffer overflow"));
            }
            else
            {
                uint8_t abBuffer[RADIOLIB_PACKET_LENGTH] = {0};

#pragma region Read Data
                int16_t wState = g_radio.readData(abBuffer, nNumBytes);
                if (wState == RADIOLIB_ERR_NONE)
                {
                    // print RSSI (Received Signal Strength Indicator)
                    // of the last received packet
                    Serial.print(F("RSSI:"));
                    Serial.print(g_radio.getRSSI());
                    Serial.print(F("dBm"));

                    // print LQI (Link Quality Indicator)
                    // of the last received packet, lower is better
                    Serial.print(F(", LQI:"));
                    Serial.print(g_radio.getLQI());

                    // print data of the packet
                    // Serial.print(F(", Data: "));
                    // for (size_t i = 0; i < nNumBytes; i++)
                    // {
                    //     Serial.printf("%02X", abBuffer[i]);
                    // }

#pragma region Decode
                    Serial.print(F(", Decoded:"));
                    constexpr uint32_t DECODED_BUFF_LEN = RADIOLIB_PACKET_LENGTH * 8 / 4;
                    uint8_t abDecodedData[DECODED_BUFF_LEN] = {0};
                    size_t nDecodeDataLen = DECODED_BUFF_LEN * 8;
                    wState = manchester_decode(abBuffer, nNumBytes * 8, abDecodedData, &nDecodeDataLen, 0);
                    if (wState == MANCHESTER_SUCCESS)
                    {
                        nDecodeDataLen /= 8;

                        // Print decode buffer
                        for (size_t i = 0; i < nDecodeDataLen; i++)
                        {
                            Serial.printf("%02X", abDecodedData[i]);
                        }

                        SensorPacketReader reader(abDecodedData);
                        if (reader.checkCrc())
                        {
                            // Print Sensor ID
                            Serial.printf(", ID:%08X", reader.getSensorId());

                            // Print Flags
                            Serial.printf(", F:%X", reader.getFlags());
                            // 0x9, 0xD - after programmed
                            // 0xA - pressure changed
                            // 1001 - 9
                            // 1010 - A
                            // 1101 - D

                            // Print Packet Number
                            Serial.printf(", N:%i", reader.getPacketNumber());

                            // Print Pressure
                            Serial.printf(", P:%.1fKPa(%.1fbar)", reader.getPressureKPa(), reader.getPressureBar());

                            // Print Temperature
                            Serial.printf(", T:%.0f°C", reader.getTemperatureC());

                            // Print Unknown Byte
                            Serial.printf(", U:%02X", reader.getUnknownByte());

                            // Plot Pressure
                            Serial2.print(">p:");
                            Serial2.print(reader.getPressureBar());
                            Serial2.println();

                            // Plot Temperature
                            Serial2.print(">t:");
                            Serial2.print(reader.getTemperatureC());
                            Serial2.println();
                        }
                        else
                        {
                            Serial.print(", CRC error");
                        }

                        Serial.println();
                    }
                    else
                    {
                        // some other error occurred
                        Serial.print(F("failed, code "));
                        Serial.println(wState);
                    }
#pragma endregion
                }
                else if (wState == RADIOLIB_ERR_CRC_MISMATCH)
                {
                    // packet was received, but is malformed
                    Serial.println(F("CRC error!"));
                }
                else
                {
                    // some other error occurred
                    Serial.print(F("read failed, code "));
                    Serial.println(wState);
                }
#pragma endregion
            }
        }
        else
        {
            Serial.println("Empty");
        }

        // put module back to listen mode
        g_radio.startReceive();
    }
}
