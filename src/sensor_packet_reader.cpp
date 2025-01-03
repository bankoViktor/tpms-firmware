/**
 ********************************************************************************
 * @file    sensor_packet_reader.cpp
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    03.01.2025
 * @brief   Source file of the Sensor reader class.
 ********************************************************************************
 */

#include "sensor_packet_reader.h"

SensorPacketReader::SensorPacketReader(const uint8_t *pabData)
    : m_pabData(pabData)
{
}

uint32_t SensorPacketReader::getSensorId() const
{
    uint32_t dwResult = 0;
    dwResult |= m_pabData[0] << 24;
    dwResult |= m_pabData[1] << 16;
    dwResult |= m_pabData[2] << 8;
    dwResult |= m_pabData[3];
    return dwResult;
}

float SensorPacketReader::getPressureKPa() const
{
    return (float)m_pabData[5] * PRESSURE_KPA_FACTOR;
}

float SensorPacketReader::getPressureBar() const
{
    return getPressureKPa() / 100.0;
}

inline float SensorPacketReader::getPressurePsi() const
{
    return getPressureKPa() * 0.1450377;
}

float SensorPacketReader::getTemperatureC() const
{
    return (float)m_pabData[6] + TEMPERATURE_C_OFFSET;
}

inline float SensorPacketReader::getTemperatureF() const
{
    return getTemperatureC() * 9.0 / 5.0 + 32;
}

inline float SensorPacketReader::getTemperatureK() const
{
    return getTemperatureC() + 273.15;
}

inline uint8_t SensorPacketReader::getCrc() const
{
    return m_pabData[8];
}

bool SensorPacketReader::checkCrc() const
{
    uint8_t bCalculatedCrc = crc8(m_pabData, 8, CRC_POLY, CRC_INIT, 0);
    return bCalculatedCrc == getCrc();
}

uint8_t SensorPacketReader::getFlags() const
{
    return (m_pabData[4] & 0xF0) >> 4;
}

uint8_t SensorPacketReader::getPacketNumber() const
{
    return m_pabData[4] & 0x0F;
}

uint8_t SensorPacketReader::getUnknownByte() const
{
    return m_pabData[7];
}
