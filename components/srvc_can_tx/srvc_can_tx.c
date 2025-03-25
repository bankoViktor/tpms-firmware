/**
 ********************************************************************************
 * @file    srvc_can_tx.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    06.02.2025
 * @brief   Source file of the CAN transmitter service.
 ********************************************************************************
 */

#include "srvc_can_tx.h"
#include "ct2_can.h"
#include "tpms_core.h"
#include <driver/twai.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#define PRESSURE_KPA_TO_BAR(kpa) ((kpa) / 100.0)

static const char *TAG = "can_srv";

static uint32_t s_flags;

static twai_message_t s_twai_msg = {
    .identifier = CT2_CAN2_MSGID_TPMS,
    .extd = 0,
    .rtr = 0,
    .data_length_code = 8,
};

static void fill_sensor_data(const tpms_core_t *tpms_core,
                             ct2_msg_tpms_config_t *ch2_msg_cfg,
                             tpms_sensor_num_t sensor_num) {
  assert(tpms_core != NULL && ch2_msg_cfg != NULL &&
         sensor_num < SENSOR_TIRE_MAX);

  // Get sensor data
  bool data_valid;
  bool tire_alarm;
  tpms_sensor_data_t sensor_data;
  esp_err_t ret = tpms_core_get_sensor_data(sensor_num, &data_valid,
                                            &tire_alarm, &sensor_data);
  if (ret == ESP_ERR_NOT_ALLOWED) {
    return;
  } else if (ret != ESP_OK) {
    ESP_LOGW(TAG, "Get data from TPMS core (%s)", esp_err_to_name(ret));
    return;
  }

  // Fill data for sensor
  float pressure_bar = PRESSURE_KPA_TO_BAR(sensor_data.pressure_kpa);
  switch (sensor_num) {
  case SENSOR_TIRE_FRONT_LEFT:
    ch2_msg_cfg->pressure_fl_bar = data_valid ? pressure_bar : -1;
    ch2_msg_cfg->tire_alarm_fl = data_valid ? tire_alarm : 0;
    break;
  case SENSOR_TIRE_FRONT_RIGHT:
    ch2_msg_cfg->pressure_fr_bar = data_valid ? pressure_bar : -1;
    ch2_msg_cfg->tire_alarm_fr = data_valid ? tire_alarm : 0;
    break;
  case SENSOR_TIRE_REAR_LEFT:
    ch2_msg_cfg->pressure_rl_bar = data_valid ? pressure_bar : -1;
    ch2_msg_cfg->tire_alarm_rl = data_valid ? tire_alarm : 0;
    break;
  case SENSOR_TIRE_REAR_RIGHT:
    ch2_msg_cfg->pressure_rr_bar = data_valid ? pressure_bar : -1;
    ch2_msg_cfg->tire_alarm_rr = data_valid ? tire_alarm : 0;
    break;
  default:
    break;
  }
}

static void fill_data(const tpms_core_t *tpms_core) {
  assert(tpms_core != NULL);

  // Fill data of the CT2 message
  ct2_msg_tpms_config_t ct2_msg_cfg = CT2_CAN_MSG_TPMS_CONFIG_DEFAULT();

  // Fill data from sensors
  fill_sensor_data(tpms_core, &ct2_msg_cfg, SENSOR_TIRE_FRONT_LEFT);
  fill_sensor_data(tpms_core, &ct2_msg_cfg, SENSOR_TIRE_FRONT_RIGHT);
  fill_sensor_data(tpms_core, &ct2_msg_cfg, SENSOR_TIRE_REAR_LEFT);
  fill_sensor_data(tpms_core, &ct2_msg_cfg, SENSOR_TIRE_REAR_RIGHT);

  // Set master light
  switch (tpms_core->master_alarm) {
  case TPMS_ALARM_CAUTION:
    ct2_msg_cfg.master_alarm = CT2_ALARM_CONTINUOUSLY;
    break;

  case TPMS_ALARM_CRITICAL:
    ct2_msg_cfg.master_alarm = CT2_ALARM_BLINKING;
    break;

  case TPMS_ALARM_NONE:
  default:
    ct2_msg_cfg.master_alarm = CT2_ALARM_NONE;
    break;
  }

  ct2_msg_cfg.master_alarm = tpms_core->master_alarm;

  // Fill data to TWAI message
  ct2_can_tpms_config_msg(&ct2_msg_cfg, s_twai_msg.data);
}

static void transmit_message(const tpms_core_t *tpms_core) {
  assert(tpms_core != NULL);

  // Read TWAI state
  twai_status_info_t twai_status;
  ESP_ERROR_CHECK(twai_get_status_info(&twai_status));
  switch (twai_status.state) {

  case TWAI_STATE_STOPPED:
    ESP_ERROR_CHECK(twai_start());
    break;

  case TWAI_STATE_BUS_OFF:
    ESP_ERROR_CHECK(twai_initiate_recovery());
    return;

  case TWAI_STATE_RECOVERING:
    return;

  case TWAI_STATE_RUNNING:
    break;

  default:
    break;
  }

  // Fill data for TX
  fill_data(tpms_core);

  // Transmit
  esp_err_t ret =
      twai_transmit(&s_twai_msg, pdMS_TO_TICKS(CONFIG_SRVC_CAN_TIMEOUT_MS));
  if (ret == ESP_OK) {

    if (s_flags & SRVC_CAN_TX_FLAG_LINK_FAULT) {
      ESP_LOGI(TAG, "TWAI bus recover");
    }

    // Clear fault flag
    s_flags &= ~SRVC_CAN_TX_FLAG_LINK_FAULT;
  } else if (ret != ESP_OK) {

    if (!(s_flags & SRVC_CAN_TX_FLAG_LINK_FAULT)) {
      ESP_LOGW(TAG, "Failded to transmit TWAI (%s)", esp_err_to_name(ret));
    }

    // Set fault flag
    s_flags |= SRVC_CAN_TX_FLAG_LINK_FAULT;
  }
}

static esp_err_t can_config() {
  // Initialize TWAI configuration structures
  twai_general_config_t g_cfg = TWAI_GENERAL_CONFIG_DEFAULT(
      CONFIG_SRVC_CAN_TX_IO_NUM, CONFIG_SRVC_CAN_RX_IO_NUM, TWAI_MODE_NORMAL);
  twai_timing_config_t t_cfg = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_cfg = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  esp_err_t ret = twai_driver_install(&g_cfg, &t_cfg, &f_cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install TWAI driver (%s)", esp_err_to_name(ret));
    return ret;
  }

  // Старт CAN
  ret = twai_start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to TWAI start (%s)", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "CAN configured");
  return ret;
}

static void src_proc(void *arg) {
  assert(arg != NULL);
  tpms_core_t *tpms_core = (tpms_core_t *)arg;
  ESP_LOGI(TAG, "Service started");

  esp_err_t ret = can_config();
  if (ret == ESP_OK) {
    while (true) {
      transmit_message(tpms_core);

      vTaskDelay(pdMS_TO_TICKS(CONFIG_SRVC_CAN_INTERVAL_MS));
    }
  }

  ESP_LOGI(TAG, "Service stopped");
  vTaskDelete(0);
}

uint32_t rvc_can_tx_get_flags() { return s_flags; }

esp_err_t srvc_can_tx_init(tpms_core_t *tpms_core) {
  assert(tpms_core != NULL);
  ESP_LOGD(TAG, "CAN Transmitter service starting...");

  BaseType_t rtos_ret = xTaskCreatePinnedToCore(
      src_proc, "can_srv", CONFIG_SRVC_CAN_TASK_STACK_DEPTH, tpms_core,
      CONFIG_SRVC_CAN_TASK_PRIORITY, NULL, 0);
  if (rtos_ret != pdPASS) {
    ESP_LOGE(TAG, "Create service task fail (RTOS error: %i)", rtos_ret);
    return ESP_FAIL;
  }

  return ESP_OK;
}
