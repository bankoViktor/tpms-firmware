/**
 ********************************************************************************
 * @file    srvc_can_tx.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    06.02.2025
 * @brief   Header file of the CAN transmitter service.
 ********************************************************************************
 */

#ifndef SRVC_CAN_TX___H
#define SRVC_CAN_TX___H

#include "tpms_core.h"
#include <esp_err.h>

#define SRVC_CAN_TX_STACK_DEPTH 4096
#define SRVC_CAN_TX_PRIORITY 4
#define SRVC_CAN_TX_INTERVAL pdMS_TO_TICKS(100)

/// @brief Start of the CAN Bus Transmitter service in separete task.
/// @param tpms_core TPMS core.
/// @return Status code.
esp_err_t srvc_can_tx(tpms_core_t *tpms_core);

#endif
