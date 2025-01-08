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
#include <esp_log.h>

static const char *TAG = "uhf";

EventGroupHandle_t UhfReceiver::m_hEventGroup = nullptr;

UhfReceiver::UhfReceiver() : m_module(new Module(PIN_CS, PIN_IRQ, RADIOLIB_NC)),
                             m_hTask(NULL),
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
    switchToReceiving();

    while (true)
    {
        xEventGroupWaitBits(
            m_hEventGroup,
            EVENT_BIT_PACKET_RECEIVED,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        processReceivedPacket();

        switchToReceiving();
    }
}

void UhfReceiver::begin()
{
    int16_t wState = RADIOLIB_ERR_NONE;

    // Initialization
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
        ESP_LOGD(TAG, "Radio module begin");
    }
    else
    {
        ESP_LOGE(TAG, "Radio module begin fail (code &i)", wState);
        return;
    }

    // Set Sync Word
    wState = m_module.setSyncWord(SYNC_WORD_HIGH, SYNC_WORD_LOW);
    if (wState == RADIOLIB_ERR_NONE)
    {
        ESP_LOGD(TAG, "Set Sync word [0x%02X, 0x%02X]", SYNC_WORD_HIGH, SYNC_WORD_LOW);
    }
    else
    {
        ESP_LOGE(TAG, "Set Sync word fail [0x%02X, 0x%02X], (code %i)", SYNC_WORD_HIGH, SYNC_WORD_LOW, wState);
        return;
    }

    // Set NRZ Encode
    wState = m_module.setEncoding(RADIOLIB_ENCODING_NRZ);
    if (wState == RADIOLIB_ERR_NONE)
    {
        ESP_LOGD(TAG, "Set NRZ Encode");
    }
    else
    {
        ESP_LOGE(TAG, "Set NRZ Encode fail (code %i)", wState);
        return;
    }

    // Disable internal CRC
    wState = m_module.setCrcFiltering(false);
    if (wState == RADIOLIB_ERR_NONE)
    {
        ESP_LOGD(TAG, "Disable CRC filtering");
    }
    else
    {
        ESP_LOGE(TAG, "Disable CRC filtering fail (code %i)", wState);
        return;
    }

    // Set Fixed Packet Length Mode
    wState = m_module.fixedPacketLengthMode(PACKET_LENGTH);
    if (wState == RADIOLIB_ERR_NONE)
    {
        ESP_LOGD(TAG, "Set Fixed Packet Length");
    }
    else
    {
        ESP_LOGE(TAG, "Set Fixed Packet Length fail (code %i)", wState);
        return;
    }

    // Set Packet Received Callback
    m_module.setPacketReceivedAction(UhfReceiver::packetReceivedCallback);

    // Create Event Group
    m_hEventGroup = xEventGroupCreate();
    if (m_hEventGroup != NULL)
    {
        ESP_LOGD(TAG, "Created Event Group");
    }
    else
    {
        ESP_LOGE(TAG, "Create Event Group fail");
        return;
    }

    // TODO try attach iterrupt handler
    // gpio_install_isr_service(0);  // Устанавливаем ISR-сервис
    // gpio_isr_handler_add(INTERRUPT_PIN, interruptHandler, NULL);

    ESP_LOGI(TAG, "Initialize success");

    // Create Task
    String taskName = "UHF Receiver " + String((uint32_t)this, HEX);
    BaseType_t xRet = xTaskCreate(
        UhfReceiver::staticTaskCallback,
        taskName.c_str(),
        TASK_STACK_SIZE,
        this,
        TASK_PRIORITY,
        &m_hTask);
    if (xRet == pdPASS)
    {
        ESP_LOGD(TAG, "Task Created");
    }
    else
    {
        ESP_LOGE(TAG, "Task Create fail (code %i)", xRet);
        return;
    }
}

void IRAM_ATTR UhfReceiver::packetReceivedCallback()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(
        m_hEventGroup,
        EVENT_BIT_PACKET_RECEIVED,
        &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UhfReceiver::processReceivedPacket()
{
    // Increment message counter
    m_dwMsgCount++;

    int16_t wState = RADIOLIB_ERR_NONE;

    // Read data
    uint8_t abBuffer[PACKET_LENGTH] = {0};
    wState = m_module.readData(abBuffer, PACKET_LENGTH);
    if (wState == RADIOLIB_ERR_NONE)
    {

#if LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG
        {
            char szBuffer[256] = {0};
            size_t nPos = 0;
            for (size_t i = 0; i < PACKET_LENGTH; i++)
            {
                sprintf(szBuffer + nPos, "%02X ", abBuffer[i]);
                nPos += 3;
            }
            ESP_LOGD(TAG, "Received packet #%3i. Raw: %s", m_dwMsgCount, szBuffer);
        }
#endif

        uint8_t abDecodedData[DECODED_BUFF_LEN] = {0};
        size_t nDecodeDataLen = DECODED_BUFF_LEN * 8;
        wState = manchester_decode(abBuffer, PACKET_LENGTH * 8, abDecodedData, &nDecodeDataLen, 0);
        if (wState == MANCHESTER_SUCCESS)
        {
            nDecodeDataLen /= 8;

#if LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG
            {
                char szBuffer[256] = {0};
                size_t nPos = 0;
                for (size_t i = 0; i < nDecodeDataLen; i++)
                {
                    sprintf(szBuffer + nPos, "%02X ", abDecodedData[i]);
                    nPos += 3;
                }
                ESP_LOGD(TAG, "Received packet #%3i. Decoded: %s", m_dwMsgCount, szBuffer);
            }
#endif

            SensorPacketReader reader(abDecodedData);
            if (reader.checkCrc())
            {
                if (m_receivedCallback)
                {
                    // TODO pass ptr to sensor data structure
                    m_receivedCallback();
                }

                ESP_LOGI(TAG, "Received packet #%3i. RSSI:%4.1fdBm LQI:%3i ID:%08X F:%1X N:%1i P:%5.1fKPa T:%2.0f°C U:%02X",
                         m_dwMsgCount,
                         m_module.getRSSI(),
                         m_module.getLQI(),
                         reader.getSensorId(),
                         reader.getFlags(),
                         reader.getPacketNumber(),
                         reader.getPressureKPa(),
                         reader.getTemperatureC(),
                         reader.getUnknownByte());
            }
            else
            {
                ESP_LOGW(TAG, "Received packet #%3i. CRC fail. ", m_dwMsgCount);
            }
        }
        else
        {
            ESP_LOGW(TAG, "Received packet #%3i. Decode fail (code %i)", m_dwMsgCount, wState);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Received packet #%3i. Read data fail (code %i)", m_dwMsgCount, wState);
    }
}

void UhfReceiver::switchToReceiving()
{
    int16_t wState = m_module.startReceive();
    if (wState == RADIOLIB_ERR_NONE)
    {
        ESP_LOGV(TAG, "Start Receiving");
    }
    else
    {
        ESP_LOGE(TAG, "Start Receiving fail (code %i)", wState);
        return;
    }
}

void UhfReceiver::setReceivedCallback(received_callback_f func)
{
    m_receivedCallback = func;
}
