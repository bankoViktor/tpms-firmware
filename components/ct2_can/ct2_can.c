/**
 ********************************************************************************
 * @file    ct2_can.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Source file of the Chery Tiggo 2 CAN.
 ********************************************************************************
 */

#include "ct2_can.h"
#include <math.h>
#include <string.h>

#define PRESSURE_BAR_SCALE 0.01771653543
#define PRESSURE_CONVERT_TO(val) ((uint8_t)(round((val) / PRESSURE_BAR_SCALE)))
#define PRESSURE_NOT_AVAILABLE_VALUE 0xFF
#define GET_PRESSURE_DATA(val)                                                 \
  (val < 0 ? PRESSURE_NOT_AVAILABLE_VALUE : PRESSURE_CONVERT_TO(val))
#define SET_BIT(byte, mask) ((byte) |= (mask))
#define RESET_BIT(byte, mask) ((byte) &= ~(mask))
#define UPDATE_BIT(byte, newState, mask)                                       \
  if (newState) {                                                              \
    SET_BIT((byte), (mask));                                                   \
  } else {                                                                     \
    RESET_BIT((byte), (mask));                                                 \
  }

void ct2_can_tpms_config_msg(const ct2_msg_tpms_config_t *cfg,
                             uint8_t *buffer_out) {
  // Clear buffer
  memset(buffer_out, 0x00, 8);

  // Set flags for Front Tire Alarm Lights
  UPDATE_BIT(buffer_out[0], cfg->tire_alarm_fr, 0x01);
  UPDATE_BIT(buffer_out[0], cfg->tire_alarm_fl, 0x02);

  // Set flags TPMS Master Alarm Light
  switch (cfg->master_alarm) {
  case CT2_ALARM_NONE:
    break;

  case CT2_ALARM_CONTINUOUSLY:
    SET_BIT(buffer_out[0], 0x20);
    break;

  case CT2_ALARM_BLINKING:
    SET_BIT(buffer_out[0], 0x10);
    break;
  }

  // Set flags for Rear Tire Alarm Lights
  UPDATE_BIT(buffer_out[1], cfg->tire_alarm_rr, 0x01);
  UPDATE_BIT(buffer_out[1], cfg->tire_alarm_rl, 0x10);

  // Set Tire Pressures
  buffer_out[4] = GET_PRESSURE_DATA(cfg->pressure_fl_bar);
  buffer_out[5] = GET_PRESSURE_DATA(cfg->pressure_fr_bar);
  buffer_out[6] = GET_PRESSURE_DATA(cfg->pressure_rl_bar);
  buffer_out[7] = GET_PRESSURE_DATA(cfg->pressure_rr_bar);
}
