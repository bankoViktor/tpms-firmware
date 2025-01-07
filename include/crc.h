/**
 ********************************************************************************
 * @file    crc.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    03.01.2025
 * @brief   Header file of the CRC.
 ********************************************************************************
 */

#ifndef CRC__H
#define CRC__H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Calculate CRC8
/// @param pbData Pointer to the data buffer.
/// @param nLength Count bytes of the data bytes for calculation.
/// @param bPoly Polinom byte
/// @param bInit Init byte
/// @param bXorOut XOR byte for result
/// @return Calculated CRC8 byte
uint8_t crc8(const uint8_t *pbData, size_t nLength, uint8_t bPoly, uint8_t bInit, uint8_t bXorOut);

#ifdef __cplusplus
}
#endif

#endif
