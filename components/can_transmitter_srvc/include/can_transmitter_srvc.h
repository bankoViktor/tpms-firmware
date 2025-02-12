/**
 ********************************************************************************
 * @file    can_transmitter_srvc.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    06.02.2025
 * @brief   Header file of the CAN transmitter service.
 ********************************************************************************
 */

#ifndef CAN_TRANSMITTER_SRVC__H
#define CAN_TRANSMITTER_SRVC__H

#include "tpms_core.h"
#include <esp_err.h>

#define SRVC_CAN_TMTR_STACK_DEPTH 4096
#define SRVC_CAN_TMTR_PRIORITY 4
#define SRVC_CAN_TMTR_INTERVAL pdMS_TO_TICKS(100)

/// @brief Start of the CAN Bus Transmitter service in separete task.
/// @param tpms_core TPMS core.
/// @return Status code.
esp_err_t can_transmitter_start_srvc(tpms_core_t *tpms_core);

#endif
