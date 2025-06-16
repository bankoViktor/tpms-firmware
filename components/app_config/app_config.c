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

  app_tpms_config_t *cfg = &cfg_out->tpms_config;

  // Wi-Fi
  // memcpy(cfg_out->wifi_config.ssid, CONFIG_SRVC_WIFI_SOFTAP_DEF_SSID,
  //        strlen(CONFIG_SRVC_WIFI_SOFTAP_DEF_SSID));
  // memcpy(cfg_out->wifi_config.password, CONFIG_SRVC_WIFI_SOFTAP_DEF_PW,
  //        strlen(CONFIG_SRVC_WIFI_SOFTAP_DEF_PW));

  // Sensor IDs
  tpms_sensor_id_t *sensors = cfg->sensor_ids;
  sensors[SENSOR_TIRE_FRONT_LEFT] = MX_SENSOR_PSN1986_ID;
  sensors[SENSOR_TIRE_FRONT_RIGHT] = MX_SENSOR_PSN1615_ID;
  sensors[SENSOR_TIRE_REAR_LEFT] = MX_SENSOR_PSN1597_ID;
  sensors[SENSOR_TIRE_REAR_RIGHT] = MX_SENSOR_PSN1813_ID;

  cfg->valid_data_time_us = 90 * 1E6; // sec

  // Pressure
  cfg->pressure_kpa_normal_front = 210;
  cfg->pressure_kpa_normal_rear = 220;
  cfg->pressure_kpa_caution_dev = 20;
  cfg->pressure_kpa_critical_dev = 50;

  // Temperature
  cfg->temperature_c_caution_thr = 0;
  cfg->temperature_c_critical_thr = 0;

  ESP_LOGI(TAG, "Preset. Presure (KPa): front %f, rear %f",
           cfg->pressure_kpa_normal_front, cfg->pressure_kpa_normal_rear);

  ESP_LOGI(TAG, "Preset. Presure deviation (KPa): caution %f, critical %f",
           cfg->pressure_kpa_caution_dev, cfg->pressure_kpa_critical_dev);

  ESP_LOGI(TAG, "Preset. Temperature thresholds (C): caution %i, critical %i",
           cfg->temperature_c_caution_thr, cfg->temperature_c_critical_thr);

  ESP_LOGI(TAG, "Preset. Value clear delay (sec): %lu",
           (uint32_t)(cfg->valid_data_time_us / 1E6));

  // ESP_LOGI(TAG, "Restored app config");
  return ESP_OK;
}

esp_err_t app_config_store(const app_config_t *cfg) {
  if (cfg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // TODO: store to EEPROM

  ESP_LOGI(TAG, "Stored app config");
  return ESP_OK;
}
