/**
 ********************************************************************************
 * @file    autel_mx_sensor.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Header file of the Autel MX sensor.
 ********************************************************************************
 * Manufacturer:    Autel
 * Model:           MX-Sensor
 * HW:              8306
 * SW:              V6.52
 * Protocol:        Chery Tiggo 4 / 06/2019 - 12/2023 (433MHz 802000121AA)
 * AC/PI:           FF00/03CA
 * PV/PT/MI:        3.02/26.0/B3626
 * Packet scheme:   IIIIIIII FN PP TT FF CC
 *   where:
 *   I  - sensor ID (32-bit)
 *   F  - Flags:
 *        * 0xA - pressure changed,
 *        * 0x9 - after programmed.
 *   N  - Packet number (1-3)
 *   PP - Pressure in KPa (KPa=p*1.37)
 *   TT - Temparature in C (C=t-50)
 *   FF - Flags:
 *        * 0x01 -
 *        * 0x02 -
 *   CC - CRC8 0-8 bytes (poly 0x07, init 0xF1)
 */

#ifndef AUTEL_MX_SENSOR__H
#define AUTEL_MX_SENSOR__H

#include <esp_err.h>

#define AUTEL_MX_SENSOR_PACKET_LENGTH 9

typedef struct mxsensor_data_t {
  uint32_t sensor_id;    // Sensor identifier.
  uint8_t flags1;        // Flags 1.
  uint8_t packet_number; // Packet number: 1..3.
  int8_t temperature_c;  // Temperature C.
  float pressure_kpa;    // Pressure kPa.
  uint8_t flags2;        // Flags 2.
  uint8_t crc8;          // CRC8.
} mxsensor_data_t;

/// @brief Extract data from MX sensor packet.
/// @param buffer Raw data buffer.
/// @param packet_out Output MX sensor data.
/// @return Status code.
esp_err_t mxsensor_get_data(const uint8_t *buffer, mxsensor_data_t *packet_out);

#endif
