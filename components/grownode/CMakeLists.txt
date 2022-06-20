idf_build_get_property(grownode_mock USE_MOCK)
idf_build_get_property(target IDF_TARGET)
if(${grownode_mock})
    message(STATUS "building GROWNODE MOCKS")

    set(IDF_PATH $ENV{IDF_PATH})
    set(CMOCK_DIR "${IDF_PATH}/components/cmock/CMock")
    set(MOCK_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/mocks")

    file(MAKE_DIRECTORY ${MOCK_GEN_DIR})

    set(MOCK_OUTPUT
        #"${MOCK_GEN_DIR}/Mockmqtt_client.c" "${MOCK_GEN_DIR}/Mockmqtt_client.h"
        #"${MOCK_GEN_DIR}/Mockesp_wifi.c" "${MOCK_GEN_DIR}/Mockesp_wifi.h"
        #"${MOCK_GEN_DIR}/Mockesp_event.c" "${MOCK_GEN_DIR}/Mockesp_event.h"
        "${MOCK_GEN_DIR}/Mockesp_event_base.c" "${MOCK_GEN_DIR}/Mockesp_event_base.h"
        #"${MOCK_GEN_DIR}/Mockesp_event_loop.c" "${MOCK_GEN_DIR}/Mockesp_event_loop.h"
        #"${MOCK_GEN_DIR}/Mockesp_err.c" "${MOCK_GEN_DIR}/Mockesp_err.h"
        #"${MOCK_GEN_DIR}/MockFreeRTOS.c" "${MOCK_GEN_DIR}/MockFreeRTOS.h"
        #"${MOCK_GEN_DIR}/Mockgrownode.c" "${MOCK_GEN_DIR}/Mockgrownode.h"
        #"${MOCK_GEN_DIR}/Mockgrownode_intl.c" "${MOCK_GEN_DIR}/Mockgrownode_intl.h"
        "${MOCK_GEN_DIR}/Mockgn_event_source.c" "${MOCK_GEN_DIR}/Mockgn_event_source.h"
        
        )

    set(MOCK_HEADERS
    	#"${IDF_PATH}/components/mqtt/esp-mqtt/include/mqtt_client.h"
        #"${IDF_PATH}/components/esp_event/include/esp_event.h"
        "${IDF_PATH}/components/esp_event/include/esp_event_base.h"
        #"${IDF_PATH}/components/esp_event/include/esp_event_loop.h"
        #"${IDF_PATH}/components/esp_system/include/esp_system.h"
        #"${IDF_PATH}/components/esp_common/include/esp_err.h"
        #"${IDF_PATH}/components/freertos/include/freertos/FreeRTOS.h"
        #"${CMAKE_CURRENT_SOURCE_DIR}/grownode.h"
        #"${CMAKE_CURRENT_SOURCE_DIR}/grownode_intl.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/gn_event_source.h"
        )

    set(ENV{UNITY_DIR} "$ENV{IDF_PATH}/components/cmock/CMock")

    set(include_dirs
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${MOCK_GEN_DIR}"
        )

    set(srcs 
    	#"${CMAKE_CURRENT_SOURCE_DIR}"
        #"${MOCK_GEN_DIR}/Mockmqtt_client.c"
        #"${MOCK_GEN_DIR}/Mockesp_wifi.c"
    	#"${MOCK_GEN_DIR}/Mockesp_event.c"
    	"${MOCK_GEN_DIR}/Mockesp_event_base.c"
    	#"${MOCK_GEN_DIR}/Mockesp_event_loop.c"
    	#"${MOCK_GEN_DIR}/Mockesp_err.c"
    	#"${MOCK_GEN_DIR}/MockFreeRTOS.h"
    	#"${MOCK_GEN_DIR}/Mockgrownode.c"
    	#"${MOCK_GEN_DIR}/Mockgrownode_intl.c"
    	"${MOCK_GEN_DIR}/Mockgn_event_source.c"
		)
			
	if(${target} STREQUAL "linux")
        list(APPEND include_dirs
        #"${IDF_PATH}/components/esp_common/include"
        #"${IDF_PATH}/components/esp_event/include"
        #"${IDF_PATH}/components/freertos/include"
        #"${IDF_PATH}/components/freertos/port/xtensa/include"
        #"${IDF_PATH}/components/xtensa/include"
        #"${IDF_PATH}/components/xtensa/esp32/include"
        #"${IDF_PATH}/components/esp_system/include"
        #"${IDF_PATH}/components/esp_hw_support/include"
        #"${IDF_PATH}/components/newlib/platform_include"
        #"${IDF_PATH}/components/mqtt/esp-mqtt/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../host_test/fixtures"
        )
    endif()

    idf_component_register(SRCS "${srcs}"
                        INCLUDE_DIRS ${include_dirs}
                        REQUIRES cmock log)

    add_custom_command(
        OUTPUT ruby_found SYMBOLIC
        COMMAND "ruby" "-v"
        COMMENT "Try to find ruby. If this fails, you need to install ruby"
    )

    # This command builds the mocks.
    # First, environment variable UNITY_DIR is set. This is necessary to prevent unity from looking in its own submodule
    # which doesn't work in our CI yet...
    # The rest is a straight forward call to cmock.rb, consult cmock's documentation for more information.
    add_custom_command(
        OUTPUT ${MOCK_OUTPUT}
        DEPENDS ruby_found
        COMMAND ${CMAKE_COMMAND} -E env "UNITY_DIR=${IDF_PATH}/components/unity/unity"
            ruby
            ${CMOCK_DIR}/lib/cmock.rb
            -o${CMAKE_CURRENT_SOURCE_DIR}/mock/mock_config.yaml
            ${MOCK_HEADERS}
      )

else()

#normal build

	set(components_required 
		"unity" "mqtt" "spiffs" "json" "wifi_provisioning" "esp_https_ota" "app_update" "esp_adc_cal"
	)
	
	if(DEFINED ENV{COLLECTIONS_C_PATH})
		list(APPEND components_required 
     		"collections-c" 
     	)
   	endif()
	
	if(DEFINED ENV{IDF_LIB_PATH})
	    list(APPEND components_required 
     		"bmp280" "ds18x20" "ina219" "bh1750"
     	)
   	endif()

	if(DEFINED ENV{LVGL_PATH})
    	list(APPEND components_required 
     		"lvgl" "lvgl_esp32_drivers"
     	)
 	endif()
 	
	if(DEFINED ENV{CRON_LIB_PATH})
    	list(APPEND components_required 
     		"esp_cron"
     	)
 	endif()

	idf_component_register(SRCS 
					"gn_commons.c"
					"gn_display.c"
					"grownode.c"
					"gn_mqtt_protocol.c"
					"gn_network.c"
					"gn_leaf_context.c"
					"leaves/gn_bh1750.c"
					"leaves/gn_pwm.c"
					"leaves/gn_pump.c"
					"leaves/gn_pump_hs.c"
					"leaves/gn_ds18b20.c"
					"leaves/gn_bme280.c"
					"leaves/gn_capacitive_water_level.c"
					"leaves/gn_capacitive_moisture_sensor.c"
					"leaves/gn_gpio.c"
					"leaves/gn_led.c"
					"leaves/gn_leaf_status_led.c"
					"leaves/gn_leaf_ina219.c"
					"synapses/gn_hydroboard2_watering_control.c"
					"synapses/gn_pump_control.c"
					"synapses/gn_watering_control.c"
					"synapses/gn_syn_nft1_control.c"
					"boards/gn_hydroboard1.c"
					"boards/gn_hydroboard2.c"
					"boards/gn_easypot1.c"
					"boards/gn_nft2.c"
					"boards/gn_nft1.c"
					"boards/gn_blink.c"
					"boards/gn_oscilloscope.c"
					"boards/gn_board_pump.c"
					"test/test_grownode.c"
					"test/test_pump.c"
					"test/test_ds18b20.c"
					"test/test_pump_control.c"
					"test/test_relay.c"
					
					REQUIRES "${components_required}"
                    INCLUDE_DIRS 
                    "."
                    "leaves/"
                    "boards/"
                    "synapses/"
                    EMBED_TXTFILES server_certs/ca_cert.pem)
endif()

     