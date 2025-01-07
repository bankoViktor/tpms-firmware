/**
 ********************************************************************************
 * @file    crc.cpp
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    03.01.2025
 * @brief   Source file of the CRC.
 ********************************************************************************
 */

#include "crc.h"

uint8_t crc8(const uint8_t *pbData, size_t nLength, uint8_t bPoly, uint8_t bInit, uint8_t bXorOut)
{
    uint8_t bResult = bInit;
    for (size_t i = 0; i < nLength; ++i)
    {
        bResult ^= pbData[i];
        for (uint8_t j = 0; j < 8; ++j)
        {
            if (bResult & 0x80)
            {
                bResult = (bResult << 1) ^ bPoly;
            }
            else
            {
                bResult <<= 1;
            }
            bResult &= 0xFF;
        }
    }
    return bResult ^ bXorOut;
}
