/**
 ********************************************************************************
 * @file    app_config.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Source file of the application configuration.
 ********************************************************************************
 */

#include "app_config.h"
#include "tpms_core.h"
#include <esp_log.h>
#include <memory.h>

// Protocol:        Chery Tiggo 7 | 07/2018-12/2022 (433MHz 802000021AA)

/**
 * HW:              8306
 * SW:              V6.52
 * AC/PI:           FF00/02B2
 * PV/PT/MI:        3.01/26.0/B1591
 */
#define MX_SENSOR_PSN1615_ID 0x9A846232

/**
 * HW:              8306
 * SW:              V6.52
 * AC/PI:           FF00/02B2
 * PV/PT/MI:        3.01/25.0/B1591
 */
#define MX_SENSOR_PSN1597_ID 0x5A895736

/**
 * HW:              8306
 * SW:              V6.52
 * AC/PI:           FF00/02B2
 * PV/PT/MI:        3.03/25.0/B1591
 */
#define MX_SENSOR_PSN1986_ID 0x68D57BAE

/**
 * HW:              8306
 * SW:              V6.52
 * AC/PI:           FF00/02B2
 * PV/PT/MI:        3.04/26.0/B1591
 */
#define MX_SENSOR_PSN1813_ID 0xC8A65DF7

static const char *TAG = "app_cfg";

esp_err_t app_config_restore(app_config_t *cfg_out) {
  if (cfg_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // Wi-Fi
  memcpy(cfg_out->wifi_config.ssid, CONFIG_SRVC_WIFI_SOFTAP_DEF_SSID,
         strlen(CONFIG_SRVC_WIFI_SOFTAP_DEF_SSID));
  memcpy(cfg_out->wifi_config.password, CONFIG_SRVC_WIFI_SOFTAP_DEF_PW,
         strlen(CONFIG_SRVC_WIFI_SOFTAP_DEF_PW));

  // Sensor IDs
  cfg_out->tpms_config.sensor_ids[SENSOR_TIRE_FRONT_LEFT] =
      MX_SENSOR_PSN1615_ID;
  cfg_out->tpms_config.valid_data_time_us = 1000 * 1000 * 20; // sec

  // Pressure
  cfg_out->tpms_config.pressure_kpa_normal_front = 180;
  cfg_out->tpms_config.pressure_kpa_normal_rear = 150;
  cfg_out->tpms_config.pressure_kpa_caution_dev = 20;
  cfg_out->tpms_config.pressure_kpa_critical_dev = 60;

  // Temperature
  cfg_out->tpms_config.temperature_c_caution_thr = 40;
  cfg_out->tpms_config.temperature_c_critical_thr = 50;

  ESP_LOGI(TAG, "Restored app config");
  return ESP_OK;
}

esp_err_t app_config_store(const app_config_t *cfg) {
  if (cfg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Stored app config");
  return ESP_OK;
}
