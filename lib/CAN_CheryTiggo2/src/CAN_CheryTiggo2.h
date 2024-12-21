/**
 ******************************************************************************
 * @file    CAN_CheryTiggo2.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    2024-12-01
 * @brief   Header file for CAN-bus types for Chery Tiggo 2.
 ******************************************************************************
 */

#ifndef CAN_CHERYTIGGO2__H
#define CAN_CHERYTIGGO2__H

#include <stdint.h>

/// CAN1: DI (6 (CAN-H) and 14 (CAN-L) pins), BCM, ECM, etc.
///
/// Nodes: ECM, BCM.
constexpr long CAN1_BITRATE = 500E3;

/// @brief CAN2: DI (3 (CAN-H) and 11 (CAN-L) pins), BCM, IC, etc.
///
/// Nodes: IC, BCM.
constexpr long CAN2_BITRATE = 500E3;

/// @brief CAN message IDs.
enum Ct2CanMsgIds
{
    CT2_CAN1_MSGID_TPMS = 0x51B,
};

/// @brief TPMS CAN message.
class Ct2CanMsgTpms
{
private:
    uint16_t m_bmFlags;
    uint16_t m_wReserved;
    uint8_t m_bPressureFL;
    uint8_t m_bPressureFR;
    uint8_t m_bPressureRL;
    uint8_t m_bPressureRR;

public:
    Ct2CanMsgTpms();
    float pressureFL();
    void setPressureFL(float fPressure_bar);
    float pressureFR();
    void setPressureFR(float fPressure_bar);
    float pressureRL();
    void setPressureRL(float fPressure_bar);
    float pressureRR();
    void setPressureRR(float fPressure_bar);
    bool tireAlarmFL();
    void setTireAlarmFL(bool xEnable = true);
    bool tireAlarmFR();
    void setTireAlarmFR(bool xEnable = true);
    bool tireAlarmRL();
    void setTireAlarmRL(bool xEnable = true);
    bool tireAlarmRR();
    void setTireAlarmRR(bool xEnable = true);
    bool alarmContinuous();
    void setAlarmContinuous(bool xEnable = true);
    bool alarmStrobe();
    void setAlarmStrobe(bool xEnable = true);
};

#endif
