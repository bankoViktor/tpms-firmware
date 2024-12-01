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

can_msg_tpms_t::can_msg_tpms_t() : m_bmFlags(0),
								   m_wReserved(0),
								   m_bPressureFL(0),
								   m_bPressureFR(0),
								   m_bPressureRL(0),
								   m_bPressureRR(0) {}

float can_msg_tpms_t::pressureFL()
{
	return PRESSURE_CONVERT_FROM(m_bPressureFL);
}

void can_msg_tpms_t::setPressureFL(float fPressure_bar)
{
	m_bPressureFL = PRESSURE_CONVERT_TO(fPressure_bar);
}

float can_msg_tpms_t::pressureFR()
{
	return PRESSURE_CONVERT_FROM(m_bPressureFR);
}

void can_msg_tpms_t::setPressureFR(float fPressure_bar)
{
	m_bPressureFR = PRESSURE_CONVERT_TO(fPressure_bar);
}

float can_msg_tpms_t::pressureRL()
{
	return PRESSURE_CONVERT_FROM(m_bPressureRL);
}

void can_msg_tpms_t::setPressureRL(float fPressure_bar)
{
	m_bPressureRL = PRESSURE_CONVERT_TO(fPressure_bar);
}

float can_msg_tpms_t::pressureRR()
{
	return PRESSURE_CONVERT_FROM(m_bPressureRR);
}

void can_msg_tpms_t::setPressureRR(float fPressure_bar)
{
	m_bPressureRR = PRESSURE_CONVERT_TO(fPressure_bar);
}

bool can_msg_tpms_t::tireAlarmFL()
{
	return m_bmFlags & TPMS_ALARM_TIRE_FL;
}

void can_msg_tpms_t::setTireAlarmFL(bool xEnable)
{
	UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_FL);
}

bool can_msg_tpms_t::tireAlarmFR()
{
	return m_bmFlags & TPMS_ALARM_TIRE_FR;
}

void can_msg_tpms_t::setTireAlarmFR(bool xEnable)
{
	UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_FR);
}

bool can_msg_tpms_t::tireAlarmRL()
{
	return m_bmFlags & TPMS_ALARM_TIRE_RL;
}

void can_msg_tpms_t::setTireAlarmRL(bool xEnable)
{
	UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_RL);
}

bool can_msg_tpms_t::tireAlarmRR()
{
	return m_bmFlags & TPMS_ALARM_TIRE_RR;
}

void can_msg_tpms_t::setTireAlarmRR(bool xEnable)
{
	UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_TIRE_RR);
}

bool can_msg_tpms_t::alarmContinuous()
{
	return m_bmFlags & TPMS_ALARM_CONTINUOIS;
}

void can_msg_tpms_t::setAlarmContinuous(bool xEnable)
{
	UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_CONTINUOIS);
}

bool can_msg_tpms_t::alarmStrobe()
{
	return m_bmFlags & TPMS_ALARM_STROBE;
}

void can_msg_tpms_t::setAlarmStrobe(bool xEnable)
{
	UPDATE_BIT(m_bmFlags, xEnable, TPMS_ALARM_STROBE);
}
