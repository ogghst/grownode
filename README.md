
# GrowNode

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment.

# Architecture

GrowNode aims to use most common development tools. Actual release is composed by:

## Hardware

 - ESP32/ESP32S2 microcontrollers
 -  Various displays tested (ILI9341) with touch screen (tested XPT2046) capabilities
 -  Common sensors with any esp-idf compatible libraries

## Software Components

 - ESP-IDF (master branch) programming environment
 - MQTT as a messaging system
 - LVGL as display library

## Others (planning)
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

# How to use example

- install ESP-IDF as per [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- install ESP_IDF-LIB as per [ESP-IDF Components library](https://github.com/UncleRus/esp-idf-lib)
- setup main `CMakeLists.txt` file according to your local paths
- `git clone` this repository
- add `lvgl` (release/v7) and `lvgl_esp32_drivers` (master) as components as specified [here](https://github.com/lvgl/lv_port_esp32)
- connect your ESP32 board to a serial port
- open an ESP-IDF prompt to go to project directory
- type `idf.py menuconfig flash build monitor` according to your needs. this will run the latest test code as per the `main.c` file

# Sample code

    

    #include "grownode.h"
    #include "gn_pump.h"
    
    void app_main(void) {
	    gn_config_handle_t config = gn_init();
	    gn_log_message("System Initialized");
	    gn_node_config_handle_t node_config = gn_create_node(config, "node");
		gn_leaf_config_handle_t pump_config = gn_leaf_create(node_config, buf,
			gn_pump_loop);
		ESP_ERROR_CHECK(gn_node_start(node_config));
		gn_log_message("Created Leaf");
		while (true) {
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			ESP_LOGI("main", "main loop");
		}
	}
        
    void _gn_leaf_task(void *pvParam) {
    	while (true) {
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    		gn_log_message("Leaf Growing!");
    	}
    }
        
    void gn_pump_loop(gn_leaf_config_handle_t leaf_config) {
    
    	gn_leaf_param_handle_t param = gn_leaf_param_create("status",
    			GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { false });
    	gn_leaf_param_add(leaf_config, param);
		while (true) {
				//leaf code
				ESP_LOGI("main", "gn_pump_loop event");
				vTaskDelay(1);
		}
	}

		

