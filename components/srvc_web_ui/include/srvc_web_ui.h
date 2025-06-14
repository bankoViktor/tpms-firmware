/**
 ********************************************************************************
 * @file    srvc_web_ui.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.02.2025
 * @brief   Header file of the Web UI.
 ********************************************************************************
 */

#ifndef SRVC_WEB_UI__H
#define SRVC_WEB_UI__H

#include "tpms_core.h"
#include <esp_err.h>

/// @brief Start Web UI server.
/// @param tpms_core TPMS core.
/// @return Status code.
esp_err_t srvc_web_ui_init(tpms_core_t *tpms_core, app_config_t* config);

#endif
