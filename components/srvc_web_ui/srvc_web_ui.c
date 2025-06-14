/**
 ********************************************************************************
 * @file    srvc_web_ui.c
 * @author  Viktor Banko S. (bankviktor14@gmail.com)
 * @date    11.02.2025
 * @brief   Source file of the Web UI.
 ********************************************************************************
 */

#include "srvc_web_ui.h"
#include "app_config.h"
#include "srvc_web_ui_static.h"
#include "esp_netif.h"
#include <cJSON.h>
#include <ctype.h> // для isprint()
#include <esp_app_desc.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lwip/inet.h"  // Для ip4addr_ntoa

static const char *TAG = "web_ui";

#define POST_REQUEST_CONTENT_MAX_LENGTH 512
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PRESS_NORMAL_FRONT_MIN 100
#define PRESS_NORMAL_FRONT_MAX 300
#define PRESS_NORMAL_REAR_MIN 100
#define PRESS_NORMAL_REAR_MAX 300
#define PRESS_CAUTION_DEV_MIN 0
#define PRESS_CAUTION_DEV_MAX 200
#define PRESS_CRITICAL_DEV_MIN 0
#define PRESS_CRITICAL_DEV_MAX 200
#define TEMP_CAUTION_MAX 100
#define TEMP_CRITICAL_MAX 100

typedef struct user_context_t {
  tpms_core_t *tpms_core;
  app_config_t *config;
} user_context_t;

typedef cJSON *(*json_getter_f)(user_context_t *user_ctx);

#define ADD_HANDLER(_method, _uri, _func)                                      \
  uri.method = _method;                                                        \
  uri.uri = _uri;                                                              \
  uri.handler = _func;                                                         \
  ESP_ERROR_CHECK(httpd_register_uri_handler(handle, &uri));

WEB_FILE_DECLR(_index_html)
WEB_FILE_DECLR(_style_css)
WEB_FILE_DECLR(_script_js)
WEB_FILE_DECLR(_favicon_png)

WEB_STATIC_MAP_BEGIN()
WEB_STATIC_MAP_ENTITY(_style_css, MIME_CSS)
WEB_STATIC_MAP_ENTITY(_script_js, MIME_JS)
WEB_STATIC_MAP_ENTITY(_favicon_png, MIME_PNG)
WEB_STATIC_MAP_END()

static bool is_valid_wifi_ssid(const char *ssid, const char **ret_error_msg) {
  if (ssid == NULL) {
    *ret_error_msg = "Invalid Wi-Fi SSID";
    return false;
  }

  size_t len = strlen(ssid);

  if (len < WIFI_SSID_MIN_LEN || len >= WIFI_SSID_MAX_LEN) {
    *ret_error_msg = "Invalid length of Wi-Fi SSID (" STR(
        WIFI_SSID_MIN_LEN) "-" STR(WIFI_SSID_MAX_LEN) ")";
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    if (!isprint((unsigned char)ssid[i])) {
      *ret_error_msg = "Wi-Fi SSID contains not valid symbols";
      return false;
    }
  }

  *ret_error_msg = NULL;
  return true;
}

static bool is_valid_wifi_pw(const char *pw, const char **ret_error_msg) {
  if (pw == NULL) {
    *ret_error_msg = "Invalid Wi-Fi password";
    return false;
  }

  size_t len = strlen(pw);

  if (len < WIFI_PW_MIN_LEN || len >= WIFI_PW_MAX_LEN) {
    *ret_error_msg = "Invalid length of Wi-Fi password (" STR(
        WIFI_PW_MIN_LEN) "-" STR(WIFI_PW_MAX_LEN) ")";
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    if (!isprint((unsigned char)pw[i])) {
      *ret_error_msg = "Wi-Fi password contains not valid symbols";
      return false;
    }
  }

  *ret_error_msg = NULL;
  return true;
}

static bool try_parse_sensor_id(const char *hex_str,
                                tpms_sensor_id_t *sensor_id,
                                const char **ret_error_msg) {
  *sensor_id = 0;
  *ret_error_msg = "Invalid sensor Id";

  if (!hex_str || !sensor_id) {
    return false;
  }

  size_t len = strlen(hex_str);
  if (len == 0 || len > 8) {
    return false;
  }

  uint32_t value = 0;

  for (size_t i = 0; i < len; ++i) {
    char c = hex_str[i];
    uint8_t digit;

    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      digit = c - 'A' + 10;
    } else {
      return false;
    }

    value = (value << 4) | digit;
  }

  *sensor_id = value;
  *ret_error_msg = NULL;
  return true;
}

static bool get_sensor_id(cJSON *ids_json, const char *name,
                          tpms_sensor_id_t *sensor_id,
                          const char **ret_error_msg) {

  // Sensor ID - object
  cJSON *sid_json = cJSON_GetObjectItem(ids_json, name);

  // Check NULL
  if (cJSON_IsNull(sid_json)) {
    *ret_error_msg = NULL;
    return true;
  }

  // Check STRING & Validation & Parsing
  if (!cJSON_IsString(sid_json) ||
      !try_parse_sensor_id(sid_json->valuestring, sensor_id, ret_error_msg)) {
    return false;
  }

  *ret_error_msg = NULL;
  return true;
}

static void strrpl(char *buffer, char old_char, char new_char) {
  while (*buffer != '\0') {
    if (*buffer == old_char) {
      *buffer = new_char;
    }
    buffer++;
  }
}

static const web_static_map_entity_t *find_static_file(const char *file_alias) {
  const web_static_map_entity_t *target_map_entity = NULL;

  uint32_t map_len = sizeof(_web_static_map) / sizeof(*_web_static_map);
  for (uint32_t i = 0; i < map_len; i++) {
    const web_static_map_entity_t *map_entity = &_web_static_map[i];

    if (strcmp(map_entity->alias, file_alias) == 0) {
      target_map_entity = map_entity;
      break;
    }
  }

  return target_map_entity;
}

static esp_err_t resp_json(httpd_req_t *req, json_getter_f func) {
  user_context_t *user_ctx = (user_context_t *)req->user_ctx;
  assert(user_ctx != NULL);

  // Get JSON object
  cJSON *root_json = func(user_ctx);
  if (root_json == NULL) {
    httpd_resp_send_500(req);
    return ESP_OK;
  }

  // Covert JSON object to string
  char *json_str = cJSON_Print(root_json);
  if (json_str == NULL) {
    cJSON_Delete(root_json);
    httpd_resp_send_500(req);
    return ESP_OK;
  }

  // Send JSON string
  ESP_ERROR_CHECK(httpd_resp_sendstr(req, json_str));

  // Free resources
  free(json_str);
  cJSON_Delete(root_json);
  return ESP_OK;
}

static cJSON *create_json_state(user_context_t *user_ctx) {
  assert(user_ctx != NULL);

  cJSON *root_json = NULL;
  cJSON *sensors_json = NULL;

  root_json = cJSON_CreateObject();
  if (root_json == NULL) {
    goto cleanup;
  }

  sensors_json = cJSON_CreateArray();
  if (sensors_json == NULL) {
    goto cleanup;
  }

  tpms_sensor_num_t sensor_num = 0;
  for (; sensor_num < SENSOR_TIRE_MAX; sensor_num++) {
    tpms_sensor_t *sensor = &user_ctx->tpms_core->sensors[sensor_num];

    if (sensor->id == 0) {
      continue;
    }

    cJSON *sensor_json = cJSON_CreateObject();
    if (sensor_json == NULL) {
      goto cleanup;
    }

    const char *tire;
    switch (sensor_num) {
    case SENSOR_TIRE_FRONT_LEFT:
      tire = "FL";
      break;

    case SENSOR_TIRE_FRONT_RIGHT:
      tire = "FR";
      break;

    case SENSOR_TIRE_REAR_LEFT:
      tire = "RL";
      break;

    case SENSOR_TIRE_REAR_RIGHT:
      tire = "RR";
      break;

    default:
      tire = "??";
      break;
    }
    cJSON_AddStringToObject(sensor_json, "tire", tire);

    char sensor_id[9] = {0};
    sprintf(sensor_id, SIDSTR, sensor->id);
    cJSON_AddStringToObject(sensor_json, "id", sensor_id);

    if (sensor->flags & SENSOR_FLAG_VALID_DATA) {
      cJSON_AddNumberToObject(sensor_json, "rssi", sensor->data.rssi);
      cJSON_AddNumberToObject(sensor_json, "pressure_kpa",
                              sensor->data.pressure_kpa);
      cJSON_AddNumberToObject(sensor_json, "temperature_c",
                              sensor->data.temperature_c);
    } else {
      cJSON_AddNullToObject(sensor_json, "rssi");
      cJSON_AddNullToObject(sensor_json, "pressure_kpa");
      cJSON_AddNullToObject(sensor_json, "temperature_c");
    }

    cJSON_AddItemToArray(sensors_json, sensor_json);
  }

  cJSON_AddItemToObject(root_json, "sensors", sensors_json);

  return root_json;

cleanup:

  if (root_json != NULL) {
    cJSON_Delete(root_json);
  }

  if (sensors_json != NULL) {
    cJSON_Delete(sensors_json);
  }

  return NULL;
}

static cJSON *create_json_firmware_info(user_context_t *user_ctx) {
  assert(user_ctx != NULL);

  cJSON *root_json = NULL;

  root_json = cJSON_CreateObject();
  if (root_json == NULL) {
    goto cleanup;
  }

  const esp_app_desc_t *app_desc = esp_app_get_description();
  // esp_app_get_description() or esp_ota_get_partition_description()

  cJSON_AddStringToObject(root_json, "app_ver", app_desc->version);
  cJSON_AddStringToObject(root_json, "idf_ver", app_desc->idf_ver);
  cJSON_AddStringToObject(root_json, "build_date", app_desc->date);
  cJSON_AddStringToObject(root_json, "build_time", app_desc->time);

  return root_json;

cleanup:

  if (root_json != NULL) {
    cJSON_Delete(root_json);
  }

  return NULL;
}

static void AddSensorIdToObject(cJSON *const object, const char *const name,
                                tpms_sensor_id_t sensor_id) {
  if (sensor_id == 0) {
    cJSON_AddNullToObject(object, name);
  } else {
    char buff[9] = {0};
    sprintf(buff, SIDSTR, sensor_id);
    cJSON_AddStringToObject(object, name, buff);
  }
}

static cJSON *create_json_settings(user_context_t *user_ctx) {
  assert(user_ctx != NULL);

  cJSON *root_json = NULL;

  root_json = cJSON_CreateObject();
  if (root_json == NULL) {
    goto cleanup;
  }

  // Wi-Fi
  cJSON *wifi_json = cJSON_CreateObject();
  app_wifi_config_t *wifi_config = &user_ctx->config->wifi_config;
  cJSON_AddStringToObject(wifi_json, "ssid", wifi_config->ssid);
  cJSON_AddStringToObject(wifi_json, "pw", wifi_config->password);
  cJSON_AddItemToObject(root_json, "wifi", wifi_json);

  // Sensor IDs
  cJSON *ids_json = cJSON_CreateObject();
  tpms_sensor_id_t *ids = user_ctx->config->tpms_config.sensor_ids;
  AddSensorIdToObject(ids_json, "fl", ids[SENSOR_TIRE_FRONT_LEFT]);
  AddSensorIdToObject(ids_json, "fr", ids[SENSOR_TIRE_FRONT_RIGHT]);
  AddSensorIdToObject(ids_json, "rl", ids[SENSOR_TIRE_REAR_LEFT]);
  AddSensorIdToObject(ids_json, "rr", ids[SENSOR_TIRE_REAR_RIGHT]);
  cJSON_AddItemToObject(root_json, "ids", ids_json);

  float value;

  // Pressure
  cJSON *press_json = cJSON_CreateObject();
  app_tpms_config_t *tpms_config = &user_ctx->config->tpms_config;

  value = tpms_config->pressure_kpa_normal_front;
  cJSON_AddNumberToObject(press_json, "normal_front", value);

  value = tpms_config->pressure_kpa_normal_rear;
  cJSON_AddNumberToObject(press_json, "normal_rear", value);

  value = tpms_config->pressure_kpa_caution_dev;
  cJSON_AddNumberToObject(press_json, "caution_dev", value);

  value = tpms_config->pressure_kpa_critical_dev;
  cJSON_AddNumberToObject(press_json, "critical_dev", value);

  cJSON_AddItemToObject(root_json, "press", press_json);

  // Temperature
  cJSON *temp_json = cJSON_CreateObject();

  value = tpms_config->temperature_c_caution_thr;
  cJSON_AddNumberToObject(temp_json, "caution_max", value);

  value = tpms_config->temperature_c_critical_thr;
  cJSON_AddNumberToObject(temp_json, "critical_max", value);

  cJSON_AddItemToObject(root_json, "temp", temp_json);

  // valid_data_time_us
  uint64_t valid_data_time_us =
      user_ctx->config->tpms_config.valid_data_time_us;
  cJSON_AddNumberToObject(root_json, "vet_sec", valid_data_time_us / 1E6);

  return root_json;

cleanup:

  if (root_json != NULL) {
    cJSON_Delete(root_json);
  }

  return NULL;
}

static esp_err_t get_static_file_handler(httpd_req_t *req,
                                         httpd_err_code_t error) {
  uint32_t uri_len = strlen(req->uri);
  if (uri_len > 512) {
    ESP_ERROR_CHECK(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                        "Request body too large"));
    return ESP_OK;
  }

  char *file_alias = (char *)malloc(uri_len + 1);
  memcpy(file_alias, req->uri, uri_len);
  file_alias[uri_len] = '\0';
  strrpl(file_alias, '/', '_');
  strrpl(file_alias, '.', '_');
  ESP_LOGD(TAG, "Static file: uri:%s file_alias:%s", req->uri, file_alias);

  const web_static_map_entity_t *target_map_entity =
      find_static_file(file_alias);

  free(file_alias);

  esp_err_t ret = ESP_OK;

  if (target_map_entity == NULL) {
    ret = httpd_resp_send_404(req);
  } else {
    uint32_t len = target_map_entity->end - target_map_entity->start;
    ret = httpd_resp_send(req, target_map_entity->start, len);
  }

  return ret;
}

// Handlers --------------------------------------------------------------------

static esp_err_t get_home_handler(httpd_req_t *req) {
  uint32_t len = _binary_index_html_end - _binary_index_html_start;
  esp_err_t ret = httpd_resp_send(req, _binary_index_html_start, len);
  return ret;
}

static esp_err_t get_state_handler(httpd_req_t *req) {
  return resp_json(req, create_json_state);
}

static esp_err_t get_firmware_info_handler(httpd_req_t *req) {
  return resp_json(req, create_json_firmware_info);
}

static esp_err_t get_settings_handler(httpd_req_t *req) {
  return resp_json(req, create_json_settings);
}

static esp_err_t post_settings_handler(httpd_req_t *req) {
  user_context_t *user_ctx = (user_context_t *)req->user_ctx;
  assert(user_ctx != NULL);

  const char *error_msg = NULL;

  // Content length too small
  size_t content_len = req->content_len;
  if (content_len <= 2) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body too small");
    return ESP_OK;
  }

  // Content length too large
  if (content_len > POST_REQUEST_CONTENT_MAX_LENGTH) {
    httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE,
                        "Request body too large");
    return ESP_OK;
  }

  // Read content to buffer
  char content[POST_REQUEST_CONTENT_MAX_LENGTH + 1];
  assert(httpd_req_recv(req, content, POST_REQUEST_CONTENT_MAX_LENGTH) ==
         content_len);
  content[POST_REQUEST_CONTENT_MAX_LENGTH] = '\0';

  // Parse content buffer to JSON object
  cJSON *root_json = cJSON_Parse(content);
  if (root_json == NULL) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    return ESP_OK;
  }

  const char *wifi_ssid = NULL;
  const char *wifi_pw = NULL;
  tpms_sensor_id_t sid_fl;
  tpms_sensor_id_t sid_fr;
  tpms_sensor_id_t sid_rl;
  tpms_sensor_id_t sid_rr;
  uint32_t press_normal_front;
  uint32_t press_normal_rear;
  uint32_t press_caution_dev;
  uint32_t press_critical_dev;
  uint32_t temp_caution_max;
  uint32_t temp_critical_max;
  uint32_t vet_sec;

  // Wi-Fi
  {
    // Wi-Fi object
    cJSON *wifi_json = cJSON_GetObjectItem(root_json, "wifi");
    if (!cJSON_IsObject(wifi_json)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "'wifi' property not found");
      return ESP_OK;
    }

    {
      // Wi-Fi SSID object
      cJSON *wifi_ssid_json = cJSON_GetObjectItem(wifi_json, "ssid");
      if (!cJSON_IsString(wifi_ssid_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Wi-Fi SSID");
        return ESP_OK;
      }

      // Wi-Fi SSID value
      if (!is_valid_wifi_ssid(wifi_ssid_json->valuestring, &error_msg)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
        return ESP_OK;
      }

      // Save valid value
      wifi_ssid = wifi_ssid_json->valuestring;
    }

    {
      // Wi-Fi Password object
      cJSON *wifi_pw_json = cJSON_GetObjectItem(wifi_json, "pw");
      if (!cJSON_IsString(wifi_pw_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "Invalid Wi-Fi password");
        return ESP_OK;
      }

      // Wi-Fi Password value
      if (!is_valid_wifi_pw(wifi_pw_json->valuestring, &error_msg)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
        return ESP_OK;
      }

      // Save valid value
      wifi_pw = wifi_pw_json->valuestring;
    }
  }

  // Sensor IDs
  {
    // Sensor IDs object
    cJSON *ids_json = cJSON_GetObjectItem(root_json, "ids");
    if (!cJSON_IsObject(ids_json)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "'ids' property not found");
      return ESP_OK;
    }

    // Sensor Id - Front Left
    if (!get_sensor_id(ids_json, "fl", &sid_fl, &error_msg)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
      return ESP_OK;
    }

    // Sensor Id - Front Right
    if (!get_sensor_id(ids_json, "fr", &sid_fr, &error_msg)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
      return ESP_OK;
    }

    // Sensor Id - Rear Left
    if (!get_sensor_id(ids_json, "rl", &sid_rl, &error_msg)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
      return ESP_OK;
    }

    // Sensor Id - Rear Right
    if (!get_sensor_id(ids_json, "rr", &sid_rr, &error_msg)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
      return ESP_OK;
    }
  }

  // Pressure
  {
    // Pressure object
    cJSON *press_json = cJSON_GetObjectItem(root_json, "press");
    if (!cJSON_IsObject(press_json)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "'press' property not found");
      return ESP_OK;
    }

    {
      // normal_front object
      cJSON *press_norm_front_json =
          cJSON_GetObjectItem(press_json, "normal_front");
      if (!cJSON_IsNumber(press_norm_front_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "'normal_front' property not found");
        return ESP_OK;
      }

      // normal_front in range
      press_normal_front = press_norm_front_json->valueint;
      if (press_normal_front < PRESS_NORMAL_FRONT_MIN ||
          press_normal_front > PRESS_NORMAL_FRONT_MAX) {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid range of normal pressure for front tires (" STR(
                PRESS_NORMAL_FRONT_MIN) "-" STR(PRESS_NORMAL_FRONT_MAX) ")");
        return ESP_OK;
      }
    }

    {
      // normal_rear object
      cJSON *press_norm_rear_json =
          cJSON_GetObjectItem(press_json, "normal_rear");
      if (!cJSON_IsNumber(press_norm_rear_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "'normal_rear' property not found");
        return ESP_OK;
      }

      // normal_front in range
      press_normal_rear = press_norm_rear_json->valueint;
      if (press_normal_rear < PRESS_NORMAL_REAR_MIN ||
          press_normal_rear > PRESS_NORMAL_REAR_MAX) {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid range of normal pressure for rear tires (" STR(
                PRESS_NORMAL_REAR_MIN) "-" STR(PRESS_NORMAL_REAR_MAX) ")");
        return ESP_OK;
      }
    }

    {
      // caution_dev object
      cJSON *press_caution_dev_json =
          cJSON_GetObjectItem(press_json, "caution_dev");
      if (!cJSON_IsNumber(press_caution_dev_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "'caution_dev' property not found");
        return ESP_OK;
      }

      // caution_dev in range
      if (press_caution_dev_json->valueint < PRESS_CAUTION_DEV_MIN ||
          press_caution_dev_json->valueint > PRESS_CAUTION_DEV_MAX) {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid range of pressure caution deviation (" STR(
                PRESS_CAUTION_DEV_MIN) "-" STR(PRESS_CAUTION_DEV_MAX) ")");
        return ESP_OK;
      }
      press_caution_dev = press_caution_dev_json->valueint;
    }

    {
      // critical_dev object
      cJSON *press_critical_dev_json =
          cJSON_GetObjectItem(press_json, "critical_dev");
      if (!cJSON_IsNumber(press_critical_dev_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "'critical_dev' property not found");
        return ESP_OK;
      }

      // critical_dev in range
      if (press_critical_dev_json->valueint < PRESS_CRITICAL_DEV_MIN ||
          press_critical_dev_json->valueint > PRESS_CRITICAL_DEV_MAX) {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid range of pressure critical deviation (" STR(
                PRESS_CRITICAL_DEV_MIN) "-" STR(PRESS_CRITICAL_DEV_MAX) ")");
        return ESP_OK;
      }
      press_critical_dev = press_critical_dev_json->valueint;
    }

    if (press_critical_dev < press_caution_dev) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "Pressure critical deviation mush be more then "
                          "pressure caution deviation");
      return ESP_OK;
    }
  }

  // Temperature
  {
    // Temperature object
    cJSON *temp_json = cJSON_GetObjectItem(root_json, "temp");
    if (!cJSON_IsObject(temp_json)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "'temp' property not found");
      return ESP_OK;
    }

    {
      // caution_max object
      cJSON *temp_caution_max_json =
          cJSON_GetObjectItem(temp_json, "caution_max");
      if (!cJSON_IsNumber(temp_caution_max_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "'caution_max' property not found");
        return ESP_OK;
      }

      // caution_max in range
      temp_caution_max = temp_caution_max_json->valueint;
      if (temp_caution_max < TEMP_CAUTION_MAX) {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid range of temperature caution threshold (" STR(
                TEMP_CAUTION_MAX) ")");
        return ESP_OK;
      }
    }

    {
      // critical_max object
      cJSON *temp_critical_max_json =
          cJSON_GetObjectItem(temp_json, "critical_max");
      if (!cJSON_IsNumber(temp_critical_max_json)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "'critical_max' property not found");
        return ESP_OK;
      }

      // critical_max in range
      temp_critical_max = temp_critical_max_json->valueint;
      if (temp_critical_max < TEMP_CRITICAL_MAX) {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid range of temperature critical threshold (" STR(
                TEMP_CRITICAL_MAX) ")");
        return ESP_OK;
      }
    }

    if (temp_critical_max < temp_caution_max) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "Temperature critical threshold mush be more then "
                          "temperature caution threshold");
      return ESP_OK;
    }
  }

  // vet_sec
  {
    // vet_sec object
    cJSON *vet_sec_json = cJSON_GetObjectItem(root_json, "vet_sec");
    if (!cJSON_IsNumber(vet_sec_json)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "'vet_sec' property not found");
      return ESP_OK;
    }

    // caution_max in range
    if (vet_sec_json->valueint < 0 || vet_sec_json->valueint > 3600) { // 1 hour
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                          "Invalid range of vet_sec (0-3600)");
      return ESP_OK;
    }
    vet_sec = vet_sec_json->valueint;
  }

  // Update current configuration
  app_config_t *cfg = user_ctx->config;
  memcpy(cfg->wifi_config.ssid, wifi_ssid, strlen(wifi_ssid));
  memcpy(cfg->wifi_config.password, wifi_pw, strlen(wifi_pw));
  cfg->tpms_config.sensor_ids[SENSOR_TIRE_FRONT_LEFT] = sid_fl;
  cfg->tpms_config.sensor_ids[SENSOR_TIRE_FRONT_RIGHT] = sid_fr;
  cfg->tpms_config.sensor_ids[SENSOR_TIRE_REAR_LEFT] = sid_rl;
  cfg->tpms_config.sensor_ids[SENSOR_TIRE_REAR_RIGHT] = sid_rr;
  cfg->tpms_config.pressure_kpa_normal_front = press_normal_front;
  cfg->tpms_config.pressure_kpa_normal_rear = press_normal_rear;
  cfg->tpms_config.pressure_kpa_caution_dev = press_caution_dev;
  cfg->tpms_config.pressure_kpa_critical_dev = press_critical_dev;
  cfg->tpms_config.temperature_c_caution_thr = temp_caution_max;
  cfg->tpms_config.temperature_c_critical_thr = temp_critical_max;
  cfg->tpms_config.valid_data_time_us = vet_sec * 1e6;

  // Save current configuration to flash
  app_config_store(cfg);

  httpd_resp_sendstr(req, "Saved");

  // Free JSON
  cJSON_Delete(root_json);
  return ESP_OK;
}

// General functions ----------------------------------------------------------

static void config_httpd(httpd_handle_t handle, tpms_core_t *tpms_core,
                         app_config_t *config) {

  user_context_t *ctx = (user_context_t *)malloc(sizeof(user_context_t));
  assert(ctx != NULL);
  ctx->tpms_core = tpms_core;
  ctx->config = config;

  httpd_uri_t uri = {.user_ctx = ctx};

  ADD_HANDLER(HTTP_GET, "/", get_home_handler);
  ADD_HANDLER(HTTP_GET, "/api/state", get_state_handler);
  ADD_HANDLER(HTTP_GET, "/api/firmware-info", get_firmware_info_handler);
  ADD_HANDLER(HTTP_GET, "/api/settings", get_settings_handler);
  ADD_HANDLER(HTTP_POST, "/api/settings", post_settings_handler);
  // ESP_ERROR_CHECK(httpd_register_err_handler(handle, HTTPD_404_NOT_FOUND,
  //                                            get_static_file_handler));
}

esp_err_t srvc_web_ui_init(tpms_core_t *tpms_core, app_config_t *config) {
  if (tpms_core == NULL || config == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  httpd_handle_t httpd_handle = NULL;
  httpd_config_t httpd_cfg = HTTPD_DEFAULT_CONFIG();
  ESP_ERROR_CHECK(httpd_start(&httpd_handle, &httpd_cfg));

  config_httpd(httpd_handle, tpms_core, config);

  ESP_LOGI(TAG, "Started Web UI");
  return ESP_OK;
}
