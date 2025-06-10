/**
 ******************************************************************************
 * @file    manchester_codec.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    2024-11-28
 * @brief   Source file for codec of the Manchester code.
 ******************************************************************************
 */

#include "manchester_codec.h"
#include <math.h>

int16_t manchester_encode(const uint8_t *src_buff, uint32_t src_bit_count,
                          uint8_t *dst_buff, uint32_t *dsr_bit_count,
                          uint32_t bit_inverse) {
  if (src_buff == 0 || src_bit_count < 1 || dst_buff == 0) {
    return MANCHESTER_ERR_INVALID_ARGS;
  }

  if (src_buff == dst_buff) {
    return MANCHESTER_ERR_SRC_AND_DST_SAME;
  }

  uint32_t dst_bytes_min = (uint32_t)ceil((src_bit_count * 2) / 8.0);
  uint32_t dst_bytes = (uint32_t)ceil(*dsr_bit_count / 8.0);
  if (dst_bytes < dst_bytes_min) {
    return MANCHESTER_ERR_NOT_ENOUGH_DST_BUFFER_LEN;
  }

  uint32_t dst_byte_index = 0;
  uint32_t dst_bit_in_byte_index = 0;
  for (uint32_t src_bit_index = 0; src_bit_index < src_bit_count;
       src_bit_index++) {
    uint32_t src_byte_index = src_bit_index / 8;
    uint8_t src_bit_in_byte_index = 7 - (src_bit_index - src_byte_index * 8);
    uint8_t src_byte = src_buff[src_byte_index];

    // Clear bits
    dst_buff[dst_byte_index] &= ~(0b11 << (6 - dst_bit_in_byte_index));

    // Set bits
    uint8_t src_bit = (src_byte >> src_bit_in_byte_index) & 0b1;
    switch (src_bit) {
    case 0b0:
      if (bit_inverse) {
        dst_buff[dst_byte_index] |= (0b10 << (6 - dst_bit_in_byte_index));
      } else {
        dst_buff[dst_byte_index] |= (0b01 << (6 - dst_bit_in_byte_index));
      }
      break;

    case 0b1:
      if (bit_inverse) {
        dst_buff[dst_byte_index] |= (0b01 << (6 - dst_bit_in_byte_index));
      } else {
        dst_buff[dst_byte_index] |= (0b10 << (6 - dst_bit_in_byte_index));
      }
      break;

    default:
      return MANCHESTER_ERR_WRONG_BIT_COMBINATION;
    }

    dst_bit_in_byte_index += 2;
    if (dst_bit_in_byte_index >= 8) {
      dst_byte_index++;
      dst_bit_in_byte_index = 0;
    }
  }

  *dsr_bit_count = dst_byte_index * 8 + dst_bit_in_byte_index;
  return MANCHESTER_SUCCESS;
}

int16_t manchester_decode(const uint8_t *src_buff, uint32_t src_bit_count,
                          uint8_t *dst_buff, uint32_t *dst_bit_count,
                          uint32_t bit_inverse) {
  if (src_buff == 0 || src_bit_count < 2 || dst_buff == 0) {
    return MANCHESTER_ERR_INVALID_ARGS;
  }

  if (src_bit_count % 2 != 0) {
    return MANCHESTER_ERR_ODD_BIT_COUNT;
  }

  uint32_t dst_bytes_min = (uint32_t)ceil((src_bit_count / 2) / 8.0);
  uint32_t dst_bytes = (uint32_t)ceil(*dst_bit_count / 8.0);
  if (dst_bytes < dst_bytes_min) {
    return MANCHESTER_ERR_NOT_ENOUGH_DST_BUFFER_LEN;
  }

  uint32_t dst_byte_index = 0;
  uint32_t dst_bit_in_byte_index = 0;
  for (uint32_t src_bit_index = 0; src_bit_index < src_bit_count;
       src_bit_index += 2) {
    uint32_t src_byte_index = src_bit_index / 8;
    uint8_t src_byte = src_buff[src_byte_index];
    uint8_t mask = (1 << (7 - dst_bit_in_byte_index));

    // Clear bit
    dst_buff[dst_byte_index] &= ~mask;

    // Set bit
    uint8_t src_bit_in_byte_index = 8 - src_bit_index + src_byte_index * 8 - 2;
    uint8_t src_bits = (src_byte >> src_bit_in_byte_index) & 0b11;
    switch (src_bits) {
    case 0b01:
      if (bit_inverse) {
        dst_buff[dst_byte_index] |= mask;
      }
      break;

    case 0b10:
      if (!bit_inverse) {
        dst_buff[dst_byte_index] |= mask;
      }
      break;

    default:
      return MANCHESTER_ERR_WRONG_BIT_COMBINATION;
    }

    dst_bit_in_byte_index++;
    if (dst_bit_in_byte_index >= 8) {
      dst_byte_index++;
      dst_bit_in_byte_index = 0;
    }
  }

  *dst_bit_count = dst_byte_index * 8 + dst_bit_in_byte_index;
  return MANCHESTER_SUCCESS;
}
