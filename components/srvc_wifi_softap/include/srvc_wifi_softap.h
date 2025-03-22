/**
 ********************************************************************************
 * @file    srvc_wifi_softap.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Header file of the Wi-Fi AP.
 ********************************************************************************
 */

#ifndef SRVC_WIFI_SOFTAP__H
#define SRVC_WIFI_SOFTAP__H

#include "app_config.h"
#include <esp_err.h>

/// @brief Start Wi-Fi Access Point.
/// @param app_wifi_cfg Wi-Fi configuration of the application.
/// @param is_wifi_pwd_suppress Flag of the password suppressed. If not equal to
/// 0 then Wi-Fi AP started in open mode.
/// @return Status code.
esp_err_t srvc_wifi_softap(const app_wifi_config_t *app_wifi_cfg,
                           uint8_t is_wifi_pwd_suppress);

#endif