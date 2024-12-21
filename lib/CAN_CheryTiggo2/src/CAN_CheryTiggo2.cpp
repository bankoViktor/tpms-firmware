/**
 ******************************************************************************
 * @file    CAN_CheryTiggo2.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    2024-12-01
 * @brief   Source file for CAN-bus types for Chery Tiggo 2.
 ******************************************************************************
 */

#include "CAN_CheryTiggo2.h"
#include "CAN_CheryTiggo2_Types.h"

Ct2CanMsgTpms::Ct2CanMsgTpms() : m_bmFlags(0),
                                 m_wReserved(0),
                                 m_bPressureFL(0),
                                 m_bPressureFR(0),
                                 m_bPressureRL(0),
                                 m_bPressureRR(0) {}

float Ct2CanMsgTpms::pressureFL()
{
    return PRESSURE_CONVERT_FROM(m_bPressureFL);
}

void Ct2CanMsgTpms::setPressureFL(float fPressure_bar)
{
    m_bPressureFL = PRESSURE_CONVERT_TO(fPressure_bar);
}

float Ct2CanMsgTpms::pressureFR()
{
    return PRESSURE_CONVERT_FROM(m_bPressureFR);
}

void Ct2CanMsgTpms::setPressureFR(float fPressure_bar)
{
    m_bPressureFR = PRESSURE_CONVERT_TO(fPressure_bar);
}

float Ct2CanMsgTpms::pressureRL()
{
    return PRESSURE_CONVERT_FROM(m_bPressureRL);
}

void Ct2CanMsgTpms::setPressureRL(float fPressure_bar)
{
    m_bPressureRL = PRESSURE_CONVERT_TO(fPressure_bar);
}

float Ct2CanMsgTpms::pressureRR()
{
    return PRESSURE_CONVERT_FROM(m_bPressureRR);
}

void Ct2CanMsgTpms::setPressureRR(float fPressure_bar)
{
    m_bPressureRR = PRESSURE_CONVERT_TO(fPressure_bar);
}

bool Ct2CanMsgTpms::tireAlarmFL()
{
    return m_bmFlags & TPMS_ALARM_TIRE_FL;
}

void Ct2CanMsgTpms::setTireAlarmFL(bool xEnable)
{
    UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_FL);
}

bool Ct2CanMsgTpms::tireAlarmFR()
{
    return m_bmFlags & TPMS_ALARM_TIRE_FR;
}

void Ct2CanMsgTpms::setTireAlarmFR(bool xEnable)
{
    UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_FR);
}

bool Ct2CanMsgTpms::tireAlarmRL()
{
    return m_bmFlags & TPMS_ALARM_TIRE_RL;
}

void Ct2CanMsgTpms::setTireAlarmRL(bool xEnable)
{
    UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_RL);
}

bool Ct2CanMsgTpms::tireAlarmRR()
{
    return m_bmFlags & TPMS_ALARM_TIRE_RR;
}

void Ct2CanMsgTpms::setTireAlarmRR(bool xEnable)
{
    UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_RR);
}

bool Ct2CanMsgTpms::alarmContinuous()
{
    return m_bmFlags & TPMS_ALARM_CONTINUOIS;
}

void Ct2CanMsgTpms::setAlarmContinuous(bool xEnable)
{
    UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_CONTINUOIS);
}

bool Ct2CanMsgTpms::alarmStrobe()
{
    return m_bmFlags & TPMS_ALARM_STROBE;
}

void Ct2CanMsgTpms::setAlarmStrobe(bool xEnable)
{
    UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_STROBE);
}
