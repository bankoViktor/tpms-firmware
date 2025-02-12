/**
 ********************************************************************************
 * @file    crc8.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.01.2025
 * @brief   Header file of the CRC8 module.
 ********************************************************************************
 */

#ifndef CRC8__H
#define CRC8__H

#include <stdint.h>

/// @brief Calculate CRC8
/// @param data Bytes of the data.
/// @param length Count bytes of the data bytes.
/// @param poly Polinome byte.
/// @param init Initial byte.
/// @param xor_out XOR flag for result.
/// @return Calculated CRC8 byte.
uint8_t crc8_calculate(const uint8_t *data, uint32_t length, uint8_t poly,
                       uint8_t init, uint8_t xor_out);

#endif
