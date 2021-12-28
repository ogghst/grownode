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
idf_component_register(SRCS 
					"gn_commons.c"
					"gn_display.c"
					"grownode.c"
					"gn_mqtt_protocol.c"
					"gn_network.c"
					"gn_leaf_context.c"
					"cc_array.c"
					"cc_common.c"
					"cc_hashtable.c"
					"leaves/gn_pwm.c"
					"leaves/gn_pump.c"
					"leaves/gn_pump_hs.c"
					"leaves/gn_ds18b20.c"
					"leaves/gn_bme280.c"
					"leaves/gn_pump_control.c"
					"leaves/gn_watering_control.c"
					"leaves/gn_capacitive_water_level.c"
					"leaves/gn_relay.c"
					"leaves/gn_leaf_status_led.c"
					"test/test_grownode.c"
					"test/test_pump.c"
					"test/test_ds18b20.c"
					"test/test_pump_control.c"
					"test/test_relay.c"
					"boards/gn_hydroboard1.c"
					"boards/gn_hydroboard2.c"
					"boards/gn_blink.c"
					
					REQUIRES lvgl lvgl_esp32_drivers unity mqtt spiffs json wifi_provisioning esp_https_ota app_update bmp280 ds18x20
                    INCLUDE_DIRS 
                    "."
                    "leaves/"
                    "boards/"
                    EMBED_TXTFILES server_certs/ca_cert.pem)
endif()

     