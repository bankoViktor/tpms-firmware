/**
 ********************************************************************************
 * @file    tpms_sensor.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    09.01.2025
 * @brief   Header file of the TPMS Sensor.
 ********************************************************************************
 */

#ifndef TPMS_SENSOR__H
#define TPMS_SENSOR__H

#include <stdint.h>

/// @brief Data of the TPMS sensor.
typedef struct tpms_sensor_data_t {
  float pressure_kpa;   // Pressure in KPa.
  float rssi;           // Received signal strength indicator.
  int8_t temperature_c; // Temperature in C.
} tpms_sensor_data_t;

/// @brief Sensor identifier of the TPMS sensor.
typedef uint32_t tpms_sensor_id_t;

/// @brief Flags of a sensor.
typedef enum tpms_sensor_flag_t {
  SENSOR_FLAG_VALID_DATA = (1 << 0),     // Valid of the sensor data.
  SENSOR_FLAG_CAUTION_ALARM = (1 << 1),  // Caution Alarm.
  SENSOR_FLAG_CRITICAL_ALARM = (1 << 2), // Critical Alarm.
} tpms_sensor_flag_t;

/// @brief TPMS sensor.
typedef struct tpms_sensor_t {
  tpms_sensor_id_t id;      // Sensor identifier.
  tpms_sensor_data_t data;  // Data of the sensor.
  tpms_sensor_flag_t flags; // Flags of the sensor.
} tpms_sensor_t;

/// @brief Index of the TPMS Sensors.
typedef enum tpms_sensor_num_t {
  SENSOR_TIRE_FRONT_LEFT,
  SENSOR_TIRE_FRONT_RIGHT,
  SENSOR_TIRE_REAR_LEFT,
  SENSOR_TIRE_REAR_RIGHT,
  SENSOR_TIRE_MAX,
} tpms_sensor_num_t;

#endif
