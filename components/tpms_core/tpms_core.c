/**
 ********************************************************************************
 * @file    tpms_core.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    03.01.2025
 * @brief   Source file of the TPMS Core.
 ********************************************************************************
 */

#include "tpms_core.h"
#include <esp_log.h>
#include <string.h>

#define TAKE_MUTEX_OR_ERR_RETURN()                                             \
  while (0) {                                                                  \
    ESP_LOGV(TAG, "Take mutext");                                              \
    if (xSemaphoreTake(tpms_core->mutex, TPMS_CORE_TAKE_MUTEX_TIMEOUT) ==      \
        pdFAIL) {                                                              \
      esp_err_t err_rc_ = ESP_ERR_TIMEOUT;                                     \
      ESP_LOGE(TAG, "Take Mutex timeout (%s)", esp_err_to_name(err_rc_));      \
      return err_rc_;                                                          \
    }                                                                          \
  }

#define GIVE_MUTEX()                                                           \
  while (0) {                                                                  \
    ESP_LOGV(TAG, "Give mutext");                                              \
    xSemaphoreGive(tpms_core->mutex);                                          \
  }
#define SET_BIT(bm, mask) ((bm) |= (mask))
#define RESET_BIT(bm, mask) ((bm) &= ~(mask))
#define UPDATE_BIT(bm, mask, expr)                                             \
  if (expr) {                                                                  \
    SET_BIT(bm, mask);                                                         \
  } else {                                                                     \
    RESET_BIT(bm, mask);                                                       \
  }

static const char *TAG = "tpms_core";

static void get_sensor_by_id(tpms_core_t *tpms_core, tpms_sensor_id_t sensor_id,
                             tpms_sensor_t **sensor_out) {
  assert(tpms_core != NULL);
  assert(sensor_id != 0);
  assert(sensor_out != NULL);

  tpms_sensor_t *target_sensor = NULL;

  for (uint8_t i = 0; i < SENSOR_TIRE_MAX; i++) {
    tpms_sensor_t *sensor = &tpms_core->sensors[i];
    if (sensor->id == sensor_id) {
      target_sensor = sensor;
      break;
    }
  }

  *sensor_out = target_sensor;
}

static void update_sensor_state(tpms_core_t *tpms_core, tpms_sensor_t *sensor) {
  assert(tpms_core != NULL);
  assert(sensor != NULL);

  if (sensor->id != 0) {
    RESET_BIT(sensor->flags,
              SENSOR_FLAG_CAUTION_ALARM | SENSOR_FLAG_CRITICAL_ALARM);
  } else {
    uint8_t is_caution_pressure =
        sensor->data.pressure_kpa <
            (tpms_core->config->pressure_kpa_normal -
             tpms_core->config->pressure_kpa_caution_dev) ||
        sensor->data.pressure_kpa >
            (tpms_core->config->pressure_kpa_normal +
             tpms_core->config->pressure_kpa_caution_dev);

    uint8_t is_critical_pressure =
        sensor->data.pressure_kpa <
            (tpms_core->config->pressure_kpa_normal -
             tpms_core->config->pressure_kpa_critical_dev) ||
        sensor->data.pressure_kpa >
            (tpms_core->config->pressure_kpa_normal +
             tpms_core->config->pressure_kpa_critical_dev);

    uint8_t is_caution_temperature =
        sensor->data.temperature_c >
        tpms_core->config->temperature_c_caution_thr;

    uint8_t is_critical_temperature =
        sensor->data.temperature_c >
        tpms_core->config->temperature_c_critical_thr;

    UPDATE_BIT(sensor->flags, SENSOR_FLAG_CAUTION_ALARM,
               is_caution_pressure || is_caution_temperature);
    UPDATE_BIT(sensor->flags, SENSOR_FLAG_CRITICAL_ALARM,
               is_critical_pressure || is_critical_temperature);
  }
}

static void update_core_state(tpms_core_t *tpms_core) {
  assert(tpms_core != NULL);

  uint8_t is_critical_alarm = 0;

  // Update sensors state
  tpms_sensor_num_t sensor_num = 0;
  for (; sensor_num < SENSOR_TIRE_MAX; sensor_num++) {
    tpms_sensor_t *sensor = &tpms_core->sensors[sensor_num];
    update_sensor_state(tpms_core, sensor);
    is_critical_alarm =
        is_critical_alarm || (sensor->flags & SENSOR_FLAG_CRITICAL_ALARM);
  }

  // Update master state
  tpms_core->master_alarm =
      is_critical_alarm ? TPMS_ALARM_CRITICAL : TPMS_ALARM_NONE;

  ESP_LOGD(TAG, "Core state updated");
}

esp_err_t tpms_core_init(tpms_core_t *tpms_core,
                         const app_tpms_config_t *app_cfg) {
  if (tpms_core == NULL || app_cfg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  tpms_core->config = app_cfg;

  // Create Mutex
  tpms_core->mutex = xSemaphoreCreateMutex();
  if (tpms_core->mutex == NULL) {
    return ESP_ERR_NO_MEM;
  }

  // Sensors config
  tpms_sensor_num_t sensor_num = 0;
  for (; sensor_num < SENSOR_TIRE_MAX; sensor_num++) {
    tpms_sensor_id_t sensor_id = app_cfg->sensor_ids[sensor_num];
    if (sensor_id != 0) {
      esp_err_t ret =
          tpms_core_register_sensor(tpms_core, sensor_id, sensor_num);
      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failded to registry sensor (%s)", esp_err_to_name(ret));
      }
    }
  }

  return ESP_OK;
}

esp_err_t tpms_core_register_sensor(tpms_core_t *tpms_core,
                                    tpms_sensor_id_t sensor_id,
                                    tpms_sensor_num_t sensor_num) {
  if (tpms_core == NULL || sensor_id == 0 || sensor_num >= SENSOR_TIRE_MAX) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  esp_err_t ret = ESP_OK;

  // Find sensor
  tpms_sensor_t *sensor;
  get_sensor_by_id(tpms_core, sensor_id, &sensor);
  if (sensor == NULL) {
    // Set data
    sensor = &tpms_core->sensors[sensor_num];
    sensor->id = sensor_id;
    sensor->flags = 0;
    ESP_LOGI(TAG, "Registered sensor %08lX", sensor_id);
  } else {
    ret = ESP_ERR_NOT_ALLOWED;
  }

  GIVE_MUTEX();

  return ret;
}

esp_err_t tpms_core_unregister_sensor(tpms_core_t *tpms_core,
                                      tpms_sensor_id_t sensor_id) {
  if (tpms_core == NULL || sensor_id == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  esp_err_t ret = ESP_OK;

  // Find sensor
  tpms_sensor_t *sensor;
  get_sensor_by_id(tpms_core, sensor_id, &sensor);
  if (sensor != NULL) {
    // Set data
    sensor->id = 0;
    sensor->flags = 0;
    ESP_LOGI(TAG, "Unregistered sensor %08lX", sensor_id);
  } else {
    ret = ESP_ERR_NOT_ALLOWED;
  }

  GIVE_MUTEX();

  return ret;
}

esp_err_t tpms_core_update_sensor_data(tpms_core_t *tpms_core,
                                       tpms_sensor_id_t sensor_id,
                                       const tpms_sensor_data_t *sensor_data) {
  if (tpms_core == NULL || sensor_id == 0 || sensor_data == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  // Find sensor
  tpms_sensor_t *sensor;
  get_sensor_by_id(tpms_core, sensor_id, &sensor);
  if (sensor != NULL) {

    // Copy new data of the sensor
    memcpy(&sensor->data, sensor_data, sizeof(tpms_sensor_data_t));
    SET_BIT(sensor->flags, SENSOR_FLAG_VALID_DATA);
    ESP_LOGD(TAG, "Updated data of sensor %08lX", sensor->id);

    // Update TPMS core state
    update_core_state(tpms_core);
  }

  GIVE_MUTEX();

  return ESP_OK;
}

esp_err_t tpms_core_get_sensor_data(const tpms_core_t *tpms_core,
                                    tpms_sensor_num_t sensor_num,
                                    bool *data_valid_out, bool *tire_alarm_out,
                                    tpms_sensor_data_t *data_out) {
  if (tpms_core == NULL || sensor_num >= SENSOR_TIRE_MAX ||
      data_valid_out == NULL || tire_alarm_out == NULL || data_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  esp_err_t ret = ESP_OK;

  *data_valid_out = 0;
  *tire_alarm_out = 0;

  // Get Sensor
  const tpms_sensor_t *sensor = &tpms_core->sensors[sensor_num];
  if (sensor->id != 0x00) {
    if (sensor->flags & SENSOR_FLAG_VALID_DATA) {
      *data_valid_out = (sensor->flags & SENSOR_FLAG_VALID_DATA);
      *tire_alarm_out = (sensor->flags & SENSOR_FLAG_CAUTION_ALARM) ||
                        (sensor->flags & SENSOR_FLAG_CRITICAL_ALARM);

      memcpy(data_out, &sensor->data, sizeof(tpms_sensor_data_t));
    }
  } else {
    ret = ESP_ERR_NOT_ALLOWED;
  }

  GIVE_MUTEX();

  return ret;
}
