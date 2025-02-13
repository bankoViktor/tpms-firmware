/**
 ********************************************************************************
 * @file    app_config.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Header file of the application configuration.
 ********************************************************************************
 */

#ifndef APP_CONFIG__H
#define APP_CONFIG__H

#include "tpms_sensor.h"
#include <esp_err.h>
#include <stdint.h>

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PW_MAX_LEN 64

/// @brief Wi-Fi configuration.
typedef struct app_wifi_config_t {
  char ssid[WIFI_SSID_MAX_LEN];   // Wi-Fi Service Set Identifier.
  char password[WIFI_PW_MAX_LEN]; // Wi-Fi Password.
} app_wifi_config_t;

/// @brief TPMS configuration.
typedef struct app_tpms_config_t {
  tpms_sensor_id_t sensor_ids[SENSOR_TIRE_MAX]; // Sensor identifiers.
  float pressure_kpa_normal;                    // Pressure (KPa) normal.
  float pressure_kpa_caution_dev;     // Pressure (KPa) warn alarm deviation.
  float pressure_kpa_critical_dev;    // Pressure (KPa) err alarm deviation.
  uint8_t temperature_c_caution_thr;  // Temp. (C) Caution alarm threshold.
  uint8_t temperature_c_critical_thr; // Temp. (C) Critical alarm threshold.
  uint64_t valid_data_time_us;        // Time (msec) for reset valid flag.
} app_tpms_config_t;

/// @brief Application configuration.
typedef struct app_config_t {
  app_wifi_config_t wifi_config;
  app_tpms_config_t tpms_config;
} app_config_t;

/// @brief Restore configuration of the application.
/// @param cfg_out Output configuration.
/// @return Status code.
esp_err_t app_config_restore(app_config_t *cfg_out);

/// @brief Store configuration of the application.
/// @param cfg Configuration for store.
/// @return Status code.
esp_err_t app_config_store(const app_config_t *cfg);

#endif
