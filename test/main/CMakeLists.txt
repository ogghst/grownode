idf_component_register(SRCS "test_grownode.c"
                    INCLUDE_DIRS "."
                    REQUIRES unity grownode mqtt spiffs wifi_provisioning esp_https_ota app_update json spi_flash ds18x20)
