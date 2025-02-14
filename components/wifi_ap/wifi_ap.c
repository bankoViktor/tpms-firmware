/**
 ********************************************************************************
 * @file    wifi_ap.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    04.02.2025
 * @brief   Source file of the Wi-Fi AP.
 ********************************************************************************
 */

#include "wifi_ap.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <memory.h>
#include <nvs_flash.h>

static const char *TAG = "wifi_ap";

#define WIFI_CHANNEL 1
#define WIFI_MAX_STA_CONN 4

esp_err_t wifi_softap_srvc(const app_wifi_config_t *app_wifi_cfg,
                           uint8_t is_wifi_pwd_suppress) {
  if (app_wifi_cfg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

  wifi_ap_config_t wifi_ap_cfg = {
      .channel = WIFI_CHANNEL,
      .max_connection = WIFI_MAX_STA_CONN,
  };

  // Set SSID
  int ssid_len = strlen(app_wifi_cfg->ssid);
  memcpy(wifi_ap_cfg.ssid, app_wifi_cfg->ssid, ssid_len);
  wifi_ap_cfg.ssid_len = ssid_len;

  // Set Password
  if (is_wifi_pwd_suppress) {
    wifi_ap_cfg.authmode = WIFI_AUTH_OPEN;
  } else {
    wifi_ap_cfg.authmode = WIFI_AUTH_WPA2_PSK;
    int pwd_len = strlen(app_wifi_cfg->password);
    memcpy(wifi_ap_cfg.password, app_wifi_cfg->password, pwd_len);
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  wifi_config_t wifi_cfg = {.ap = wifi_ap_cfg};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Wi-Fi AP started. SSID: %s", wifi_ap_cfg.ssid);
  return ESP_OK;
}
