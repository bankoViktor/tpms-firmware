/**
 ********************************************************************************
 * @file    ct2_can.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Header file of the Chery Tiggo 2 CAN.
 ********************************************************************************
 */

#ifndef CT2_CAN__H
#define CT2_CAN__H

#include <stdint.h>

/// CAN1: DI (6 (CAN-H) and 14 (CAN-L) pins), BCM, ECM, etc.
///
/// Nodes: ECM, BCM.
#define CT2_CAN1_BITRATE 500E3;

/// CAN2: DI (3 (CAN-H) and 11 (CAN-L) pins), BCM, IC, etc.
///
/// Nodes: IC, BCM.
#define CT2_CAN2_BITRATE 500E3;

/// @brief CAN message IDs.
enum ct2_can_msgid_t {
  /// @brief Parking radar (3 rear sensors only)
  CT2_CAN2_MSGID_PARKING_RADAR = 0x440,

  /// @brief Get current time on Instrument Cluster (HH:MM)
  CT2_CAN2_MSGID_GETT_IME = 0x453,

  /// @brief Set current time on Instrument Cluster (HH:MM)
  CT2_CAN2_MSGID_SET_TIME = 0x517,

  /// @brief Tire Pressure Monitoring System
  CT2_CAN2_MSGID_TPMS = 0x51B,
};

/// @brief Modes of the TPMS Warn light on IC.
typedef enum ct2_msg_alarm_light_mode_t {
  CT2_ALARM_NONE,
  CT2_ALARM_CONTINUOUSLY,
  CT2_ALARM_BLINKING,
} ct2_msg_alarm_light_mode_t;

/// @brief Configuration struct for TPMS message.
typedef struct ct2_msg_tpms_config_t {
  float pressure_fl_bar; // Pressure [bar] of the Front-Left tire.
  float pressure_fr_bar; // Pressure [bar] of the Front-Right tire.
  float pressure_rl_bar; // Pressure [bar] of the Rear-Left tire.
  float pressure_rr_bar; // Pressure [bar] of the Rear-Right tire.
  uint8_t tire_alarm_fl; // Alarm light of the Front-Left tire.
  uint8_t tire_alarm_fr; // Alarm light of the Front-Right tire.
  uint8_t tire_alarm_rl; // Alarm light of the Rear-Left tire.
  uint8_t tire_alarm_rr; // Alarm light of the Rear-Right tire.
  ct2_msg_alarm_light_mode_t master_alarm; // Mode of the TPMS Warn light.
} ct2_msg_tpms_config_t;

#define CT2_CAN_MSG_TPMS_CONFIG_DEFAULT()                                      \
  {                                                                            \
      .pressure_fl_bar = -1,                                                   \
      .pressure_fr_bar = -1,                                                   \
      .pressure_rl_bar = -1,                                                   \
      .pressure_rr_bar = -1,                                                   \
      .tire_alarm_fl = 0,                                                      \
      .tire_alarm_fr = 0,                                                      \
      .tire_alarm_rl = 0,                                                      \
      .tire_alarm_rr = 0,                                                      \
      .master_alarm = CT2_ALARM_NONE,                                          \
  }

/// @brief Configure CAN message for TPMS.
/// @param config Configuration structure.
/// @param buffer_out Output buffer of the CAN TPMS message.
void ct2_can_tpms_config_msg(const ct2_msg_tpms_config_t *config,
                             uint8_t *buffer_out);

#endif
