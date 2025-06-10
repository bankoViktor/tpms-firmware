file(GLOB WEB_STATIC_FILES "static/*")

idf_component_register(SRCS "srvc_web_ui.c"
                    INCLUDE_DIRS "include"
                    REQUIRES tpms_core esp_http_server json esp_app_format esp_netif
                    EMBED_FILES ${WEB_STATIC_FILES})
