
# GrowNode

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment.

![GrowNode Main Screen with a Temperature Leaf](/docs/resources/main_screen.png "Main Screen")

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

## Testing

Unit testing are working although in early stage (started using after core writing), both SOC and Host tests are implemented

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

## Configuration

Test code to configure and startup a full node. depending on options in sdkconfig, this could be sufficient to start a full node with networking, mqtt protocol, flash data storage, and display.
    

	//creates the config handle
	gn_config_handle_t config = gn_init();

	//waits until the config process ends
	while (gn_get_config_status(config) != GN_CONFIG_STATUS_OK) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "grownode startup sequence code: %d",
				gn_get_config_status(config));
	}
	//creates a new node
	gn_node_config_handle_t node = gn_node_create(config, "node");
	ESP_LOGI("main", "gn_create_node, name %s",
			gn_get_node_config_name(node));
	//send log to screen
	gn_message_display("initialized");
	char buf[5] = "pump";
	//create new leaf, controlling pump. size of the leaf task is 4096 bytes
	gn_leaf_config_handle_t pump = gn_leaf_create(node, buf,
			gn_pump_task, 4096);

	ESP_LOGI("main", "gn_create_leaf number - name %s on node %s",
			gn_get_leaf_config_name(pump),
			gn_get_node_config_name(pump));
	//finally, start node
	gn_node_start(node);
	while (true) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "main loop");
	}


## Leaf Task Example

Every leaf is a FreeRTOS task. Hence, is it possible to startup several tasks with the same callback. Here is a demo code for a simple leaf (a pwm controlled pump through a GPIO) 

	void gn_pump_task(gn_leaf_config_handle_t leaf_config) {
	const size_t GN_PUMP_STATE_STOP = 0;
	const size_t GN_PUMP_STATE_RUNNING = 1;
	const size_t GPIO_PWM0A_OUT = 32;
	size_t gn_pump_state = GN_PUMP_STATE_RUNNING;
	gn_leaf_event_t evt;
	//parameter definition. if found in flash storage, they will be created with found values instead of default
	gn_leaf_param_handle_t status_param = gn_leaf_param_create(leaf_config,
			"status", GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b = false });
	gn_leaf_param_add(leaf_config, status_param);
	gn_leaf_param_handle_t power_param = gn_leaf_param_create(leaf_config,
			"power", GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 });
	gn_leaf_param_add(leaf_config, power_param);
	//setup pwm
	mcpwm_pin_config_t pin_config = { .mcpwm0a_out_num = GPIO_PWM0A_OUT };
	mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
	mcpwm_config_t pwm_config;
	pwm_config.frequency = 3000; //TODO make configurable
	pwm_config.cmpr_a = power_param->param_val->v.d;
	pwm_config.cmpr_b = 0.0;
	pwm_config.counter_mode = MCPWM_UP_COUNTER;
	pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings
	//setup screen, if defined in sdkconfig
	#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	static lv_obj_t *label_status;
	static lv_obj_t *label_power;
	static lv_obj_t *label_pump;
	//parent container where adding elements
	lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);
	if (_cnt) {
		//style from the container
		lv_style_t *style = lv_style_list_get_local_style(&_cnt->style_list);
		label_pump = lv_label_create(_cnt, NULL);
		lv_label_set_text(label_pump, "PUMP");
		lv_obj_add_style(label_pump, LV_LABEL_PART_MAIN, style);
		label_status = lv_label_create(_cnt, NULL);
		lv_obj_add_style(label_status, LV_LABEL_PART_MAIN, style);
		lv_label_set_text(label_status,
				status_param->param_val->v.b ? "status: on" : "status: off");
		label_power = lv_label_create(_cnt, NULL);
		lv_obj_add_style(label_power, LV_LABEL_PART_MAIN, style);
		char _p[21];
		snprintf(_p, 20, "power: %f", power_param->param_val->v.d);
		lv_label_set_text(label_power, _p);
	}
	#endif
	//task cycle
	while (true) {
		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {
			//event arrived for this node
			switch (evt.id) {
			//parameter change
			case GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT:
				//parameter is status
				if (gn_common_leaf_event_mask_param(&evt, status_param) == 0) {
	#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {
						lv_label_set_text(label_status,
								status_param->param_val->v.b ?
										"status: on" : "status: off");
						gn_display_leaf_refresh_end();
					}
	#endif
					//parameter is power
				} else if (gn_common_leaf_event_mask_param(&evt, power_param)
						== 0) {
	#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {
						char _p[21];
						snprintf(_p, 20, "power: %f",
								power_param->param_val->v.d);
						lv_label_set_text(label_power, _p);
						gn_display_leaf_refresh_end();
					}
	#endif
				}
				break;
				//what to do when network is connected
			case GN_NETWORK_CONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_RUNNING;
				break;
				//what to do when network is disconnected
			case GN_NETWORK_DISCONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_STOP;
				break;
				//what to do when server is connected
			case GN_SERVER_CONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_RUNNING;
				break;
				//what to do when server is disconnected
			case GN_SERVER_DISCONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_STOP;
				break;
			default:
				break;
			}
		}
		//finally, we update sensor using the parameter values
		if (gn_pump_state != GN_PUMP_STATE_RUNNING) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
		} else if (!status_param->param_val->v.b) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
			//change = false;
		} else {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A,
					power_param->param_val->v.d);
			//change = false;
		}
		vTaskDelay(1);
	}
	}	
