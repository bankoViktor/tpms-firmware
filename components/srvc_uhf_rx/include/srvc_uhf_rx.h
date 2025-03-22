/**
 ********************************************************************************
 * @file    srvc_uhf_rx.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Header file of the UHF receiver service.
 ********************************************************************************
 */

#ifndef SRVC_UHF_RX__H
#define SRVC_UHF_RX__H

#include "tpms_core.h"
#include <esp_err.h>

#define SRVC_UHF_RX_STACK_DEPTH 4096
#define SRVC_UHF_RX_PRIORITY 5
#define SRVC_UHF_RX_INTERVAL pdMS_TO_TICKS(50)

/// @brief Start of the UHF Receiver service in separete task.
/// @param tpms_core TPMS core.
/// @return Status code.
esp_err_t srvc_uhf_rx(tpms_core_t *tpms_core);

#endif
