/**
 ********************************************************************************
 * @file    uhf_receiver_srvc.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Source file of the UHF receiver service.
 ********************************************************************************
 */

#include "uhf_receiver_srvc.h"
#include "autel_mx_sensor.h"
#include "cc1101.h"
#include "manchester_codec.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#define SPI_HOST SPI2_HOST
#define SPI_MOSI 11
#define SPI_CLK 12
#define SPI_MISO 13

#define CC1101_GDO0_1 9
#define CC1101_CS_1 10
#define CC1101_GDO0_2 17
#define CC1101_CS_2 18
#define CC1101_PACKET_LENGTH (AUTEL_MX_SENSOR_PACKET_LENGTH * 2)
#define COUNTOF(arr) ((sizeof(arr)) / sizeof(*arr))

static const char *TAG = "uhf_srv";

static cc1101_handle_t s_cc1101_handle;

static const cc1101_register_init_t s_cc1101_regs[] = {
    {CC1101_ADDR_IOCFG2, 0x2E}, // GDO2 to High impedance (3-state)
    // GDO0 to Asserts when sync word has been sent/received, and
    // de-asserts at the end of the packet. In RX, the pin will also
    // deassert when a packet is discarded due to address or maximum
    // length filtering or when the radio enters RXFIFO_OVERFLOW
    // state. In TX the pin will de-assert if the TX FIFO underflows.
    {CC1101_ADDR_IOCFG0, 0x46},
    {CC1101_ADDR_SYNC1, 0xA9},    // Sync Word High byte 0xA9
    {CC1101_ADDR_SYNC0, 0x55},    // Sync Word Low byte 0x55
    {CC1101_ADDR_PKTLEN, 0x12},   // Packet Length 18 bytes
    {CC1101_ADDR_PKTCTRL1, 0x64}, // Preamble quality estimator threshold 3
    {CC1101_ADDR_PKTCTRL0, 0x00}, // Whitening off, CRC off, Fixed packet length
    {CC1101_ADDR_FREQ2, 0x10},    // Carrier Freq 433.92 MHz
    {CC1101_ADDR_FREQ1, 0xB0},    // --/--
    {CC1101_ADDR_FREQ0, 0x71},    // --/--
    {CC1101_ADDR_MDMCFG4, 0xA9},  // Data Rate 19200 bps
    {CC1101_ADDR_MDMCFG3, 0x83},  // --/--
    {CC1101_ADDR_MDMCFG1, 0x02},  // Min preamble bytes number for TX - 2
    {CC1101_ADDR_DEVIATN, 0x24},  // Freq Dev 10 kHz
    {CC1101_ADDR_MCSM0, 0x14},    // Auto calibrate when IDLE -> RX/TX/FSTXON
};

static const uint8_t s_cc1101_patable[] = {0x84, 0x11};

static esp_err_t spi_config() {
  esp_err_t ret = ESP_OK;

  // Config SPI driver
  spi_bus_config_t spi_cfg = {
      .mosi_io_num = SPI_MOSI,
      .miso_io_num = SPI_MISO,
      .sclk_io_num = SPI_CLK,
      .data2_io_num = -1,
      .data3_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 256,
  };
  ret = spi_bus_initialize(SPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "SPI driver init fail");
  }

  return ret;
}

static esp_err_t cc1101_config() {
  cc1101_config_t cc1101_cfg = {
      .spi_host = SPI_HOST,
      .miso_io_num = SPI_MISO,
  };

  // Module
  cc1101_cfg.cs_io_num = CC1101_CS_1;
  cc1101_cfg.gdo0_io_num = CC1101_GDO0_1;
  esp_err_t ret = cc1101_init(&cc1101_cfg, &s_cc1101_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "CC1101 init fail (0x%02X): %s", ret, esp_err_to_name(ret));
    return ret;
  }

  // Write registers
  ret =
      cc1101_set_config(s_cc1101_handle, s_cc1101_regs, COUNTOF(s_cc1101_regs));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "CC1101 #1 set config fail");
    goto cleanup;
  }

  // Write PATABLE
  ret = cc1101_write(s_cc1101_handle, CC1101_ADDR_PATABLE, s_cc1101_patable, 1);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "CC1101 #1 PATABLE fail");
    goto cleanup;
  }

  ESP_LOGI(TAG, "Configured CC1101");
  return ret;

cleanup:

  ESP_ERROR_CHECK(cc1101_deinit(s_cc1101_handle));
  s_cc1101_handle = NULL;

  return ret;
}

static void process_packet(const uint8_t *data, float rssi_dbm, uint8_t lqi,
                           tpms_core_t *tpms_core) {

  // Get sensor data
  mxsensor_data_t mx_data = {0};
  esp_err_t ret = mxsensor_get_data(data, &mx_data);
  if (ret == ESP_ERR_INVALID_CRC) {
    ESP_LOGD(TAG, "Invalid CRC");
    return;
  } else if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mxsensor_get_data (%s)", esp_err_to_name(ret));
    return;
  }

  ESP_LOGI(TAG,
           "Received RSSI:%4.1fdBm LQI:%3i ID:%08lX F:%1X N:%1i P:%5.1fKPa "
           "T:%3i°C F:%02X",
           rssi_dbm, lqi, mx_data.sensor_id, mx_data.flags1,
           mx_data.packet_number, mx_data.pressure_kpa, mx_data.temperature_c,
           mx_data.flags2);

  // Update sensor data
  tpms_sensor_data_t sensor_data = {
      .pressure_kpa = mx_data.pressure_kpa,
      .rssi = rssi_dbm,
      .temperature_c = mx_data.temperature_c,
  };
  ret = tpms_core_update_sensor_data(mx_data.sensor_id, &sensor_data);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "Failed to update sensor %08lX (%s)", mx_data.sensor_id,
             esp_err_to_name(ret));
  }
}

static void src_proc(void *arg) {
  assert(arg != NULL);
  tpms_core_t *tpms_core = (tpms_core_t *)arg;

  ESP_LOGI(TAG, "Service started");

  // SPI Configure
  esp_err_t ret = spi_config();
  if (ret == ESP_OK) {
    // CC1101 Configure
    ret = cc1101_config();
    if (ret == ESP_OK) {
      // CC1101 Set RX mode
      ret = cc1101_set_rx_mode(s_cc1101_handle);
      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "set RX mode fail");
      }
    }
  }

  uint8_t raw_data[CC1101_PACKET_LENGTH] = {0};
  uint8_t raw_length = CC1101_PACKET_LENGTH;
  uint8_t decoded_data[CC1101_PACKET_LENGTH / 2] = {0};
  uint32_t decoded_bit_length = CC1101_PACKET_LENGTH / 2 * 8;
  float rssi_dbm;
  uint8_t lqi;

  while (true) {

    if (s_cc1101_handle != NULL) {
      // Read data if has
      ret = cc1101_read_packet(s_cc1101_handle, raw_data, CC1101_PACKET_LENGTH,
                               &rssi_dbm, &lqi);
      if (ret == ESP_OK) {
        ESP_LOGV(TAG, "Raw:");
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, raw_data, CC1101_PACKET_LENGTH,
                                 ESP_LOG_VERBOSE);

        int8_t mc_ret = manchester_decode(raw_data, raw_length * 8,
                                          decoded_data, &decoded_bit_length, 0);
        if (mc_ret == MANCHESTER_SUCCESS) {
          uint8_t decoded_length = (uint8_t)decoded_bit_length / 8;

          ESP_LOGD(TAG, "Decoded:");
          ESP_LOG_BUFFER_HEX_LEVEL(TAG, decoded_data, decoded_length,
                                   ESP_LOG_DEBUG);

          if (decoded_length == CC1101_PACKET_LENGTH / 2) {
            process_packet(decoded_data, rssi_dbm, lqi, tpms_core);
          } else {
            ESP_LOGE(TAG, "Decoded bytes %i, expected %i", decoded_length,
                     CC1101_PACKET_LENGTH / 2);
          }
        } else {
          ESP_LOGW(TAG, "Decode fail (%i)", mc_ret);
        }

        // Back to RX mode
        ret = cc1101_set_rx_mode(s_cc1101_handle);
        if (ret != ESP_OK) {
          ESP_LOGE(TAG, "CC1101 to RX mode fail");
        }
      }
    }

    vTaskDelay(SRVC_UHF_RCV_INTERVAL);
  }

  ESP_LOGI(TAG, "Service stopped");
  vTaskDelete(0);
}

esp_err_t uhf_receiver_start_srvc(tpms_core_t *tpms_core) {
  assert(tpms_core != NULL);

  ESP_LOGD(TAG, "UHF Receiver service starting...");

  BaseType_t rtos_ret =
      xTaskCreate(src_proc, "uhf_srv", SRVC_UHF_RCV_STACK_DEPTH, tpms_core,
                  SRVC_UHF_RCV_PRIORITY, NULL);
  if (rtos_ret != pdPASS) {
    ESP_LOGE(TAG, "Create service task fail (RTOS error: %i)", rtos_ret);
    return ESP_FAIL;
  }

  return ESP_OK;
}
