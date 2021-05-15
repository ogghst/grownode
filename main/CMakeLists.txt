idf_component_register(SRCS 
					"gn_commons.c"
					"gn_display.c"
					"grownode.c"
					"gn_mqtt_protocol.c"
					"gn_network.c"
					"gn_pump.c"
					"gn_ds18b20.c"
					"main.c"
					
                    INCLUDE_DIRS ""
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
     