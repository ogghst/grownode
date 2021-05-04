
# GrowNode 2 - ESP-IDF edition

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment.

# Architecture

GrowNode aims to use most common development tools. Actual release is composed by:

## Hardware

 - ESP32/ESP32S2 microcontrollers
 -  Various displays like ILI9341 with touch screen capabilities
 -  Common sensors with any esp-idf compatible libraries

## Software Components

 - ESP-IDF v4.2 programming environment
 - MQTT as a messaging system
 - LVGL as display library

## Others
 - OpenHab as a controller
 - Fusion360 as Schematic and 3D CAD


## Functionalities

Every node provides some basic services

- High level device configuration
- Hardware and Software abstraction layer - think about nodes as devices and leaves as sensors
- SoftAP/Bluetooth Provisioning to join and change your wireless network without any hardcode configuration through a mobile app
- Firmware update Over The Air
- NTP clock sync
- Client/Server and Client/Client network configuration and high level messaging protocol, having in mind highly distributed environment (mesh networks, cloud communication)

## How to use example

add lvgl (release/v7) and lvgl_esp32_drivers (master) as components as specified here: [esp32 port](https://github.com/lvgl/lv_port_esp32)

## Sample code

    

    #include "grownode.h"
    #include "gn_pump.h"
    
    void app_main(void) {
	    gn_config_handle_t config = gn_init();
	    gn_log_message("System Initialized");
	    gn_node_config_handle_t node_config = gn_create_node(config, "node");
	    gn_leaf_config_handle_t leaf_config = gn_create_leaf(node_config,
					"leaf", gn_leaf_callback);
		gn_leaf_init(leaf_config);
		gn_log_message("Created Leaf");
	}
        
    void _gn_leaf_task(void *pvParam) {
    	while (true) {
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    		gn_log_message("Leaf Growing!");
    	}
    }
    
    void gn_leaf_init(gn_leaf_config_handle_t config) {
    	xTaskCreate(_gn_leaf_task, "_gn_leaf_task", 2048, NULL, 0, NULL);
    }
    
    void gn_pump_callback(gn_event_handle_t event, void *event_data) {
    	gn_log_message("gn_leaf_callback");
    }
