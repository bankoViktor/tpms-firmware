/**
 ********************************************************************************
 * @file    task_uhf_receiver.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    09.01.2025
 * @brief   Source file of the UHF receiver task.
 ********************************************************************************
 */

 // CC1101 has the following connections:
 //
 //                                  # 1
 //                               +---+---+
 //                           GND | 1 | 2 | VCC
 //          D15 <----       GDO0 | 3 | 4 | CS    ----> D5 (VSPI)
 //   (VSPI) D18 <----        CLK | 5 | 6 | MOSI  ----> D23 (VSPI)
 //   (VSPI) D19 <----  MISO/GDO1 | 7 | 8 | GDO2
 //                               +---+---+
 //
 //                                  # 2
 //                               +---+---+
 //                           GND | 1 | 2 | VCC
 //                          GDO0 | 3 | 4 | CS    ----> D22
 //   (VSPI) D18 <----        CLK | 5 | 6 | MOSI  ----> D23 (VSPI)
 //   (VSPI) D19 <----  MISO/GDO1 | 7 | 8 | GDO2
 //                               +---+---+

#include "tasks/task_uhf_receiver.h"
#include "app.h"
#include "app_utils.h"
#include "cc1101.h"
#include "cc1101_config.h"
// ESP IDF
#include <esp_log.h>
#include <driver/spi_master.h>
// RTOS
#include <freertos/FreeRTOS.h>


#include <driver/gpio.h>

#define CC1101_1_SPI_CS         10
#define CC1101_1_IRQ            15
#define CC1101_2_SPI_CS         9


/// @brief Task context.
typedef struct uhf_receiver_context_t {
    cc1101_handle_t handle1;    /// @brief Handle of the CC1101.
    cc1101_handle_t handle2;    /// @brief Handle of the CC1101.
} uhf_receiver_context_t;


/// @brief Task context single instance.
static uhf_receiver_context_t s_context;

/// @brief Current unit ESP log tag.
static const char *TAG = "uhf_receiver";


/// @brief Task entry point.
void task_uhf_receiver(void *parameter)
{
    ESP_LOGI(TAG, "Task started");

    esp_err_t ret = ESP_OK;

    // Config CS pin
    gpio_config_t iocfg =
    {
        .pin_bit_mask = (1ULL << CC1101_1_SPI_CS) | (1ULL << CC1101_2_SPI_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&iocfg);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "CS pins config as OUTPUT");
    }
    else
    {
        ESP_LOGE(TAG, "CS pins config fail (0x%X)", ret);
    }

    ret = gpio_set_level(CC1101_1_SPI_CS, 1);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "CS pin #1 set high");
    }
    else
    {
        ESP_LOGE(TAG, "CS pin #1 set high fail (0x%X)", ret);
    }

    ret = gpio_set_level(CC1101_2_SPI_CS, 1);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "CS pin #2 set high");
    }
    else
    {
        ESP_LOGE(TAG, "CS pin #2 set high fail (0x%X)", ret);
    }

    // CC1101
    cc1101_config_t cfg = {
        .spi_host = APP_SPI_HOST,
        .spi_so_pin = APP_SPI_MISO,
        .spi_cs_pin = CC1101_1_SPI_CS,
    };

    // Module #1
    // ret = cc1101_init(&cfg, &s_context.handle1);
    // if (ret == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "Handle #1 - %p", (void*)s_context.handle1);
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "Init #1 fail (0x%X)", ret);
    // }

    // Check Read Chip Version
    // uint8_t chip_version1;
    // ret = cc1101_read(s_context.handle1, CC1101_ADDR_VERSION, &chip_version1, 1);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Chip Version #1 read fail (0x%X)", ret);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Chip #1 Version: %i", chip_version1);
    // }

    // Set Config CC1101
    // ret = cc1101_set_config(s_context.handle1, g_cc1101_reg_inits, countof(g_cc1101_reg_inits));
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Set c1101 config fail (0x%X)", ret);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Setted c1101 config (regs wrote %i)", countof(g_cc1101_reg_inits));
    // }

    // uint8_t gdo0_cfg;
    // ret = cc1101_read(s_context.handle1, CC1101_ADDR_IOCFG0, &gdo0_cfg, 1);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "GDO0 cfg read fail (0x%X)", ret);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "GDO0 cfg: %02X (def 0x3F, target 0x06)", gdo0_cfg);
    // }

    //ESP_ERROR_CHECK(cc1101_set_rx_mode(s_context.handle1));
    //ESP_LOGI(TAG, "Setted c1101 to RX mode");

    // Module #2
    // cfg.spi_cs_pin = CC1101_2_SPI_CS;
    // ret = cc1101_init(&cfg, &s_context.handle2);
    // if (ret == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "Handle #2 - %p", (void*)s_context.handle2);
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "Init #2 fail (0x%X)", ret);
    // }

    // Check Read Chip Version
    // uint8_t chip_version2;
    // ret = cc1101_read(s_context.handle2, CC1101_ADDR_VERSION, &chip_version2, 1);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Chip Version #2 read fail (0x%X)", ret);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Chip #2 Version: %i", chip_version2);
    // }

    // Set Config CC1101
    // ret = cc1101_set_config(s_context.handle2, g_cc1101_reg_inits, countof(g_cc1101_reg_inits));
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Set c1101 #2 config fail (0x%X)", ret);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "Setted c1101 #2 config (regs wrote %i)", countof(g_cc1101_reg_inits));
    // }

    // uint8_t gdo0_cfg2_2;
    // ret = cc1101_read(s_context.handle2, CC1101_ADDR_IOCFG0, &gdo0_cfg2_2, 1);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "GDO0 cfg #2 read fail (0x%X)", ret);
    // }
    // else
    // {
    //     ESP_LOGI(TAG, "GDO0 cfg #2: %02X (def 0x3F, target 0x06)", gdo0_cfg2_2);
    // }

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
    ESP_LOGI(TAG, "Task stoped");
}
