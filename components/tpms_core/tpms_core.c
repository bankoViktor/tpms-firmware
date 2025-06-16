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
    if (xSemaphoreTake(s_tpms_core.mutex, TPMS_CORE_TAKE_MUTEX_TIMEOUT) ==     \
        pdFAIL) {                                                              \
      esp_err_t err_rc_ = ESP_ERR_TIMEOUT;                                     \
      ESP_LOGE(TAG, "Take Mutex timeout (%s)", esp_err_to_name(err_rc_));      \
      return err_rc_;                                                          \
    }                                                                          \
  }

#define GIVE_MUTEX()                                                           \
  while (0) {                                                                  \
    ESP_LOGV(TAG, "Give mutext");                                              \
    xSemaphoreGive(s_tpms_core.mutex);                                         \
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

static tpms_core_t s_tpms_core;

static void get_sensor_by_id(tpms_sensor_id_t sensor_id,
                             tpms_sensor_t **sensor_out,
                             tpms_sensor_num_t *sensor_num_out) {
  assert(sensor_id != 0);
  assert(sensor_out != NULL);

  tpms_sensor_t *target_sensor = NULL;

  tpms_sensor_num_t sensor_num = 0;
  for (; sensor_num < SENSOR_TIRE_MAX; sensor_num++) {
    tpms_sensor_t *sensor = &s_tpms_core.sensors[sensor_num];

    if (sensor->id == sensor_id) {
      target_sensor = sensor;
      break;
    }
  }

  *sensor_out = target_sensor;
  if (sensor_num_out != NULL) {
    *sensor_num_out = sensor_num;
  }
}

static void update_sensor_state(tpms_sensor_t *sensor, bool is_front) {
  assert(sensor != NULL);

  if (sensor->id == 0) {
    RESET_BIT(sensor->flags,
              SENSOR_FLAG_CAUTION_ALARM | SENSOR_FLAG_CRITICAL_ALARM);
  } else {

    float press_normal = is_front
                             ? s_tpms_core.config->pressure_kpa_normal_front
                             : s_tpms_core.config->pressure_kpa_normal_rear;
    float press_caution_dev = s_tpms_core.config->pressure_kpa_caution_dev;
    float press_critical_dev = s_tpms_core.config->pressure_kpa_critical_dev;

    uint8_t is_caution_pressure =
        sensor->data.pressure_kpa < (press_normal - press_caution_dev) ||
        sensor->data.pressure_kpa > (press_normal + press_caution_dev);

    uint8_t is_critical_pressure =
        sensor->data.pressure_kpa < (press_normal - press_critical_dev) ||
        sensor->data.pressure_kpa > (press_normal + press_critical_dev);

    float temp_caution_max = s_tpms_core.config->temperature_c_caution_thr;
    float temp_critical_max = s_tpms_core.config->temperature_c_critical_thr;

    uint8_t is_caution_temperature =
        temp_caution_max > 0 && sensor->data.temperature_c > temp_caution_max;

    uint8_t is_critical_temperature =
        temp_critical_max > 0 && sensor->data.temperature_c > temp_critical_max;

    UPDATE_BIT(sensor->flags, SENSOR_FLAG_CAUTION_ALARM,
               is_caution_pressure || is_caution_temperature);
    UPDATE_BIT(sensor->flags, SENSOR_FLAG_CRITICAL_ALARM,
               is_critical_pressure || is_critical_temperature);
  }
}

static void update_core_state() {
  uint8_t is_critical_alarm = 0;

  // Update sensors state
  tpms_sensor_num_t sensor_num = 0;
  for (; sensor_num < SENSOR_TIRE_MAX; sensor_num++) {
    tpms_sensor_t *sensor = &s_tpms_core.sensors[sensor_num];

    bool is_front = sensor_num == SENSOR_TIRE_FRONT_LEFT ||
                    sensor_num == SENSOR_TIRE_FRONT_RIGHT;

    update_sensor_state(sensor, is_front);

    if (sensor->flags & SENSOR_FLAG_VALID_DATA) {
      is_critical_alarm =
          is_critical_alarm || (sensor->flags & SENSOR_FLAG_CRITICAL_ALARM);
    }
  }

  // Update master state
  s_tpms_core.master_alarm =
      is_critical_alarm ? TPMS_ALARM_CRITICAL : TPMS_ALARM_NONE;

  ESP_LOGD(TAG, "Core state updated");
}

static void sensor_timer_callback(void *arg) {
  assert(arg != NULL);
  tpms_sensor_t *sensor = (tpms_sensor_t *)arg;

  RESET_BIT(sensor->flags, SENSOR_FLAG_VALID_DATA);
  update_core_state();

  ESP_LOGD(TAG, "Timer: reset data for " SIDSTR, sensor->id);
}

static esp_err_t sensor_timer_create(tpms_sensor_t *sensor) {
  if (sensor == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_timer_create_args_t timer_args = {.callback = &sensor_timer_callback,
                                        .arg = sensor,
                                        .name = "sensor valid data"};
  esp_err_t ret =
      esp_timer_create(&timer_args, &sensor->valid_data_timer_handle);

  ESP_LOGD(TAG, "Timer: create timer for " SIDSTR, sensor->id);
  return ret;
}

static esp_err_t sensor_timer_delete(tpms_sensor_t *sensor) {
  if (sensor == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_timer_stop(sensor->valid_data_timer_handle);
  ESP_ERROR_CHECK(esp_timer_delete(sensor->valid_data_timer_handle));
  sensor->valid_data_timer_handle = 0;

  update_core_state();

  ESP_LOGD(TAG, "Timer: delete timer for " SIDSTR, sensor->id);
  return ESP_OK;
}

static esp_err_t sensor_timer_restart(tpms_sensor_t *sensor, uint64_t time_us) {
  if (sensor == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_timer_stop(sensor->valid_data_timer_handle);
  ESP_ERROR_CHECK(
      esp_timer_start_once(sensor->valid_data_timer_handle, time_us));

  update_core_state();

  ESP_LOGD(TAG, "Timer: restart timer for " SIDSTR, sensor->id);
  return ESP_OK;
}

esp_err_t tpms_core_init(const app_tpms_config_t *app_cfg,
                         tpms_core_t **tpms_core_out) {
  if (app_cfg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  s_tpms_core.config = app_cfg;

  // Create Mutex
  s_tpms_core.mutex = xSemaphoreCreateMutex();
  if (s_tpms_core.mutex == NULL) {
    return ESP_ERR_NO_MEM;
  }

  // Sensors config
  tpms_sensor_num_t sensor_num = 0;
  for (; sensor_num < SENSOR_TIRE_MAX; sensor_num++) {
    tpms_sensor_id_t sensor_id = app_cfg->sensor_ids[sensor_num];
    if (sensor_id != 0) {
      esp_err_t ret = tpms_core_register_sensor(sensor_id, sensor_num);
      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failded to registry sensor (%s)", esp_err_to_name(ret));
      }
    }
  }

  if (tpms_core_out != NULL) {
    *tpms_core_out = &s_tpms_core;
  }

  return ESP_OK;
}

esp_err_t tpms_core_register_sensor(tpms_sensor_id_t sensor_id,
                                    tpms_sensor_num_t sensor_num) {
  if (sensor_id == 0 || sensor_num >= SENSOR_TIRE_MAX) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  esp_err_t ret = ESP_OK;

  // Find sensor
  tpms_sensor_t *found_sensor;
  tpms_sensor_num_t found_sensor_num;
  get_sensor_by_id(sensor_id, &found_sensor, &found_sensor_num);
  if (found_sensor == NULL) {
    // Set data
    tpms_sensor_t *sensor = &s_tpms_core.sensors[sensor_num];
    sensor->id = sensor_id;
    sensor->flags = 0;

    // Create timer
    ESP_ERROR_CHECK(sensor_timer_create(sensor));

    ESP_LOGI(TAG, "Registered sensor " SIDSTR, sensor_id);
  } else {
    ESP_LOGW(TAG,
             "Try register dublicat sensor " SIDSTR
             " of current tire %i (new tire %i)",
             found_sensor->id, found_sensor_num, sensor_num);
    ret = ESP_ERR_NOT_ALLOWED;
  }

  GIVE_MUTEX();

  return ret;
}

esp_err_t tpms_core_unregister_sensor(tpms_sensor_id_t sensor_id) {
  if (sensor_id == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  esp_err_t ret = ESP_OK;

  // Find sensor
  tpms_sensor_t *sensor;
  get_sensor_by_id(sensor_id, &sensor, NULL);
  if (sensor != NULL) {
    // Delete timer
    ESP_ERROR_CHECK(sensor_timer_delete(sensor));

    // Set data
    sensor->id = 0;
    sensor->flags = 0;

    ESP_LOGI(TAG, "Unregistered sensor " SIDSTR, sensor_id);
  } else {
    ret = ESP_ERR_NOT_ALLOWED;
    ESP_LOGW(TAG, "Try unregister of unknown sensor " SIDSTR, sensor_id);
  }

  GIVE_MUTEX();

  return ret;
}

esp_err_t tpms_core_update_sensor_data(tpms_sensor_id_t sensor_id,
                                       const tpms_sensor_data_t *sensor_data) {
  if (sensor_id == 0 || sensor_data == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  // Find sensor
  tpms_sensor_t *sensor;
  get_sensor_by_id(sensor_id, &sensor, NULL);
  if (sensor != NULL) {
    // Copy new data of the sensor
    memcpy(&sensor->data, sensor_data, sizeof(tpms_sensor_data_t));
    SET_BIT(sensor->flags, SENSOR_FLAG_VALID_DATA);
    ESP_LOGD(TAG, "Updated data of sensor " SIDSTR, sensor->id);

    // Timer config
    if (s_tpms_core.config->valid_data_time_us == 0) {
      // Delete timer
      if (sensor->valid_data_timer_handle) {
        ESP_ERROR_CHECK(sensor_timer_delete(sensor));
      }
    } else {
      // Restart timer
      if (!sensor->valid_data_timer_handle) {
        ESP_ERROR_CHECK(sensor_timer_create(sensor));
      }

      ESP_ERROR_CHECK(
          sensor_timer_restart(sensor, s_tpms_core.config->valid_data_time_us));
    }

    // Update TPMS core state
    update_core_state();
  }

  GIVE_MUTEX();

  return ESP_OK;
}

esp_err_t tpms_core_get_sensor_data(tpms_sensor_num_t sensor_num,
                                    bool *data_valid_out, bool *tire_alarm_out,
                                    tpms_sensor_data_t *data_out) {
  if (sensor_num >= SENSOR_TIRE_MAX || data_valid_out == NULL ||
      tire_alarm_out == NULL || data_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  esp_err_t ret = ESP_OK;

  *data_valid_out = 0;
  *tire_alarm_out = 0;

  // Get Sensor
  const tpms_sensor_t *sensor = &s_tpms_core.sensors[sensor_num];
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
