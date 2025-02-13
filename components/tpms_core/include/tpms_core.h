/**
 ********************************************************************************
 * @file    tpms_core.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    03.01.2025
 * @brief   Header file of the TPMS Core.
 ********************************************************************************
 */

#ifndef TPMS_CORE__H
#define TPMS_CORE__H

#include "app_config.h"
#include "tpms_sensor.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>

#ifndef TPMS_CORE_TAKE_MUTEX_TIMEOUT
#define TPMS_CORE_TAKE_MUTEX_TIMEOUT pdMS_TO_TICKS(500)
#endif

/// @brief Types of the alarm light.
typedef enum tpms_alarm_type_t {
  TPMS_ALARM_NONE,
  TPMS_ALARM_CAUTION,
  TPMS_ALARM_CRITICAL,
} tpms_alarm_type_t;

/// @brief TPMS core.
typedef struct tpms_core_t {
  SemaphoreHandle_t mutex;                // Mutex of TPMS code.
  tpms_sensor_t sensors[SENSOR_TIRE_MAX]; // Sensor identifiers.
  tpms_alarm_type_t master_alarm;         // Master alarm type.
  const app_tpms_config_t *config;        // Application configuration.
} tpms_core_t;

/// @brief Initialize TPMS core.
/// @param app_cfg Application configuration.
/// @param tpms_core_out Output TPMS core.
/// @return Status code.
esp_err_t tpms_core_init(const app_tpms_config_t *app_cfg,
                         tpms_core_t **tpms_core_out);

/// @brief Register specified sensor by sensor identifier and sensor number.
/// @param sensor_id Sensor identifier.
/// @param sensor_num Number of the sensor.
/// @return Status code.
esp_err_t tpms_core_register_sensor(tpms_sensor_id_t sensor_id,
                                    tpms_sensor_num_t sensor_num);

/// @brief Unregister specified sensor by sensor identifier.
/// @param sensor_id Sensor identifier.
/// @return Status code.
esp_err_t tpms_core_unregister_sensor(tpms_sensor_id_t sensor_id);

/// @brief Update data of the sensor by sensor identifier.
/// @param sensor_id Sensor identifier.
/// @param sensor_data_out New data of the specified sensor.
/// @return Status code.
esp_err_t
tpms_core_update_sensor_data(tpms_sensor_id_t sensor_id,
                             const tpms_sensor_data_t *sensor_data_out);

/// @brief Update data of the sensor by sensor identifier.
/// @param sensor_num Number of the sensor.
/// @param data_valid_out Output sensor validatiton flag.
/// @param tire_alarm_out Output sensor alarm flag.
/// @param data_out Output sensor data.
/// @return Status code.
esp_err_t tpms_core_get_sensor_data(tpms_sensor_num_t sensor_num,
                                    bool *data_valid_out, bool *tire_alarm_out,
                                    tpms_sensor_data_t *data_out);

#endif
