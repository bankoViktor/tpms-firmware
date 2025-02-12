idf_component_register(SRCS "uhf_receiver_srvc.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver tpms_core cc1101 manchester_codec autel_mx_sensor)
