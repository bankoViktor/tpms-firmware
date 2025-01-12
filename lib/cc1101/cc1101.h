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


/// @brief Register init data of the CC1101.
typedef struct cc1101_register_init_t {
    uint8_t reg_addr;   /// @brief Register address.
    uint8_t reg_value;  /// @brief Register init value.
} cc1101_register_init_t;

/// @brief Configurations of the CC1101.
typedef struct cc1101_config_t {
    spi_host_device_t spi_host;     /// @brief SPI host.
    uint32_t spi_cs_pin;            /// @brief SPI CS pin number.
    uint32_t spi_so_pin;            /// @brief SPI MISO pin number.
} cc1101_config_t;

/// @brief Handle of the CC1101.
typedef struct cc1101_context_t *cc1101_handle_t;


/// @brief Initialize of the CC1101.
/// @param config Configurations CC1101.
/// @param out_handle Output handle of the CC1101.
/// @returns Status code.
esp_err_t cc1101_init(const cc1101_config_t *config, cc1101_handle_t *out_handle);

/// @brief Deinitialize and free resources.
/// @param out_handle Handle of the CC1101.
/// @returns Status code.
esp_err_t cc1101_deinit(cc1101_handle_t handle);

/// @brief Software reset procedure. Execute Manual Reset method of the CC1101.
/// @param handle Handle of the CC1101.
/// @returns Status code.
//esp_err_t cc1101_reset_software(cc1101_handle_t handle);

/// @brief Set configurations of the CC1101.
/// @param handle Handle of the CC1101.
/// @param reg_inits Array of the register init items for CC1101.
/// @param length Length of the array of the register init items for CC1101.
/// @returns Status code.
//esp_err_t cc1101_set_configuration(cc1101_handle_t handle, const cc1101_register_init_t *reg_inits, uint32_t length);

#endif // CC1101__H
