idf_component_register(SRCS 
					"main.cpp"
					"GrowNodeController.cpp"
					"WiFiController.cpp"
					"ILI9341Controller.cpp"
                    INCLUDE_DIRS ""
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)