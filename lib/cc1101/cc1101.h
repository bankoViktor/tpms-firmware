/**
 ********************************************************************************
 * @file    cc1101.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    11.01.2025
 * @brief   Header file of the CC1101 module.
 ********************************************************************************
 */

#ifndef CC1101__H
#define CC1101__H

#include <stdint.h>
 // ESP IDF
#include <driver/spi_master.h>


/// default 5MHz, maximum 10Mhz
#ifndef CC1101_SPI_CLOCK_SPEED_HZ
#define CC1101_SPI_CLOCK_SPEED_HZ   (1 * 1000 * 1000)
#endif

#ifndef CC1101_SPI_WAIT_MOSI_READY_TIMEOUT_TICK
#define CC1101_SPI_WAIT_MOSI_READY_TIMEOUT_TICK     pdMS_TO_TICKS(100)
#endif


/// @brief Register init data of the CC1101.
typedef struct cc1101_register_init_t {
    uint8_t addr;   /// @brief Register address.
    uint8_t value;  /// @brief Register init value.
} cc1101_register_init_t;

/// @brief Handle of the CC1101.
typedef struct cc1101_context_t *cc1101_handle_t;

/// @brief Callback of the CC1101 for GDO0 pin.
typedef void (*cc1101_rx_cb_t)(cc1101_handle_t handle, uint8_t *data, uint8_t length);

/// @brief Configurations of the CC1101.
typedef struct cc1101_config_t {
    spi_host_device_t spi_host;     /// @brief SPI host.
    int32_t spi_cs_pin;             /// @brief SPI CS pin number.
    int32_t spi_so_pin;             /// @brief SPI MISO pin number.
    int32_t gdo0_pin;               /// @brief СС1101 GDO0 pin number.
    cc1101_rx_cb_t rx_cb;           /// @brief Callback on received packet.
    void *user_data;                /// @brief User data.
} cc1101_config_t;

/// @brief Register adresses of the CC1101.
typedef enum cc1101_addr_t {
    CC1101_ADDR_IOCFG2 = 0x00,
    CC1101_ADDR_IOCFG1 = 0x01,
    CC1101_ADDR_IOCFG0 = 0x02,
    CC1101_ADDR_FIFOTHR = 0x03,
    CC1101_ADDR_SYNC1 = 0x04,
    CC1101_ADDR_SYNC0 = 0x05,
    CC1101_ADDR_PKTLEN = 0x06,
    CC1101_ADDR_PKTCTRL1 = 0x07,
    CC1101_ADDR_PKTCTRL0 = 0x08,
    CC1101_ADDR_ADDR = 0x09,
    CC1101_ADDR_CHANNR = 0x0A,
    CC1101_ADDR_FSCTRL1 = 0x0B,
    CC1101_ADDR_FSCTRL0 = 0x0C,
    CC1101_ADDR_FREQ2 = 0x0D,
    CC1101_ADDR_FREQ1 = 0x0E,
    CC1101_ADDR_FREQ0 = 0x0F,
    CC1101_ADDR_MDMCFG4 = 0x10,
    CC1101_ADDR_MDMCFG3 = 0x11,
    CC1101_ADDR_MDMCFG2 = 0x12,
    CC1101_ADDR_MDMCFG1 = 0x13,
    CC1101_ADDR_MDMCFG0 = 0x14,
    CC1101_ADDR_DEVIATN = 0x15,
    CC1101_ADDR_MCSM2 = 0x16,
    CC1101_ADDR_MCSM1 = 0x17,
    CC1101_ADDR_MCSM0 = 0x18,
    CC1101_ADDR_FOCCFG = 0x19,
    CC1101_ADDR_BSCFG = 0x1A,
    CC1101_ADDR_AGCCTRL2 = 0x1B,
    CC1101_ADDR_AGCCTRL1 = 0x1C,
    CC1101_ADDR_AGCCTRL0 = 0x1D,
    CC1101_ADDR_WOREVT1 = 0x1E,
    CC1101_ADDR_WOREVT0 = 0x1F,
    CC1101_ADDR_WORCTRL = 0x20,
    CC1101_ADDR_FREND1 = 0x21,
    CC1101_ADDR_FREND0 = 0x22,
    CC1101_ADDR_FSCAL3 = 0x23,
    CC1101_ADDR_FSCAL2 = 0x24,
    CC1101_ADDR_FSCAL1 = 0x25,
    CC1101_ADDR_FSCAL0 = 0x26,
    CC1101_ADDR_RCCTRL1 = 0x27,
    CC1101_ADDR_RCCTRL0 = 0x28,
    CC1101_ADDR_FSTEST = 0x29,
    CC1101_ADDR_PTEST = 0x2A,
    CC1101_ADDR_AGCTEST = 0x2B,
    CC1101_ADDR_TEST2 = 0x2C,
    CC1101_ADDR_TEST1 = 0x2D,
    CC1101_ADDR_TEST0 = 0x2E,
    CC1101_ADDR_PARTNUM = 0x30,
    CC1101_ADDR_VERSION = 0x31,
    CC1101_ADDR_FREQEST = 0x32,
    CC1101_ADDR_LQI = 0x33,
    CC1101_ADDR_RSSI = 0x34,
    CC1101_ADDR_MARCSTATE = 0x35,
    CC1101_ADDR_WORTIME1 = 0x36,
    CC1101_ADDR_WORTIME0 = 0x37,
    CC1101_ADDR_PKTSTATUS = 0x38,
    CC1101_ADDR_VCO_VC_DAC = 0x39,
    CC1101_ADDR_TXBYTES = 0x3A,
    CC1101_ADDR_RXBYTES = 0x3B,
    CC1101_ADDR_RCCTRL1_STATUS = 0x3C,
    CC1101_ADDR_RCCTRL0_STATUS = 0x3D,
    CC1101_ADDR_PATABLE = 0x3E,
    CC1101_ADDR_FIFO = 0x3F,
} cc1101_addr_t;

/// @brief Commands of the CC1101.
typedef enum cc1101_cmd_t {
    CC1101_CMD_SRES = 0x30,             /// @brief Reset chip.
    CC1101_CMD_SFSTXON = 0x31,          /// @brief Enable and calibrate frequency synthesizer.
    CC1101_CMD_SXOFF = 0x32,            /// @brief Turn off crystal oscillator.
    CC1101_CMD_SCAL = 0x33,             /// @brief Calibrate frequency synthesizer and turn it off.
    CC1101_CMD_SRX = 0x34,              /// @brief Enable RX.
    CC1101_CMD_STX = 0x35,              /// @brief In IDLE state: Enable TX. If in RX state and CCA is enabled: Only go to TX if channel is clear.
    CC1101_CMD_SIDLE = 0x36,            /// @brief Exit RX / TX, turn off frequency synthesizer and exit Wake-On-Radio mode if applicable.
    CC1101_CMD_SWOR = 0x38,             /// @brief Start automatic RX polling sequence (Wake-on-Radio).
    CC1101_CMD_SPWD = 0x39,             /// @brief Enter power down mode when CSn goes high.
    CC1101_CMD_SFRX = 0x3A,             /// @brief Flush the RX FIFO buffer.
    CC1101_CMD_SFTX = 0x3B,             /// @brief Flush the TX FIFO buffer.
    CC1101_CMD_SWORRST = 0x3C,          /// @brief Reset real time clock to Event1 value.
    CC1101_CMD_SNOP = 0x3D,             /// @brief No operation. May be used to get access to the chip status byte.
} cc1101_cmd_t;


/// @brief Initialize of the CC1101.
/// @param config Configurations CC1101.
/// @param out_handle Output handle of the CC1101.
/// @returns Status code.
esp_err_t cc1101_init(const cc1101_config_t *config, cc1101_handle_t *out_handle);

/// @brief Deinitialize and free resources.
/// @param out_handle Handle of the CC1101.
/// @returns Status code.
esp_err_t cc1101_deinit(cc1101_handle_t handle);

/// @brief Software reset procedure. Execute Manual Reset of the CC1101.
/// @param handle Handle of the CC1101.
/// @returns Status code.
//esp_err_t cc1101_reset_software(cc1101_handle_t handle);

/// @brief Write byte to the CC1101.
/// @param handle Handle of the CC1101.
/// @param addr Address byte.
/// @param buffer Buffer of the data for write.
/// @param length Length of the buffer.
/// @return Status code.
esp_err_t cc1101_write(cc1101_handle_t handle, cc1101_addr_t addr, const uint8_t *buffer, uint8_t length);

/// @brief Read byte from the CC1101.
/// @param handle Handle of the CC1101.
/// @param addr Address byte.
/// @param out_buffer Output buffer of the data for read.
/// @param length Length of the buffer.
/// @return Status code.
esp_err_t cc1101_read(cc1101_handle_t handle, cc1101_addr_t addr, uint8_t *out_buffer, uint8_t length);

/// @brief Write strobe command to the CC1101.
/// @param handle Handle of the CC1101.
/// @param command Command.
/// @param rw_flag Read/Write flag.
/// @return Status code.
esp_err_t cc1101_strobe(cc1101_handle_t handle, cc1101_cmd_t command, uint8_t rw_flag);

/// @brief Set configurations of the CC1101.
/// @param handle Handle of the CC1101.
/// @param reg_inits Array of the register init items for CC1101.
/// @param length Length of the array of the register init items for CC1101.
/// @returns Status code.
esp_err_t cc1101_set_config(cc1101_handle_t handle, const cc1101_register_init_t *reg_inits, uint8_t length);

/// @brief Set IDLE mode.
/// @param handle Handle of the CC1101.
/// @return Status code.
esp_err_t cc1101_set_idle_mode(cc1101_handle_t handle);

/// @brief Set receiving mode.
/// @param handle Handle of the CC1101.
/// @return Status code.
esp_err_t cc1101_set_rx_mode(cc1101_handle_t handle);

/// @brief Set transmission mode.
/// @param handle Handle of the CC1101.
/// @return Status code.
esp_err_t cc1101_set_tx_mode(cc1101_handle_t handle);

#endif // CC1101__H
