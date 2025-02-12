idf_component_register(SRCS "can_transmitter_srvc.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver tpms_core ct2_can)
