/**
 ********************************************************************************
 * @file    uhf_receiver_srvc.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Header file of the UHF receiver service.
 ********************************************************************************
 */

#ifndef UHF_RECEIVER_SRVC__H
#define UHF_RECEIVER_SRVC__H

#include <esp_err.h>
#include "tpms_core.h"

#define SRVC_UHF_RCV_STACK_DEPTH 4096
#define SRVC_UHF_RCV_PRIORITY 5
#define SRVC_UHF_RCV_INTERVAL pdMS_TO_TICKS(50)

/// @brief Start of the UHF Receiver service in separete task.
/// @param tpms_core TPMS core.
/// @return Status code.
esp_err_t uhf_receiver_start_srvc(tpms_core_t *tpms_core);

#endif
