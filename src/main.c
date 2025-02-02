/**
 ********************************************************************************
 * @file    main.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    09.01.2025
 * @brief   Entry point of the application.
 ********************************************************************************
 */

#include "app.h"
#include "task_params.h"
#include "app_utils.h"
 // ESP IDF
#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
 // RTOS
#include <freertos/FreeRTOS.h>
 // Tasks
#include <tasks/task_uhf_receiver.h>


/// @brief Tasks definition array.
static task_init_params_t s_tasks_init_params[] =
{
    {"uhf_receiver", task_uhf_receiver, TASK_UHF_RECEIVER_STACK_DEPTH, NULL, TASK_UHF_RECEIVER_PRIORITY, NULL}
};

/// @brief Current unit ESP log tag.
static const char *TAG = "app";

/// @brief Application flags bit mask.
uint32_t g_app_flags;


/// @brief SPI mater configuration.
static void config_spi_master()
{
    spi_bus_config_t buscfg =
    {
        .miso_io_num = APP_SPI_MISO,
        .mosi_io_num = APP_SPI_MOSI,
        .sclk_io_num = APP_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(APP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

/// @brief Starting tasks defined in task array.
static void start_tasks()
{
    ESP_LOGV(TAG, "Starting tasks: count %i", countof(s_tasks_init_params));

    BaseType_t ret;

    for (uint8_t task_count = 0; task_count < countof(s_tasks_init_params); task_count++)
    {
        const task_init_params_t *params = &s_tasks_init_params[task_count];

        ret = xTaskCreate(
            params->func,
            params->name,
            params->stack_depth,
            params->parameter,
            params->priority,
            params->handle
        );

        if (ret == pdPASS)
        {
            ESP_LOGV(TAG, "Task started '%s'", params->name);
        }
        else
        {
            ESP_LOGE(TAG, "Could not allocate memory for task '%s' (0x%X)", params->name, ret);
        }
    }
}

/// @brief Entry point of the application.
void app_main()
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("app", ESP_LOG_VERBOSE);
    esp_log_level_set("uhf_receiver", ESP_LOG_VERBOSE);
    esp_log_level_set("cc1101", ESP_LOG_VERBOSE);

    g_app_flags = 0;

    // Install ISR service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    // Init shared hardware
    config_spi_master();

    // Start RTOS tasks
    start_tasks();
}
