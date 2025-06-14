/**
 ********************************************************************************
 * @file    cc1101.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.01.2025
 * @brief   Source file of the CC1101 module.
 ********************************************************************************
 */

#include "cc1101.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_check.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <memory.h>

#define ESP_RETURN_ON_ERR(x)                                                   \
  do {                                                                         \
    esp_err_t err_rc_ = (x);                                                   \
    if (err_rc_ != ESP_OK) {                                                   \
      return err_rc_;                                                          \
    }                                                                          \
  } while (0)

#define TAKE_MUTEX_OR_ERR_RETURN()                                             \
  if (context->config.spi_mutex != 0) {                                        \
    ESP_LOGV(TAG, "Take mutext");                                              \
    if (xSemaphoreTake(context->config.spi_mutex,                              \
                       CC1101_SPI_TAKE_MUTEX_TIMEOUT) == pdFAIL) {             \
      esp_err_t err_rc_ = ESP_ERR_TIMEOUT;                                     \
      ESP_LOGE(TAG, "Take Mutex timeout (%s)", esp_err_to_name(err_rc_));      \
      return err_rc_;                                                          \
    }                                                                          \
  }

#define GIVE_MUTEX()                                                           \
  if (context->config.spi_mutex != 0) {                                        \
    ESP_LOGV(TAG, "Give mutext");                                              \
    xSemaphoreGive(context->config.spi_mutex);                                 \
  }

#define CC1101_RSSI_OFFSET 74
#define CC1101_BURST 0x40
#define CC1101_READ 0x80
#define CC1101_WRITE 0x00
#define CC1101_IS_RO(addr) (addr >= 0x30 && addr <= 0x3D)

/// @brief Context of the CC1101.
typedef struct cc1101_context_t {
  cc1101_config_t config; /// @brief Configurations structure for CC1101.
  spi_device_handle_t dev_handle; /// @brief CC1101 SPI device handle.
  bool packet_received;           /// @brief Packet received flag.
  SemaphoreHandle_t sem_gdo; /// @brief Handle of the semaphore for GDO pin.
  uint8_t status; /// @brief Status byte. Updating per each SPI transaction.
} cc1101_context_t;

static const char *TAG = "cc1101";

static IRAM_ATTR void isr_handler_gdo0(void *arg) {
  assert(arg != NULL);
  cc1101_context_t *context = (cc1101_context_t *)arg;
  context->packet_received = true;
}

static esp_err_t spi_transaction(cc1101_context_t *context, uint8_t value,
                                 uint8_t *out_value) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  spi_transaction_t tr = {
      .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
      .length = 8,
      .tx_data = {value},
  };
  esp_err_t ret = ESP_OK;

  ESP_RETURN_ON_ERROR(spi_device_polling_transmit(context->dev_handle, &tr),
                      TAG, "SPI transmit fail (%s)", esp_err_to_name(err_rc_));

  if (out_value != NULL && ret == ESP_OK) {
    *out_value = tr.rx_data[0];
  }

  ESP_LOGV(TAG, "SPI tx:%02X rx:%02X", tr.tx_data[0], tr.rx_data[0]);
  return ret;
}

static esp_err_t spi_select(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = ESP_OK;

  // Set CS to low
  ESP_RETURN_ON_ERROR(gpio_set_level(context->config.cs_io_num, 0), TAG,
                      "Set CS pin to low fail (%s)", esp_err_to_name(err_rc_));

  // Wait unlil SO is low
  uint8_t try_counter = 0;
  while (gpio_get_level(context->config.miso_io_num)) {
    ESP_LOGV(TAG, "check SO state in loop, try %i", try_counter + 1);
    try_counter++;

    esp_rom_delay_us(500);

    if (try_counter >= 5) {
      esp_err_t sl_ret = gpio_set_level(context->config.cs_io_num, 1);
      if (sl_ret != ESP_OK) {
        ESP_LOGE(TAG, "CS reset to high fail (%s)", esp_err_to_name(sl_ret));
      }

      ESP_LOGE(TAG, "Wait SO to low timeout fail");
      return ESP_ERR_TIMEOUT;
    }
  }

  ESP_LOGV(TAG, "Selected device");
  return ret;
}

static esp_err_t spi_deselect(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_RETURN_ON_ERROR(gpio_set_level(context->config.cs_io_num, 1), TAG,
                      "Deselect device fail (%s)", esp_err_to_name(err_rc_));

  ESP_LOGV(TAG, "Deselected device");
  return ESP_OK;
}

static esp_err_t spi_test_connection(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t chip_ver;
  ESP_RETURN_ON_ERROR(cc1101_read(context, CC1101_ADDR_VERSION, &chip_ver, 1),
                      TAG, "Read version fail");

  if (chip_ver == 0xFF) {
    ESP_LOGE(TAG, "Invalid version");
    return ESP_FAIL;
  }

  if (chip_ver != 0x14) {
    ESP_LOGW(TAG, "Unexpected version (0x14): 0x%02X", chip_ver);
  }

  return ESP_OK;
}

static void isr_enable(cc1101_context_t *context) {
  assert(context != NULL);
  context->packet_received = 0;
  ESP_ERROR_CHECK(
      gpio_set_intr_type(context->config.gdo0_io_num, GPIO_INTR_POSEDGE));
  ESP_LOGD(TAG, "Enabled ISR");
}

static void isr_disable(cc1101_context_t *context) {
  assert(context != NULL);
  context->packet_received = 0;
  ESP_ERROR_CHECK(
      gpio_set_intr_type(context->config.gdo0_io_num, GPIO_INTR_DISABLE));
  ESP_LOGD(TAG, "Disabled ISR");
}

esp_err_t cc1101_write(cc1101_context_t *context, cc1101_addr_t addr,
                       const uint8_t *buffer, uint8_t length) {
  if (context == NULL || CC1101_IS_RO(addr) || buffer == NULL || length == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  // Select device
  esp_err_t ret = spi_select(context);
  if (ret == ESP_OK) {
    // Write Header
    uint8_t header = CC1101_WRITE | addr;
    if (length > 1) {
      header |= CC1101_BURST;
    }
    ret = spi_transaction(context, header, NULL);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "SPI Write header 0x%02X (%s)", header,
               esp_err_to_name(ret));
    } else {
      // Write data bytes
      for (uint8_t i = 0; i < length; i++) {
        ret = spi_transaction(context, buffer[i], &context->status);
        if (ret != ESP_OK) {
          ESP_LOGE(TAG, "SPI Write data fail [%i]=0x%02X (%s)", i, buffer[i],
                   esp_err_to_name(ret));
          break;
        }
      }
    }
  }

  // Deselect device
  ESP_ERROR_CHECK(spi_deselect(context));

#if CC1101_SPI_WRITE_CHECING == 1
  if (ret == ESP_OK) {
    // Allocate memory for read buffer
    uint8_t *readed_buffer = (uint8_t *)malloc(length);
    if (readed_buffer == NULL) {
      ESP_LOGE(TAG, "Allocate RX buffer fail");
      ret = ESP_ERR_NO_MEM;
    } else {
      // Read
      ret = cc1101_read(context, addr, readed_buffer, length);
      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Can't checking read (%s)", esp_err_to_name(ret));
      } else {
        // Checking
        for (uint8_t i = 0; i < length; i++) {
          if (readed_buffer[i] != buffer[i]) {
            ESP_LOGE(
                TAG,
                "Readed not same: addr 0x%02X, excepted: 0x%02X, readed 0x%02X",
                addr, buffer[i], readed_buffer[i]);
            ret = ESP_FAIL;
            break;
          }
        }
      }

      // Free memory
      free(readed_buffer);
    }
  }
#endif

  GIVE_MUTEX();

  if (ret == ESP_OK) {
    ESP_LOGD(TAG, "SPI wrote %i bytes to addr 0x%02X:", length, addr);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, length, ESP_LOG_DEBUG);
  }
  return ret;
}

esp_err_t cc1101_read(cc1101_context_t *context, cc1101_addr_t addr,
                      uint8_t *out_buffer, uint8_t length) {
  if (context == NULL || out_buffer == NULL || length == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  // Select device
  esp_err_t ret = spi_select(context);
  if (ret == ESP_OK) {
    // Write Header
    uint8_t header = CC1101_READ | addr;
    if (length > 1 || CC1101_IS_RO(addr)) {
      header |= CC1101_BURST;
    }
    ret = spi_transaction(context, header, &context->status);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "SPI Write header 0x%02X (%s)", header,
               esp_err_to_name(ret));
    } else {
      // Read Data bytes
      for (uint8_t i = 0; i < length; i++) {
        ret = spi_transaction(context, 0x00, out_buffer + i);
        if (ret != ESP_OK) {
          ESP_LOGE(TAG, "SPI Read data fail [%i]=0x%02X (%s)", i, out_buffer[i],
                   esp_err_to_name(ret));
          break;
        }
      }
    }
  }

  // Deselect device
  ESP_ERROR_CHECK(spi_deselect(context));

  GIVE_MUTEX();

  if (ret == ESP_OK) {
    ESP_LOGD(TAG, "SPI readed %i bytes from addr 0x%02X:", length, addr);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, out_buffer, length, ESP_LOG_DEBUG);
  }
  return ret;
}

esp_err_t cc1101_strobe(cc1101_context_t *context, cc1101_cmd_t command,
                        uint8_t read_flag) {
  if (context == NULL || !CC1101_IS_RO(command)) {
    return ESP_ERR_INVALID_ARG;
  }

  TAKE_MUTEX_OR_ERR_RETURN();

  // Select device
  esp_err_t ret = spi_select(context);
  if (ret == ESP_OK) {
    // Write Strobe
    uint8_t strobe = read_flag | command;
    ret = spi_transaction(context, strobe, &context->status);
    if (ret != ESP_OK) {
      ESP_LOGD(TAG, "SPI Strobe 0x%02X (%s)", strobe, esp_err_to_name(ret));
    }
  }

  // Deselect device
  ESP_ERROR_CHECK(spi_deselect(context));

  GIVE_MUTEX();

  if (ret == ESP_OK) {
    ESP_LOGD(TAG, "SPI (0x%02X) strobe", command);
  }
  return ret;
}

esp_err_t cc1101_init(const cc1101_config_t *config,
                      cc1101_context_t **out_context) {
  if (config == NULL || config->spi_host < SPI2_HOST ||
      config->spi_host >= SPI_HOST_MAX ||
      !GPIO_IS_VALID_OUTPUT_GPIO(config->cs_io_num) || out_context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGV(TAG, "Start initializing");
  esp_err_t ret = ESP_OK;

  // Create context
  cc1101_context_t *context =
      (cc1101_context_t *)malloc(sizeof(cc1101_context_t));
  if (context == NULL) {
    ret = ESP_ERR_NO_MEM;
    ESP_LOGE(TAG, "Can't allocate context (%s)", esp_err_to_name(ret));
    return ret;
  }
  memset(context, 0, sizeof(*context));
  ESP_LOGV(TAG, "Allocated memory for context");

  // Copy config to context
  memcpy(&context->config, config, sizeof(cc1101_config_t));
  ESP_LOGV(TAG, "Copied config to context");

  // Config CS output
  gpio_config_t cs_io_cfg = {
      .pin_bit_mask = (1 << config->cs_io_num),
      .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&cs_io_cfg));
  ESP_ERROR_CHECK(gpio_set_level(config->cs_io_num, 1));

  // Config SPI device and got handle
  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = CC1101_SPI_CLOCK_SPEED_HZ,
      .spics_io_num = -1,
      .queue_size = 8,
  };
  ESP_GOTO_ON_ERROR(spi_bus_add_device(context->config.spi_host, &devcfg,
                                       &context->dev_handle),
                    cleanup, TAG, "SPI bus add device (%s)",
                    esp_err_to_name(ret));
  ESP_LOGV(TAG, "Added device to SPI bus");

  // Test connection: version check
  ret = spi_test_connection(context);
  if (ret != ESP_OK) {
    goto cleanup;
  }

  // Create semaphore
  context->sem_gdo = xSemaphoreCreateBinary();
  if (context->sem_gdo == NULL) {
    ESP_LOGE(TAG, "Can't allocate semaphore");
    return ESP_ERR_NO_MEM;
  }
  ESP_LOGV(TAG, "Created semaphore");

  // Add ISR handler
  ret = gpio_isr_handler_add(context->config.gdo0_io_num, isr_handler_gdo0,
                             context);
  if (ret != ESP_OK) {
    goto cleanup;
  }
  ESP_LOGV(TAG, "Added ISR handler");

  // Software Reset
  ret = cc1101_sw_reset(context);
  if (ret != ESP_OK) {
    goto cleanup;
  }

  // Set IDLE
  ret = cc1101_set_idle_mode(context);
  if (ret != ESP_OK) {
    goto cleanup;
  }

  *out_context = context;

  ESP_LOGI(TAG, "Init completed");
  return ret;

cleanup:
  esp_err_t di_ret = cc1101_deinit(context);
  if (di_ret != ESP_OK) {
    ESP_LOGE(TAG, "Deinit fail (%s)", esp_err_to_name(di_ret));
  }
  return ret;
}

esp_err_t cc1101_deinit(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Deinit start");

  // Disable ISR
  ESP_ERROR_CHECK(
      gpio_set_intr_type(context->config.gdo0_io_num, GPIO_INTR_DISABLE));
  ESP_LOGV(TAG, "Disable ISR");

  // Removed ISR handler
  esp_err_t ret = gpio_isr_handler_remove(context->config.gdo0_io_num);
  if (ret == ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "Call gpio_install_isr_service before init.");
  } else if (ret == ESP_OK) {
    ESP_LOGV(TAG, "Removed ISR handler");
  }

  // Remove SPI device
  if (context->dev_handle != NULL) {
    ESP_ERROR_CHECK(spi_bus_remove_device(context->dev_handle));
    ESP_LOGV(TAG, "Remove device from bus");
    context->dev_handle = NULL;
  }

  // Delete semaphore
  if (context->sem_gdo != NULL) {
    vSemaphoreDelete(context->sem_gdo);
    context->sem_gdo = NULL;
  }

  // Delete context
  free(context);

  ESP_LOGI(TAG, "Deinit completed");
  return ESP_OK;
}

esp_err_t cc1101_set_config(cc1101_context_t *context,
                            const cc1101_register_init_t *reg_inits,
                            uint8_t length) {
  if (context == NULL || reg_inits == NULL || length == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGV(TAG, "Setting configuration (%i regs)", length);

  esp_err_t ret = ESP_OK;

  for (size_t i = 0; i < length; i++) {
    const cc1101_register_init_t *reg_init = &reg_inits[i];

    // Write
    ret = cc1101_write(context, reg_init->addr, &reg_init->value, 1);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Can't write: addr 0x%02X, value 0x%02X (0x%X)",
               reg_init->addr, reg_init->value, ret);
      return ret;
    }
  }

  ESP_LOGI(TAG, "Setted configuration (%i regs wrote)", length);
  return ret;
}

esp_err_t cc1101_set_idle_mode(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret = cc1101_strobe(context, CC1101_CMD_SIDLE, 0);
  if (ret == ESP_OK) {
    isr_disable(context);
    ESP_LOGD(TAG, "Set IDLE mode");
  }
  return ret;
}

esp_err_t cc1101_set_rx_mode(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret = cc1101_strobe(context, CC1101_CMD_SRX, 0);
  if (ret == ESP_OK) {
    isr_enable(context);
    ESP_LOGD(TAG, "Set RX mode");
  }
  return ret;
}

esp_err_t cc1101_sw_reset(cc1101_context_t *context) {
  if (context == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret = cc1101_strobe(context, CC1101_CMD_SRES, 0);
  if (ret == ESP_OK) {
    esp_rom_delay_us(50);
    ESP_LOGD(TAG, "SW Reset");
  }
  return ret;
}

static inline float get_rssi_dbm(uint8_t byte) {
  int32_t rssi = (int32_t)byte;

  if (rssi >= 128) {
    rssi -= 256;
  }

  return rssi / 2.0 - CC1101_RSSI_OFFSET;
}

static inline uint8_t get_lqi(uint8_t byte) { return byte & 0x7F; }

esp_err_t cc1101_read_packet(cc1101_context_t *context, uint8_t *data_out,
                             uint8_t length, float *rssi_dbm_out,
                             uint8_t *lqi_out) {
  if (context == NULL || data_out == NULL || length == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = ESP_OK;

  if (context->packet_received) {
    // Allocate temp buffer
    uint8_t buffer_len = length + 2; // + RSSI & LQI
    uint8_t *buffer = (uint8_t *)malloc(buffer_len);
    if (buffer == NULL) {
      return ESP_ERR_NO_MEM;
    }

    // Reset flag
    context->packet_received = false;

    // Read data to temp buffer
    ret = cc1101_read(context, CC1101_ADDR_FIFO, buffer, buffer_len);
    if (ret == ESP_OK) {

      // Copy data to output buffer
      memcpy(data_out, buffer, length);

      // RSSI & LQI
      *rssi_dbm_out = get_rssi_dbm(buffer[buffer_len - 2]);
      *lqi_out = get_lqi(buffer[buffer_len - 1]);
    }

    // Free temp buffer
    free(buffer);
  } else {
    ret = ESP_ERR_NOT_ALLOWED;
  }

  return ret;
}
