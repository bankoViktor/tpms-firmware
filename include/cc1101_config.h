/**
 ********************************************************************************
 * @file    cc1101_config.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    09.01.2025
 * @brief   Header file of the CC1101 configuration.
 ********************************************************************************
 */

#ifndef CC1101_CONFIG__H
#define CC1101_CONFIG__H

#include "cc1101.h"


DRAM_ATTR const cc1101_register_init_t g_cc1101_reg_inits[] =
{
   { CC1101_ADDR_IOCFG2, 0x2E },    // GDO2 - High impedance (3-state)
   { CC1101_ADDR_IOCFG0, 0x06 },    // GDO0
   // Asserts: when sync word has been sent/received, and de-asserts: at the end of the packet.
   // In RX, the pin will also deassert when a packet is discarded due to address
   // or maximum length filtering or when the radio enters RXFIFO_OVERFLOW state.
   // In TX the pin will de-assert if the TX FIFO underflows.
   {CC1101_ADDR_SYNC1, 0xA9 },      // Sync Word, High Byte
   {CC1101_ADDR_SYNC0, 0x55 },      // Sync Word, Low Byte
   {CC1101_ADDR_PKTLEN, 18 },       // Packet Length 18 byte
   {CC1101_ADDR_PKTCTRL0, 0x00 },   // Whitening off, CRC disabled for TX and RX, Fixed Packet Length
   {CC1101_ADDR_FSCTRL1, 0x06 },
   {CC1101_ADDR_FREQ2, 0x10 },
   {CC1101_ADDR_FREQ1, 0xB0 },
   {CC1101_ADDR_FREQ0, 0x71 },
   {CC1101_ADDR_MDMCFG4, 0xA9 },
   {CC1101_ADDR_MDMCFG3, 0x83 },
   {CC1101_ADDR_MDMCFG2, 0x07 },
   {CC1101_ADDR_MDMCFG1, 0x00 },
   {CC1101_ADDR_DEVIATN, 0x24 },
   {CC1101_ADDR_MCSM0, 0x18 },
   {CC1101_ADDR_FOCCFG, 0x16 },
   {CC1101_ADDR_WORCTRL, 0xFB },
   {CC1101_ADDR_FSCAL3, 0xE9 },
   {CC1101_ADDR_FSCAL2, 0x2A },
   {CC1101_ADDR_FSCAL1, 0x00 },
   {CC1101_ADDR_FSCAL0, 0x1F },
   {CC1101_ADDR_PATABLE, 0x60 },
};

#endif // CC1101_CONFIG__H