/**
 ********************************************************************************
 * @file    task_uhf_receiver.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    09.01.2025
 * @brief   Source file of the UHF receiver task.
 ********************************************************************************
 */

 // CC1101 has the following connections:
 //                               +---+---+
 //                           GND | 1 | 2 | VCC
 //          D15 <----       GDO0 | 3 | 4 | CS    ----> D5 (VSPI)
 //   (VSPI) D18 <----        CLK | 5 | 6 | MOSI  ----> D23 (VSPI)
 //   (VSPI) D19 <----  MISO/GDO1 | 7 | 8 | GDO2
 //                               +---+---+

#include "tasks/task_uhf_receiver.h"
#include "cc1101.h"
#include "app.h"
// ESP IDF
#include <esp_log.h>
#include <driver/spi_master.h>
// RTOS
#include <freertos/FreeRTOS.h>


#define CC1101_SPI_CS       5
#define CC1101_IRQ          15


/// @brief Task context.
typedef struct uhf_receiver_context_t {
    cc1101_handle_t handle1;    /// @brief Handle of the CC1101.
    cc1101_handle_t handle2;    /// @brief Handle of the CC1101.
} uhf_receiver_context_t;


/// @brief Task context single instance.
static uhf_receiver_context_t s_context;

/// CC1101 registers init array.
#include "cc1101_configuration"

/// @brief Current unit ESP log tag.
static const char *TAG = "uhf_receiver";


//interrupt setting
// gpio_config_t io_conf;
// //interrupt of falling edge
// io_conf.intr_type = GPIO_INTR_NEGEDGE; // GPIO interrupt type : falling edge
// //bit mask of the pins
// io_conf.pin_bit_mask = 1ULL<<CONFIG_GDO0_GPIO;
// //set as input mode
// io_conf.mode = GPIO_MODE_INPUT;
// //enable pull-up mode
// io_conf.pull_up_en = 1;
// io_conf.pull_down_en = 0;
// gpio_config(&io_conf);
// //install gpio isr service
// gpio_install_isr_service(0);
// //hook isr handler for specific gpio pin
// gpio_isr_handler_add(CONFIG_GDO0_GPIO, gpio_isr_handler, (void*) CONFIG_GDO0_GPIO);

/// @brief Task entry point.
void task_uhf_receiver(void *parameter)
{
    g_app_flags |= APP_FLAG_UHF_TASK_RUNNING;
    ESP_LOGI(TAG, "Task started");

    esp_err_t ret = ESP_OK;

    cc1101_config_t cfg = {
        .spi_host = APP_SPI_HOST,
        .spi_so_pin = APP_SPI_MISO,
    };

    // Module #1
    cfg.spi_cs_pin = CC1101_SPI_CS;
    ret = cc1101_init(&cfg, &s_context.handle1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Init #1 fail (%i)", ret);
    }

    // Module #2
    // cfg.spi_cs_pin = CC1101_SPI_CS + 1;
    // ret = cc1101_init(&cfg, &s_context.handle2);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Init #2 fail (%i)", ret);
    // }

    for (;;)
    {
        // waiting bit
        // read
        // re-start receiving

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
    g_app_flags &= ~APP_FLAG_UHF_TASK_RUNNING;
    ESP_LOGI(TAG, "Task stoped");
}
