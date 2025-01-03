/**
 ********************************************************************************
 * @file    sensor_packet_reader.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    03.01.2025
 * @brief   Header file of the Sensor reader class.
 ********************************************************************************
 */

// Sensor Info:
//
// Manufacturer:    Autel
// Model:           MX-Sensor
// HW:              8306
// SW:              V6.52
// Protocol:        Chery Tiggo 4 / 06/2019 - 12/2023 (433MHz 802000121AA)
// AC/PI:           FF00/03CA
// PV/PT/MI:        3.02/26.0/B3626
// Packet scheme:   IIIIIIII FN PP TT yy CC
//                  where:
//                      I - sensor ID (32-bit)
//                      F - Flags (0xA - pressure changed, 0x9 - after programmed)
//                      N - Packet number (1-3)
//                      PP - Pressure in KPa (KPa=p*1.37)
//                      TT - Temparature in C (C=t-50)
//                      yy - Unknown (always 0x01)
//                      CC - CRC8 0-8 bytes (poly 0x07, init 0xF1)

#ifndef SENSOR_PACKET_READER__H
#define SENSOR_PACKET_READER__H

#include <stdint.h>
#include "crc.h"

/// @brief Reader class for the sensor packet buffer.
class SensorPacketReader
{
private:
    /// @brief Pointer to the packet data.
    const uint8_t *m_pabData;

    static constexpr float PRESSURE_KPA_FACTOR = 1.37;
    static constexpr int8_t TEMPERATURE_C_OFFSET = -50;
    static constexpr uint8_t CRC_POLY = 0x07;
    static constexpr uint8_t CRC_INIT = 0xF1;

public:
    /// @brief Default constructor.
    /// @param pabData Pointer to the packet data.
    SensorPacketReader(const uint8_t *pabData);

    /// @brief Returns the identifier of the transmitting sensor.
    /// @return Sensor identifier.
    uint32_t getSensorId() const;

    /// @brief Returns the pressure in KPa.
    /// @return Pressure value in KPa.
    float getPressureKPa() const;

    /// @brief Returns the pressure in Bar.
    /// @return Pressure value in Bar.
    float getPressureBar() const;

    /// @brief Returns the pressure in Psi.
    /// @return Pressure value in Psi.
    float getPressurePsi() const;

    /// @brief Returns the temperature in Celsius .
    /// @return Temperature value in Celsius .
    float getTemperatureC() const;

    /// @brief Returns the temperature in Fahrenheit.
    /// @return Temperature value in Fahrenheit.
    float getTemperatureF() const;

    /// @brief Returns the temperature in Kelvin.
    /// @return Temperature value in Kelvin.
    float getTemperatureK() const;

    /// @brief Returns the received CRC value.
    /// @return The CRC value of the received packet.
    uint8_t getCrc() const;

    /// @brief Checking of correct of the received packet CRC.
    /// @return TRUE if CRC correct.
    bool checkCrc() const;

    /// @brief Returns the flags.
    /// @return Flag value (always 0xA).
    uint8_t getFlags() const;

    /// @brief Returns the packet number.
    /// @return Packet number.
    uint8_t getPacketNumber() const;

    /// @brief Returns the unknown byte in received packet.
    /// @return Unknown byte (always 0x01).
    uint8_t getUnknownByte() const;
};

#endif
