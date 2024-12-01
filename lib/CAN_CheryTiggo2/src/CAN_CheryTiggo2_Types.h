/**
 ******************************************************************************
 * @file    CAN_CheryTiggo2_Types.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    2024-12-01
 * @brief   Header file for types of CAN messages.
 ******************************************************************************
 */

#ifndef CAN_CHERYTIGGO2_TYPES__H
#define CAN_CHERYTIGGO2_TYPES__H

#include <stdint.h>
#include <math.h>

constexpr float PRESSURE_BAR_SCALE = 0.01771653543;

#define PRESSURE_CONVERT_TO(val) ((uint8_t)(round((val) / PRESSURE_BAR_SCALE)))
#define PRESSURE_CONVERT_FROM(val) ((float)((val) * PRESSURE_BAR_SCALE))
#define UPDATE_BIT(bm, newState, mask) \
    if ((newState))                    \
    {                                  \
        (bm) |= (mask);                \
    }                                  \
    else                               \
    {                                  \
        (bm) &= ~(mask);               \
    }

enum can_msg_tpms_flags_t : uint16_t
{
    TPMS_ALARM_TIRE_FR = 0x0001,
    TPMS_ALARM_TIRE_FL = 0x0002,
    TPMS_ALARM_STROBE = 0x0010,
    TPMS_ALARM_CONTINUOIS = 0x0020,
    TPMS_ALARM_TIRE_RR = 0x0100,
    TPMS_ALARM_TIRE_RL = 0x1000,
};

#endif
