/**
 ********************************************************************************
 * @file    app.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    11.01.2025
 * @brief   Header file of the application.
 ********************************************************************************
 */

#ifndef APP__H
#define APP__H

#include "app_config.h"
#include <stdint.h>


#define APP_SPI_HOST    VSPI_HOST
#define APP_SPI_MISO    19
#define APP_SPI_MOSI    23
#define APP_SPI_CLK     18


 /// @brief Application flags.
typedef enum app_flags_t
{
    APP_FLAG_SPI_OK = (1 << 0),
    APP_FLAG_UHF_TASK_RUNNING = (1 << 1),
    APP_FLAG_UHF_ADDED_TO_SPI = (1 << 2)
} app_flags_t;

/// @brief Application flags bit mask.
extern uint32_t g_app_flags;

#endif // APP__H
