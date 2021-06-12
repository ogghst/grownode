idf_component_register(SRCS 
					"gn_commons.c"
					"gn_display.c"
					"grownode.c"
					"gn_mqtt_protocol.c"
					"gn_network.c"
					REQUIRES lvgl lvgl_esp32_drivers
							
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES server_certs/ca_cert.pem)
     