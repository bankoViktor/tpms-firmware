idf_component_register(SRCS "srvc_uhf_rx.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver tpms_core cc1101 manchester_codec autel_mx_sensor)
