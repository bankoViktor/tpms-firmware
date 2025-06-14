idf_component_register(SRCS "srvc_wifi_softap.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver nvs_flash esp_wifi app_config)
