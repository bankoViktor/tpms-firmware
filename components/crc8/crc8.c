/**
 ********************************************************************************
 * @file    crc8.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.01.2025
 * @brief   Source file of the CRC8 module.
 ********************************************************************************
 */

#include "crc8.h"

uint8_t crc8_calculate(const uint8_t *data, uint32_t length, uint8_t poly,
                       uint8_t init, uint8_t xor_output) {
  uint8_t result = init;

  for (uint32_t i = 0; i < length; ++i) {
    result ^= data[i];
    for (uint8_t j = 0; j < 8; ++j) {
      if (result & 0x80) {
        result = (result << 1) ^ poly;
      } else {
        result <<= 1;
      }
      result &= 0xFF;
    }
  }

  return result ^ xor_output;
}