/**
 ********************************************************************************
 * @file    cc1101.c
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    11.01.2025
 * @brief   Implementation file of the CC1101 module.
 ********************************************************************************
 */

#include "cc1101.h"
 // ESP IDF
#include "esp_log.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
// RTOS
#include <freertos/task.h>
// C Runtime
#include <memory.h>


#define RET_IF_ERR(err)    if (err != ESP_OK) { return err; }


/// @brief Context of the CC1101.
typedef struct cc1101_context_t {
    cc1101_config_t config;         /// @brief Configurations structure for CC1101.
    spi_device_handle_t dev_handle; /// @brief CC1101 SPI device handle.
} cc1101_context_t;

/// @brief Transfer type for CC1101.
typedef enum cc1101_transfer_type {
    CC1101_TRANSTYPE_WRITE_SINGLE = 0x00,
    CC1101_TRANSTYPE_WRITE_BURST = 0x40,
    CC1101_TRANSTYPE_READ_SINGLE = 0x80,
    CC1101_TRANSTYPE_READ_BURST = 0xC0,
} cc1101_transfer_type;

/// @brief Register type for CC1101.
typedef enum cc1101_reg_type {
    CC1101_REGTYPE_CONFIG = CC1101_TRANSTYPE_READ_SINGLE,
    CC1101_REGTYPE_STATUS = CC1101_TRANSTYPE_READ_BURST,
} cc1101_reg_type;

/// @brief Register addresses of the CC1101.
typedef enum cc1101_reg_address {
    CC1101_REGADD_PARTNUM = 0x30,       /// @brief Part number for CC1101.
    CC1101_REGADD_VERSION = 0x31,       /// @brief Current version number.
} cc1101_reg_address;


static const char *TAG = "cc1101";


static esp_err_t cc1101_read_register(cc1101_context_t *context, uint8_t address, uint8_t *out_value)
{
    assert(context != NULL);
    assert(out_value != NULL);

    esp_err_t ret;
    *out_value = 0;

    spi_transaction_t tr = {
        .cmd = CC1101_REGTYPE_STATUS | address,
        .length = 8,
        .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
        .user = context,
    };
    ret = spi_device_polling_transmit(context->dev_handle, &tr);
    RET_IF_ERR(ret);

    *out_value = tr.rx_data[0];
    return ret;
}

static void cc1101_select(spi_transaction_t *tr)
{
    assert(tr != NULL);
    cc1101_context_t *context = (cc1101_context_t *)tr->user;
    assert(context != NULL);

    esp_err_t ret = gpio_set_level(context->config.spi_cs_pin, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "select device fail (%i)", ret);
    }
}

static void cc1101_deselect(spi_transaction_t *tr)
{
    assert(tr != NULL);
    cc1101_context_t *context = (cc1101_context_t *)tr->user;
    assert(context != NULL);

    esp_err_t ret = gpio_set_level(context->config.spi_cs_pin, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "deselect device fail (%i)", ret);
    }
}

static esp_err_t cc1101_init_gpio_device_selector(cc1101_context_t *context)
{
    gpio_config_t cfg =
    {
        .pin_bit_mask = (1ULL << context->config.spi_cs_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "CS pin config fail (%i)", ret);
        return ret;
    }

    spi_transaction_t dummy_tr = {
        .user = context,
    };
    cc1101_deselect(&dummy_tr);

    return ret;
}

static esp_err_t cc1101_reset_on_power_on(cc1101_context_t *context)
{
    assert(context != NULL);

    vTaskDelay(pdMS_TO_TICKS(5)); // by datasheet for stabilized chip power

    spi_transaction_t dummy_tr = {
        .user = context,
    };

    cc1101_select(&dummy_tr);

    // Waiting until SO to low
    while (gpio_get_level(context->config.spi_so_pin) > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    // TODO add timeout wating chip ready

    cc1101_deselect(&dummy_tr);

    ESP_LOGD(TAG, "Reset Hard");
    return ESP_OK;
}

static esp_err_t cc1101_config_spi_device(cc1101_context_t *context)
{
    assert(context != NULL);

    esp_err_t ret;

    spi_device_interface_config_t devcfg = {
        .command_bits = 8,
        .mode = 0,
        .clock_speed_hz = 5 * 1000 * 1000, // 5MHz
        .spics_io_num = -1,
        .flags = 0,
        .queue_size = 8,
        .pre_cb = cc1101_select,
        .post_cb = cc1101_deselect,
    };

    ret = spi_bus_add_device(context->config.spi_host, &devcfg, &context->dev_handle);
    RET_IF_ERR(ret);

    assert(context->dev_handle != NULL);
    return ret;
}

// esp_err_t cc1101_reset_software(cc1101_context_t *context)
// {
//     assert(context != NULL);

//     esp_err_t ret;

//     ESP_LOGD(TAG, "Reset Soft");
//     ret = ESP_ERR_NOT_SUPPORTED;

//     return ret;
// }

esp_err_t cc1101_init(const cc1101_config_t *config, cc1101_context_t **out_context)
{
    assert(config != NULL);
    assert(config->spi_host != 0);
    assert(config->spi_cs_pin > 0);
    assert(config->spi_so_pin > 0);
    assert(out_context != NULL);

    cc1101_context_t *context = (cc1101_context_t *)malloc(sizeof(cc1101_context_t));
    if (!context)
    {
        return ESP_ERR_NO_MEM;
    }
    memcpy(&context->config, config, sizeof(*config));

    esp_err_t ret;

    ret = cc1101_init_gpio_device_selector(context);
    ESP_ERROR_CHECK(ret);
    RET_IF_ERR(ret);

    ret = cc1101_reset_on_power_on(context);
    ESP_ERROR_CHECK(ret);
    RET_IF_ERR(ret);

    ret = cc1101_config_spi_device(context);
    ESP_ERROR_CHECK(ret);
    RET_IF_ERR(ret);

    // Read PATABLE
    // uint8_t ptable[8];
    // readBurstReg(ptable, CC1101_PATABLE, 8);
    // ESP_LOG_BUFFER_HEXDUMP(TAG, ptable, 8, ESP_LOG_INFO);

    // Read Chip Part Number
    uint8_t chip_partNum;
    ret = cc1101_read_register(context, CC1101_REGADD_PARTNUM, &chip_partNum);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Chip Part Number %i", chip_partNum);
    }
    else
    {
        ESP_LOGW(TAG, "Read Part Number fail (%i)", ret);
        return ESP_FAIL;
    }

    // Read Chip Version
    uint8_t chip_version;
    ret = cc1101_read_register(context, CC1101_REGADD_VERSION, &chip_version);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Chip Version %i", chip_version);
    }
    else
    {
        ESP_LOGW(TAG, "Read Chip Version fail (%i)", ret);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Init completed");

    *out_context = context;
    return ret;
}

esp_err_t cc1101_deinit(cc1101_context_t *context)
{
    assert(context != NULL);

    esp_err_t ret;

    ret = gpio_reset_pin(context->config.spi_cs_pin);
    RET_IF_ERR(ret);

    if (context->dev_handle != NULL)
    {
        ret = spi_bus_remove_device(context->dev_handle);
        if (ret != ESP_OK) return ret;

        context->dev_handle = NULL;
    }

    free(context);

    return ret;
}

// esp_err_t cc1101_set_configuration(cc1101_handle_t handle, const cc1101_register_init_t *reg_inits, uint32_t length)
// {
//     esp_err_t ret;

//     ESP_LOGI(TAG, "Set configuration");

//     return ret;
// }
