/**
 ********************************************************************************
 * @file    autel_mx_sensor.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Source file of the Autel MX sensor.
 ********************************************************************************
 */

#include "autel_mx_sensor.h"
#include "crc8.h"

#define PRESSURE_KPA_FACTOR 1.37
#define TEMPERATURE_C_OFFSET -50
#define CRC8_POLY 0x07
#define CRC8_INIT 0xF1
#define CRC8_XOR_OUT 0x00

static uint32_t get_sensor_id(const uint8_t *buffer) {
  uint32_t result = 0;
  result |= buffer[0] << 24;
  result |= buffer[1] << 16;
  result |= buffer[2] << 8;
  result |= buffer[3];
  return result;
}

static float get_pressure_kpa(const uint8_t *buffer) {
  return (float)buffer[5] * PRESSURE_KPA_FACTOR;
}

static int8_t get_temperature_c(const uint8_t *buffer) {
  return (int8_t)buffer[6] + TEMPERATURE_C_OFFSET;
}

esp_err_t mxsensor_get_data(const uint8_t *buffer,
                            mxsensor_data_t *packet_out) {
  if (buffer == NULL || packet_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // Match with calculated CRC8
  uint8_t crc8 = buffer[AUTEL_MX_SENSOR_PACKET_LENGTH - 1];
  uint8_t calculated_crc8 =
      crc8_calculate(buffer, AUTEL_MX_SENSOR_PACKET_LENGTH - 1, CRC8_POLY,
                     CRC8_INIT, CRC8_XOR_OUT);
  if (calculated_crc8 != crc8) {
    return ESP_ERR_INVALID_CRC;
  }

  // Extract and fill data
  packet_out->sensor_id = get_sensor_id(buffer);
  packet_out->flags1 = (buffer[4] & 0xF0) >> 4;
  packet_out->packet_number = buffer[4] & 0x0F;
  packet_out->pressure_kpa = get_pressure_kpa(buffer);
  packet_out->temperature_c = get_temperature_c(buffer);
  packet_out->flags2 = buffer[7];
  packet_out->crc8 = buffer[8];
  return ESP_OK;
}
