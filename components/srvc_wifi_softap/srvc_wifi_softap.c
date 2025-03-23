/**
 ********************************************************************************
 * @file    srvc_wifi_softap.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Source file of the Wi-Fi AP.
 ********************************************************************************
 */

#include "srvc_wifi_softap.h"
#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <memory.h>
#include <nvs_flash.h>

static const char *TAG = "wifi_ap";

#define WIFI_CHANNEL 1
#define WIFI_MAX_STA_CONN 4

#ifndef WIFI_PW_SUPPRESS_IO_NUM
#define WIFI_PW_SUPPRESS_IO_NUM 8
#endif

static int get_pw_suppress_io_state() {
  gpio_config_t io_cfg = {.pin_bit_mask = (1ULL << WIFI_PW_SUPPRESS_IO_NUM),
                          .mode = GPIO_MODE_INPUT,
                          .pull_up_en = GPIO_PULLUP_ENABLE,
                          .pull_down_en = GPIO_PULLDOWN_DISABLE,
                          .intr_type = GPIO_INTR_DISABLE};
  ESP_ERROR_CHECK(gpio_config(&io_cfg));

  return gpio_get_level(WIFI_PW_SUPPRESS_IO_NUM) ? 0 : 1;
}

static void wifi_ap_config(const app_wifi_config_t *app_wifi_cfg) {
  wifi_ap_config_t wifi_ap_cfg = {
      .channel = WIFI_CHANNEL,
      .max_connection = WIFI_MAX_STA_CONN,
  };

  uint8_t is_wifi_pwd_suppression = get_pw_suppress_io_state();

  // Set Wi-Fi SSID
  int ssid_len = strlen(app_wifi_cfg->ssid);
  memcpy(wifi_ap_cfg.ssid, app_wifi_cfg->ssid, ssid_len);
  wifi_ap_cfg.ssid_len = ssid_len;

  // Set Wi-Fi Password
  if (is_wifi_pwd_suppression) {
    wifi_ap_cfg.authmode = WIFI_AUTH_OPEN;
    ESP_LOGI(TAG, "Wi-Fi AP open");
  } else {
    wifi_ap_cfg.authmode = WIFI_AUTH_WPA2_PSK;
    int pwd_len = strlen(app_wifi_cfg->password);
    memcpy(wifi_ap_cfg.password, app_wifi_cfg->password, pwd_len);
  }

  // Set Wi-Fi to AP mode
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

  // Set Wi-Fi config
  wifi_config_t wifi_cfg = {.ap = wifi_ap_cfg};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
}

esp_err_t srvc_wifi_softap(const app_wifi_config_t *app_wifi_cfg) {
  if (app_wifi_cfg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // Init TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  // Init Wi-Fi
  wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

  // Config Wi-Fi AP
  wifi_ap_config(app_wifi_cfg);

  // Wi-Fi start
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Wi-Fi AP started. SSID: %s", app_wifi_cfg->ssid);
  return ESP_OK;
}
