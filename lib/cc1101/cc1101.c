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
#include <esp_log.h>
#include <esp_check.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
// RTOS
#include <freertos/task.h>
// C Runtime
#include <memory.h>


#define ESP_RETURN_ON_ERR(x) do { esp_err_t err_rc_ = (x); if (err_rc_ != ESP_OK) { return err_rc_; } } while(0)

#define CC1101_BURST                0x40
#define CC1101_READ                 0x80
#define CC1101_WRITE                0x00
#define CC1101_IS_RO(addr)          (addr >= 0x30 && addr <= 0x3D)


/// @brief Context of the CC1101.
typedef struct cc1101_context_t {
    cc1101_config_t config;         /// @brief Configurations structure for CC1101.
    spi_device_handle_t dev_handle; /// @brief CC1101 SPI device handle.
    SemaphoreHandle_t sem_so_pin;   /// @brief Handle of the semaphore for MISO pin waitings.
    SemaphoreHandle_t sem_gdo_pin;  /// @brief Handle of the semaphore for GDO pin.
    uint8_t status;                 /// @brief Status byte. Updating per each SPI transaction.
} cc1101_context_t;


static const char *TAG = "cc1101";


// static IRAM_ATTR void isr_gdo_callback(void *arg)
// {
//     assert(arg != NULL);
//     cc1101_context_t *context = (cc1101_context_t *)arg;
//     BaseType_t xHigherPriorityTaskWoken;
//     xSemaphoreGiveFromISR(context->sem_gdo_pin, &xHigherPriorityTaskWoken);
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }

static esp_err_t spi_transaction(cc1101_context_t *context, uint8_t value, uint8_t *out_value)
{
    if (context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t tr = {
        .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
        .length = 8,
        .tx_data = { value },
    };
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_ERROR(spi_device_polling_transmit(context->dev_handle, &tr), TAG, "SPI transmit fail (0x%X)", err_rc_);

    if (out_value != NULL && ret == ESP_OK)
    {
        *out_value = tr.rx_data[0];
    }

    ESP_LOGD(TAG, "SPI transaction TX:%02X RX:%02X", tr.tx_data[0], tr.rx_data[0]);

    return ret;
}

static esp_err_t spi_select(cc1101_context_t *context)
{
    if (context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    // Set CS to low
    ESP_RETURN_ON_ERROR(gpio_set_level(context->config.spi_cs_pin, 0), TAG, "Set CS pin to low fail (%X)", err_rc_);

    // Wait unlil SO is low
    uint8_t try_counter = 0;
    while (gpio_get_level(context->config.spi_so_pin))
    {
        ESP_LOGV(TAG, "check SO state in loop, try %i", try_counter + 1);
        try_counter++;
        vTaskDelay(pdMS_TO_TICKS(1));

        if (try_counter >= 5)
        {
            esp_err_t sl_ret = gpio_set_level(context->config.spi_cs_pin, 1);
            if (sl_ret != ESP_OK)
            {
                ESP_LOGE(TAG, "CS reset to high fail (0x%X)", sl_ret);
            }

            ESP_LOGE(TAG, "Wait SO to low timeout fail");
            return ESP_ERR_TIMEOUT;
        }
    }

    ESP_LOGV(TAG, "Selected device");

    return ret;
}

static esp_err_t spi_deselect(cc1101_context_t *context)
{
    if (context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(gpio_set_level(context->config.spi_cs_pin, 1), TAG, "Deselect device fail (0x%X)", err_rc_);
    ESP_LOGV(TAG, "Deselected device");

    return ESP_OK;
}

esp_err_t cc1101_write(cc1101_context_t *context, cc1101_addr_t addr, const uint8_t *buffer, uint8_t length)
{
    if (context == NULL ||
        CC1101_IS_RO(addr))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERR(spi_select(context));

    uint8_t header = CC1101_WRITE | addr;

    if (length > 1)
    {
        header |= CC1101_BURST;
    }

    esp_err_t ret = spi_transaction(context, header, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI Write: TR-1 fail (0x%X)", ret);
    }
    else
    {
        for (uint8_t i = 0; i < length; i++)
        {
            ret = spi_transaction(context, buffer[i], &context->status);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "SPI Write: TR-2 fail [%i]=0x%02X (0x%X)", i, buffer[i], ret);
                break;
            }
        }
    }

    esp_err_t s_ret = spi_deselect(context);
    if (s_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI Read: select fail (0x%X)", s_ret);
    }

    ESP_LOGI(TAG, "SPI wrote %i bytes:", length);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, length, ESP_LOG_INFO);

    return ret;
}

esp_err_t cc1101_read(cc1101_context_t *context, cc1101_addr_t addr, uint8_t *out_buffer, uint8_t length)
{
    if (context == NULL ||
        out_buffer == NULL ||
        length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERR(spi_select(context));

    uint8_t header = CC1101_READ | addr;
    if (length > 1 || CC1101_IS_RO(addr))
    {
        header |= CC1101_BURST;
    }

    esp_err_t ret = spi_transaction(context, header, &context->status);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI Read: TR-1 fail (0x%X)", ret);
    }
    else
    {
        for (uint8_t i = 0; i < length; i++)
        {
            ret = spi_transaction(context, 0x00, out_buffer + i);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "SPI Read: TR-2 fail [%i]=0x%02X (0x%X)", i, out_buffer[i], ret);
                break;
            }
        }
    }

    esp_err_t ds_ret = spi_deselect(context);
    if (ds_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI Read: deselect fail (0x%X)", ds_ret);
    }

    ESP_LOGI(TAG, "SPI readed %i bytes:", length);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, out_buffer, length, ESP_LOG_INFO);

    return ret;
}

esp_err_t cc1101_strobe(cc1101_context_t *context, cc1101_cmd_t command, uint8_t rw_flag)
{
    if (context == NULL || !CC1101_IS_RO(command))
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERR(spi_select(context));

    uint8_t header = rw_flag | command;
    esp_err_t ret = spi_transaction(context, header, &context->status);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI Strobe: TR fail (0x%X)", ret);
    }

    spi_deselect(context);

    ESP_LOGI(TAG, "SPI (0x%02X) strobe", command);

    return ret;
}

// static esp_err_t config_isr(cc1101_context_t *context, uint32_t pin_num, SemaphoreHandle_t *out_sem_handle)
// {
//     // Create semaphore
//     context->sem_so_pin = xSemaphoreCreateBinary();
//     if (context->sem_so_pin == NULL)
//     {
//         ESP_LOGE(TAG, "Can't allocate semaphore");
//         return ESP_ERR_NO_MEM;
//     }
//     ESP_LOGV(TAG, "Created semaphore");

//     // Config ISR type
//     ESP_RETURN_ON_ERR(gpio_set_intr_type(context->config.spi_so_pin, GPIO_INTR_NEGEDGE));
//     ESP_LOGV(TAG, "Configured ISR type");

//     // Add ISR handler
//     ESP_RETURN_ON_ERR(gpio_isr_handler_add(context->config.spi_so_pin, isr_so_callback, context));
//     ESP_LOGV(TAG, "Added ISR handler");

//     // Disable ISR
//     ESP_RETURN_ON_ERR(gpio_intr_disable(context->config.spi_so_pin));
//     ESP_LOGV(TAG, "Disable ISR");

//     return ESP_OK;
// }

esp_err_t cc1101_init(const cc1101_config_t *config, cc1101_context_t **out_context)
{
    if (config == NULL ||
        config->spi_host == 0 ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->spi_cs_pin) ||
        out_context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGV(TAG, "Start initializing");

    esp_err_t ret = ESP_OK;

    // Create context
    cc1101_context_t *context = (cc1101_context_t *)malloc(sizeof(cc1101_context_t));
    if (context == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        ESP_LOGE(TAG, "Can't allocate context (0x%X)", ret);
        return ret;
    }
    memset(context, 0, sizeof(*context));
    ESP_LOGV(TAG, "Allocated memory for context");

    // Copy config to context
    memcpy(&context->config, config, sizeof(cc1101_config_t));
    ESP_LOGV(TAG, "Copied config to context");

    // Config SPI device and got handle
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = CC1101_SPI_CLOCK_SPEED_HZ,
        .spics_io_num = -1,
        .queue_size = 8,
    };
    ESP_GOTO_ON_ERROR(spi_bus_add_device(context->config.spi_host, &devcfg, &context->dev_handle), cleanup, TAG, "SPI bus add device (0x%X)", err_rc_);
    ESP_LOGV(TAG, "Added device to SPI bus");

    *out_context = context;

    ESP_LOGI(TAG, "Init completed");
    return ret;

cleanup:
    esp_err_t di_ret = cc1101_deinit(context);
    if (di_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Deinit fail (0x%X)", di_ret);
    }
    return ret;
}

esp_err_t cc1101_deinit(cc1101_context_t *context)
{
    if (context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Deinit start");

    // Remove SPI device
    if (context->dev_handle != NULL)
    {
        ESP_ERROR_CHECK(spi_bus_remove_device(context->dev_handle));
        ESP_LOGV(TAG, "Remove device from bus");
        context->dev_handle = NULL;
    }

    // Delete context
    free(context);

    ESP_LOGI(TAG, "Deinit completed");
    return ESP_OK;
}

esp_err_t cc1101_set_config(cc1101_context_t *context, const cc1101_register_init_t *reg_inits, uint8_t length)
{
    if (context == NULL || reg_inits == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGV(TAG, "Setting configuration (%i regs)", length);

    esp_err_t ret = ESP_OK;

    for (size_t i = 0; i < length; i++)
    {
        const cc1101_register_init_t *reg_init = &reg_inits[i];

        ret = cc1101_write(context, reg_init->addr, &reg_init->value, 1);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Can't write: addr 0x%02X, value 0x%02X (0x%X)", reg_init->addr, reg_init->value, ret);
            return ret;
        }

        // if (reg_init->addr == CC1101_ADDR_IOCFG0 && reg_init->value & 0x01)
        // {
        //     if (config_isr(context, context->config.spi_gdo_pin, &context->sem_gdo_pin) != ESP_OK)
        //     {
        //         ESP_LOGE(TAG, "Config ISR for pin fail");
        //     }
        //     else
        //     {
        //         ESP_LOGV(TAG, "Config ISR for pin");
        //     }
        // }
    }

    ESP_LOGI(TAG, "Setted configuration (%i regs wrote)", length);

    return ret;
}

inline esp_err_t cc1101_set_idle_mode(cc1101_handle_t handle)
{
    return cc1101_strobe(handle, CC1101_CMD_SIDLE, 0);
}

inline esp_err_t cc1101_set_rx_mode(cc1101_handle_t handle)
{
    return cc1101_strobe(handle, CC1101_CMD_SRX, 0);
}

inline esp_err_t cc1101_set_tx_mode(cc1101_handle_t handle)
{
    return cc1101_strobe(handle, CC1101_CMD_STX, 0);
}
