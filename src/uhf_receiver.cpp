/**
 ********************************************************************************
 * @file    uhf_receiver.cpp
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    07.01.2025
 * @brief   Source file of the UHF receiver.
 ********************************************************************************
 */

#include "uhf_receiver.h"
#include <manchester_codec.h>
#include "sensor_packet_reader.h"
#include <RTOS.h>

volatile bool UhfReceiver::s_xReceivedFlag = false;

UhfReceiver::UhfReceiver() : m_module(new Module(PIN_CS, PIN_IRQ, RADIOLIB_NC)),
                             m_xTaskHandle(NULL),
                             m_dwMsgCount(0)
{
}

void UhfReceiver::staticTaskCallback(void *pvData)
{
    UhfReceiver *pInst = static_cast<UhfReceiver *>(pvData);
    pInst->taskCallback();
}

void UhfReceiver::taskCallback()
{
    while (true)
    {
        if (s_xReceivedFlag)
        {
            s_xReceivedFlag = false;

            processReceivedPacket();
        }
    }
}

void UhfReceiver::begin()
{
    int16_t wState = RADIOLIB_ERR_NONE;

#pragma region Init
    Serial.print(F("[CC1101] Initializing ... "));
    wState = m_module.begin(
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

#pragma region Sync word
    Serial.print(F("[CC1101] Set Sync word "));
    Serial.printf("(0x%02X, 0x%02X)", SYNC_WORD_HIGH, SYNC_WORD_LOW);
    Serial.print(F(" ... "));
    wState = m_module.setSyncWord(SYNC_WORD_HIGH, SYNC_WORD_LOW);
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
    Serial.print(F("[CC1101] Set NRZ Encode ... "));
    wState = m_module.setEncoding(RADIOLIB_ENCODING_NRZ);
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

#pragma region CRC
    Serial.print(F("[CC1101] Disable CRC filtering ... "));
    wState = m_module.setCrcFiltering(false);
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

#pragma region Fixed Pkt Len
    Serial.print(F("[CC1101] Fixed Packet Length Mode ("));
    Serial.print(PACKET_LENGTH);
    Serial.print(F(" bytes) ... "));
    wState = m_module.fixedPacketLengthMode(PACKET_LENGTH);
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

    // Receive Callback
    m_module.setPacketReceivedAction(UhfReceiver::packetReceivedCallback);

#pragma region Start Task
    String taskName = "UHF Receiver " + String((uint32_t)this, HEX);
    BaseType_t xRet = xTaskCreate(
        UhfReceiver::staticTaskCallback,
        taskName.c_str(),
        TASK_STACK_SIZE,
        this,
        TASK_PRIORITY,
        &m_xTaskHandle);
    if (xRet != pdPASS)
    {
        Serial.print(F("[CC1101] xTaskCreate failed"));
        while (true)
        {
            delay(10);
        }
    }
#pragma endregion

#pragma region Start Receiving
    Serial.print(F("[CC1101] Starting to listen ... "));
    wState = m_module.startReceive();
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

    s_xReceivedFlag = false;
}

void IRAM_ATTR UhfReceiver::packetReceivedCallback()
{
    s_xReceivedFlag = true;
}

void UhfReceiver::processReceivedPacket()
{
    // Increment message counter
    m_dwMsgCount++;

    int16_t wState = RADIOLIB_ERR_NONE;

    // Read data
    Serial.printf("[CC1101] Received #%3i - ", m_dwMsgCount);
    uint8_t abBuffer[PACKET_LENGTH] = {0};
    wState = m_module.readData(abBuffer, PACKET_LENGTH);
    if (wState == RADIOLIB_ERR_NONE)
    {
        // Pint RSSI (Received Signal Strength Indicator)
        Serial.print(F("RSSI:"));
        Serial.print(m_module.getRSSI());
        Serial.print(F("dBm"));

        // Print LQI (Link Quality Indicator)
        Serial.print(F(", LQI:"));
        Serial.print(m_module.getLQI());

        // Print data of the packet
        // Serial.print(F(", Data: "));
        // for (size_t i = 0; i < nNumBytes; i++)
        // {
        //     Serial.printf("%02X", abBuffer[i]);
        // }

        Serial.print(F(", Decoded:"));
        constexpr uint32_t DECODED_BUFF_LEN = PACKET_LENGTH * 8 / 4;
        uint8_t abDecodedData[DECODED_BUFF_LEN] = {0};
        size_t nDecodeDataLen = DECODED_BUFF_LEN * 8;
        wState = manchester_decode(abBuffer, PACKET_LENGTH * 8, abDecodedData, &nDecodeDataLen, 0);
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
                // if (m_receivedCallback)
                // {
                //     // TODO pass ptr to sensor data structure
                //     m_receivedCallback();
                // }

                // Print Sensor ID
                Serial.printf(", ID:%08X", reader.getSensorId());

                // Print Flags
                Serial.printf(", F:%1X", reader.getFlags());

                // Print Packet Number
                Serial.printf(", N:%1i", reader.getPacketNumber());

                // Print Pressure
                Serial.printf(", P:%5.1fKPa(%3.1fbar)", reader.getPressureKPa(), reader.getPressureBar());

                // Print Temperature
                Serial.printf(", T:%2.0f°C", reader.getTemperatureC());

                // Print Unknown Byte
                Serial.printf(", U:%02X", reader.getUnknownByte());

                // Plot Pressure
                // Serial2.print(">p:");
                // Serial2.print(reader.getPressureBar());
                // Serial2.println();

                // Plot Temperature
                // Serial2.print(">t:");
                // Serial2.print(reader.getTemperatureC());
                // Serial2.println();
            }
            else
            {
                Serial.print(", CRC error");
            }
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.print(wState);
        }
    }
    else
    {
        Serial.print(F("read failed, code "));
        Serial.print(wState);
    }
    Serial.println();

    // Put module back to listen mode
    wState = m_module.startReceive();
    if (wState != RADIOLIB_ERR_NONE)
    {
        Serial.print(F("[CC1101] Re-start receive failed, code "));
        Serial.println(wState);
        while (true)
        {
            delay(10);
        }
    }
}

void UhfReceiver::setReceivedCallback(received_callback_f func)
{
    m_receivedCallback = func;
}
