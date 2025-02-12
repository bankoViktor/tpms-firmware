/**
 ********************************************************************************
 * @file    cc1101.h
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.01.2025
 * @brief   Header file of the CC1101 module.
 ********************************************************************************
 * CC1101 module pinouts:
 *
 *            +-------+
 *        GND | 1   2 | 3V3
 *       GDO0 | 3   4 | CSN
 *        SCK | 5   6 | MOSI
 *  MISO/GDO1 | 7   8 | GDO2 (not used)
 *            +-------+
 *
 * Implemented features:
 * - Receiving fixed packet length support only.
 * - Received packet notification via external interrupt
 *   (GDO0 pin of the module).
 * - Non-blocked cc1101_read_packet function.
 *
 * Call gpio_install_isr_service(..) before cc1101_init(..)
 *
 * Example:
 *   gpio_install_isr_service(..)
 *   cc1101_init(..);
 *   cc1101_set_config(..);
 *   cc1101_set_rx_mode(..)
 */

#ifndef CC1101__H
#define CC1101__H

#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <stdint.h>

/**
 * For override configuration defines just define need macro before inculding:
 *
 * #define CC1101_SPI_CLOCK_SPEED_HZ 2500000 // for 2.5MHz
 * #include <cc1101.h>
 *
 */

/// default 5MHz, maximum 10Mhz
#ifndef CC1101_SPI_CLOCK_SPEED_HZ
#define CC1101_SPI_CLOCK_SPEED_HZ (5000000)
#endif

#ifndef CC1101_SPI_MOSI_READY_TIMEOUT
#define CC1101_SPI_MOSI_READY_TIMEOUT pdMS_TO_TICKS(100)
#endif

#ifndef CC1101_SPI_TAKE_MUTEX_TIMEOUT
#define CC1101_SPI_TAKE_MUTEX_TIMEOUT pdMS_TO_TICKS(2000)
#endif

// define equal 0 before include this file for disable write checking
#ifndef CC1101_SPI_WRITE_CHECING
#define CC1101_SPI_WRITE_CHECING 1
#endif

/// @brief Register init data of the CC1101.
typedef struct cc1101_register_init_t {
  uint8_t addr;  /// @brief Register address.
  uint8_t value; /// @brief Register init value.
} cc1101_register_init_t;

/// @brief Handle of the CC1101.
typedef struct cc1101_context_t *cc1101_handle_t;

/// @brief Configurations of the CC1101.
typedef struct cc1101_config_t {
  spi_host_device_t spi_host;  /// @brief SPI host.
  int32_t cs_io_num;           /// @brief SPI CS pin number.
  int32_t miso_io_num;         /// @brief SPI MISO pin number.
  int32_t gdo0_io_num;         /// @brief СС1101 GDO0 pin number.
  SemaphoreHandle_t spi_mutex; /// @brief Mutex handle for SPI. Optional.
  void *user_data;             /// @brief User data. Optional.
} cc1101_config_t;

/// @brief Register adresses of the CC1101.
typedef enum cc1101_addr_t {

  // 29.1 Configuration Register Details
  //
  // Registers with preserved values in SLEEP state
  // https://www.ti.com/lit/ds/symlink/cc1101.pdf#page=71

  // GDOx Output Pin Config Table
  // https://www.ti.com/lit/ds/symlink/cc1101.pdf#page=62

  CC1101_ADDR_IOCFG2 = 0x00,   // [def 0x29] GDO2 Output Pin Config
  CC1101_ADDR_IOCFG1 = 0x01,   // [def 0x2E] GDO1 Output Pin Config
  CC1101_ADDR_IOCFG0 = 0x02,   // [def 0x3F] GDO0 Output Pin Config
  CC1101_ADDR_FIFOTHR = 0x03,  // [def 0x07] RX FIFO and TX FIFO Thresholds
  CC1101_ADDR_SYNC1 = 0x04,    // [def 0xD3] Sync Word, High Byte
  CC1101_ADDR_SYNC0 = 0x05,    // [def 0x91] Sync Word, Low Byte
  CC1101_ADDR_PKTLEN = 0x06,   // [def 0xFF] Packet Length
  CC1101_ADDR_PKTCTRL1 = 0x07, // [def 0x04] Packet Automation Control
  CC1101_ADDR_PKTCTRL0 = 0x08, // [def 0x45] Packet Automation Control
  CC1101_ADDR_ADDR = 0x09,     // [def 0x00] Device Address
  CC1101_ADDR_CHANNR = 0x0A,   // [def 0x00] Channel Number
  CC1101_ADDR_FSCTRL1 = 0x0B,  // [def 0x0F] Frequency Synthesizer Control
  CC1101_ADDR_FSCTRL0 = 0x0C,  // [def 0x00] Frequency Synthesizer Control
  CC1101_ADDR_FREQ2 = 0x0D,    // [def 0x1E] Frequency Control Word, High Byte
  CC1101_ADDR_FREQ1 = 0x0E,    // [def 0xC4] Frequency Control Word, Middle Byte
  CC1101_ADDR_FREQ0 = 0x0F,    // [def 0xEC] Frequency Control Word, Low Byte
  CC1101_ADDR_MDMCFG4 = 0x10,  // [def 0x8C] Modem Configuration
  CC1101_ADDR_MDMCFG3 = 0x11,  // [def 0x22] Modem Configuration
  CC1101_ADDR_MDMCFG2 = 0x12,  // [def 0x02] Modem Configuration
  CC1101_ADDR_MDMCFG1 = 0x13,  // [def 0x22] Modem Configuration
  CC1101_ADDR_MDMCFG0 = 0x14,  // [def 0xF8] Modem Configuration
  CC1101_ADDR_DEVIATN = 0x15,  // [def 0x47] Modem Deviation Setting
  CC1101_ADDR_MCSM2 = 0x16,    // [def 0x07] Main Control State Machine Config
  CC1101_ADDR_MCSM1 = 0x17,    // [def 0x30] Main Control State Machine Config
  CC1101_ADDR_MCSM0 = 0x18,    // [def 0x04] Main Control State Machine Config
  CC1101_ADDR_FOCCFG = 0x19,   // [def 0x36] Freq Offset Compensation Config
  CC1101_ADDR_BSCFG = 0x1A,    // [def 0x6C] Bit Synchronization Configuration
  CC1101_ADDR_AGCCTRL2 = 0x1B, // [def 0x03] AGC Control
  CC1101_ADDR_AGCCTRL1 = 0x1C, // [def 0x40] AGC Control
  CC1101_ADDR_AGCCTRL0 = 0x1D, // [def 0x91] AGC Control
  CC1101_ADDR_WOREVT1 = 0x1E,  // [def 0x87] High Byte Event0 Timeout
  CC1101_ADDR_WOREVT0 = 0x1F,  // [def 0x6B] Low Byte Event0 Timeout
  CC1101_ADDR_WORCTRL = 0x20,  // [def 0xF8] Wake On Radio Control
  CC1101_ADDR_FREND1 = 0x21,   // [def 0x56] Front End RX Configuration
  CC1101_ADDR_FREND0 = 0x22,   // [def 0x10] Front End TX Configuration
  CC1101_ADDR_FSCAL3 = 0x23,   // [def 0xA9] Frequency Synthesizer Calibration
  CC1101_ADDR_FSCAL2 = 0x24,   // [def 0x0A] Frequency Synthesizer Calibration
  CC1101_ADDR_FSCAL1 = 0x25,   // [def 0x20] Frequency Synthesizer Calibration
  CC1101_ADDR_FSCAL0 = 0x26,   // [def 0x0D] Frequency Synthesizer Calibration
  CC1101_ADDR_RCCTRL1 = 0x27,  // [def 0x41] RC Oscillator Configuration
  CC1101_ADDR_RCCTRL0 = 0x28,  // [def 0x00] RC Oscillator Configuration

  // 29.2 Configuration Register Details.
  //
  // Registers that Loose Programming in SLEEP State.
  // https://www.ti.com/lit/ds/symlink/cc1101.pdf#page=91

  CC1101_ADDR_FSTEST = 0x29,  // [def 0x59] Freq Synthesizer Calibration Control
  CC1101_ADDR_PTEST = 0x2A,   // [def 0x7F] Production Test
  CC1101_ADDR_AGCTEST = 0x2B, // [def 0x3F] AGC Test
  CC1101_ADDR_TEST2 = 0x2C,   // [def 0x88] Various Test Settings
  CC1101_ADDR_TEST1 = 0x2D,   // [def 0x31] Various Test Settings
  CC1101_ADDR_TEST0 = 0x2E,   // [def 0x0B] Various Test Settings

  /// 29.3 Status Register Details
  //
  // https://www.ti.com/lit/ds/symlink/cc1101.pdf#page=92

  CC1101_ADDR_PARTNUM = 0x30,    // [def 0x00] Chip ID - Chip part number
  CC1101_ADDR_VERSION = 0x31,    // [def 0x14] Chip ID - Chip version number
  CC1101_ADDR_FREQEST = 0x32,    // Freq Offset Estimate from Demodul-r
  CC1101_ADDR_LQI = 0x33,        // Demodul-r Estimate for Link Quality
  CC1101_ADDR_RSSI = 0x34,       // Received Signal Strength Indication
  CC1101_ADDR_MARCSTATE = 0x35,  // Main Control State Machine State
  CC1101_ADDR_WORTIME1 = 0x36,   // High Byte of WOR Time
  CC1101_ADDR_WORTIME0 = 0x37,   // Low Byte of WOR Time
  CC1101_ADDR_PKTSTATUS = 0x38,  // Current GDOx and Packet Status
  CC1101_ADDR_VCO_VC_DAC = 0x39, // Current Setting from PLL Calibration
  CC1101_ADDR_TXBYTES = 0x3A,    // Underflow and Number of Bytes
  CC1101_ADDR_RXBYTES = 0x3B,    // Overflow and Number of Bytes
  CC1101_ADDR_RCCTRL1_ST = 0x3C, // Last RC Oscillator Calibr-n Result
  CC1101_ADDR_RCCTRL0_ST = 0x3D, // Last RC Oscillator Calibr-n Result

  /// Others

  CC1101_ADDR_PATABLE = 0x3E, // PATABLE
  CC1101_ADDR_FIFO = 0x3F,    // TX/RX FIFO
} cc1101_addr_t;

/// @brief Commands of the CC1101.
typedef enum cc1101_cmd_t {
  /// @brief Reset chip.
  CC1101_CMD_SRES = 0x30,
  /// @brief Enable and calibrate frequency synthesizer.
  CC1101_CMD_SFSTXON = 0x31,
  /// @brief Turn off crystal oscillator.
  CC1101_CMD_SXOFF = 0x32,
  /// @brief Calibrate frequency synthesizer and turn it off.
  CC1101_CMD_SCAL = 0x33,
  /// @brief Enable RX.
  CC1101_CMD_SRX = 0x34,
  /// @brief In IDLE state: Enable TX. If in RX state and
  /// CCA is enabled: Only go to TX if channel is clear.
  CC1101_CMD_STX = 0x35,
  /// @brief Exit RX / TX, turn off frequency synthesizer and exit
  /// Wake-On-Radio mode if applicable.
  CC1101_CMD_SIDLE = 0x36,
  /// @brief Start automatic RX polling sequence (Wake-on-Radio).
  CC1101_CMD_SWOR = 0x38,
  /// @brief Enter power down mode when CSn goes high.
  CC1101_CMD_SPWD = 0x39,
  /// @brief Flush the RX FIFO buffer.
  CC1101_CMD_SFRX = 0x3A,
  /// @brief Flush the TX FIFO buffer.
  CC1101_CMD_SFTX = 0x3B,
  /// @brief Reset real time clock to Event1 value.
  CC1101_CMD_SWORRST = 0x3C,
  /// @brief No operation. May be used to get access to
  /// the chip status byte.
  CC1101_CMD_SNOP = 0x3D,
} cc1101_cmd_t;

/// @brief Initialize of the CC1101.
/// @param config Configurations CC1101.
/// @param out_handle Output handle of the CC1101.
/// @returns Status code.
esp_err_t cc1101_init(const cc1101_config_t *config,
                      cc1101_handle_t *out_handle);

/// @brief Deinitialize and free resources.
/// @param out_handle Handle of the CC1101.
/// @returns Status code.
esp_err_t cc1101_deinit(cc1101_handle_t handle);

/// @brief Write byte to the CC1101.
/// @param handle Handle of the CC1101.
/// @param addr Address byte.
/// @param buffer Buffer of the data for write.
/// @param length Length of the buffer.
/// @return Status code.
esp_err_t cc1101_write(cc1101_handle_t handle, cc1101_addr_t addr,
                       const uint8_t *buffer, uint8_t length);

/// @brief Read byte from the CC1101.
/// @param handle Handle of the CC1101.
/// @param addr Address byte.
/// @param out_buffer Output buffer of the data for read.
/// @param length Length of the buffer.
/// @return Status code.
esp_err_t cc1101_read(cc1101_handle_t handle, cc1101_addr_t addr,
                      uint8_t *out_buffer, uint8_t length);

/// @brief Write strobe command to the CC1101.
/// @param handle Handle of the CC1101.
/// @param command Command.
/// @param read_flag Read flag.
/// @return Status code.
esp_err_t cc1101_strobe(cc1101_handle_t handle, cc1101_cmd_t command,
                        uint8_t read_flag);

/// @brief Set configurations of the CC1101.
/// @param handle Handle of the CC1101.
/// @param reg_inits Array of the register init items for CC1101.
/// @param length Length of the array of the register init items for CC1101.
/// @returns Status code.
esp_err_t cc1101_set_config(cc1101_handle_t handle,
                            const cc1101_register_init_t *reg_inits,
                            uint8_t length);

/// @brief Set IDLE mode.
/// @param handle Handle of the CC1101.
/// @return Status code.
esp_err_t cc1101_set_idle_mode(cc1101_handle_t handle);

/// @brief Set receiving mode.
/// @param handle Handle of the CC1101.
/// @return Status code.
esp_err_t cc1101_set_rx_mode(cc1101_handle_t handle);

/// @brief Software reset of the chip.
/// @param handle Handle of the CC1101.
/// @return Status code.
esp_err_t cc1101_sw_reset(cc1101_handle_t handle);

/// @brief Read recieved packet to specify buffer if a packet avaliable.
///
/// Non-blocked function.
///
/// If a packet didn't received then the function immediately
/// back control (the length parameter will equal to 0), else it will be read
/// the received packet from the module and then back control (the length
/// parameter will equal to packet length).
/// @param handle Handle of the CC1101.
/// @param data_out Output buffer for recieved packet.
/// @param length Length of the output buffer.
/// @param rssi_dbm_out Received Signal Strength Indicator in dBm: -100 dBm
/// (very weak), -50 dBm (good), 0 dBm (maximal).
/// @param lqi_out Link Quality Indicator: 0 - bad; 127 - good.
/// @return Statua code, ESP_ERR_NOT_ALLOWED if not received data, else ESP_OK.
esp_err_t cc1101_read_packet(cc1101_handle_t handle, uint8_t *data_out,
                             uint8_t length, float *rssi_dbm_out,
                             uint8_t *lqi_out);

#endif // CC1101__H
