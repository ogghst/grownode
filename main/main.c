// Copyright 2021 Nicola Muratori (nicola.muratori@gmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_event.h"

#include "grownode.h"
#include "gn_pump_hs.h"
#include "gn_ds18b20.h"
#include "gn_relay.h"
#include "gn_capacitive_water_level.h"
#include "gn_pump.h"
#include "gn_bme280.h"

#define TASK_STACK_SIZE 8192*4

#define TAG "gn_main"

void app_main(void) {

	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("grownode", ESP_LOG_INFO);
	esp_log_level_set("gn_commons", ESP_LOG_INFO);
	esp_log_level_set("gn_nvs", ESP_LOG_INFO);
	esp_log_level_set("gn_mqtt_protocol", ESP_LOG_INFO);
	esp_log_level_set("gn_network", ESP_LOG_INFO);
	esp_log_level_set("gn_display", ESP_LOG_INFO);

	esp_log_level_set("gn_leaf_relay", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_pump_hs", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_pump_control", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_ds18b20", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_cwl", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_bme280", ESP_LOG_INFO);

	//creates the config handle
	gn_config_handle_t config = gn_init();

	//waits until the config process ends
	while (gn_get_config_status(config) != GN_CONFIG_STATUS_COMPLETED) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "grownode startup sequence code: %d",
				gn_get_config_status(config));
	}

	//creates a new node
	gn_node_config_handle_t node = gn_node_create(config, "node");
	ESP_LOGI(TAG, "gn_create_node, name %s", gn_get_node_config_name(node));

	//send log to screen
	gn_log("initialized");

	gn_leaf_config_handle_t lights1in = gn_leaf_create(node, "lights1in",
			gn_relay_config, 4096);
	gn_leaf_param_init_double(lights1in, GN_RELAY_PARAM_GPIO, 25);
	gn_leaf_param_init_bool(lights1in, GN_RELAY_PARAM_STATUS, false);

	gn_leaf_config_handle_t lights2in = gn_leaf_create(node, "lights2in",
			gn_relay_config, 4096);
	gn_leaf_param_init_double(lights2in, GN_RELAY_PARAM_GPIO, 33);
	gn_leaf_param_init_bool(lights2in, GN_RELAY_PARAM_STATUS, false);

	gn_leaf_config_handle_t pltonoffin = gn_leaf_create(node, "pltonoffin",
			gn_relay_config, 4096);
	gn_leaf_param_init_double(pltonoffin, GN_RELAY_PARAM_GPIO, 23);
	gn_leaf_param_init_bool(pltonoffin, GN_RELAY_PARAM_STATUS, false);

	gn_leaf_config_handle_t pltin = gn_leaf_create(node, "pltin",
			gn_relay_config, 4096);
	gn_leaf_param_init_double(pltin, GN_RELAY_PARAM_GPIO, 5);
	gn_leaf_param_init_bool(pltin, GN_RELAY_PARAM_STATUS, false);

	gn_leaf_config_handle_t waterpumpin = gn_leaf_create(node, "waterpumpin",
			gn_relay_config, 4096);
	gn_leaf_param_init_double(waterpumpin, GN_RELAY_PARAM_GPIO, 17);
	gn_leaf_param_init_bool(waterpumpin, GN_RELAY_PARAM_STATUS, false);

	gn_leaf_config_handle_t waterlevelin = gn_leaf_create(node, "waterlevelin",
			gn_capacitive_water_level_config, 4096);
	gn_leaf_param_init_bool(waterlevelin, GN_CWL_PARAM_ACTIVE, false);
	gn_leaf_param_init_double(waterlevelin, GN_CWL_PARAM_TOUCH_CHANNEL, 2);
	gn_leaf_param_init_double(waterlevelin, GN_CWL_PARAM_UPDATE_TIME_SEC, 10);


	gn_leaf_config_handle_t hcc_speed = gn_leaf_create(node, "hcc",
			gn_pump_hs_config, 4096);
	gn_leaf_param_init_bool(hcc_speed, GN_PUMP_HS_PARAM_CHANNEL, 0);
	gn_leaf_param_init_double(hcc_speed, GN_PUMP_HS_PARAM_GPIO_POWER, 18);
	gn_leaf_param_init_double(hcc_speed, GN_PUMP_HS_PARAM_POWER, 0);
	gn_leaf_param_init_double(hcc_speed, GN_PUMP_HS_PARAM_GPIO_TOGGLE, 26);
	gn_leaf_param_init_bool(hcc_speed, GN_PUMP_HS_PARAM_TOGGLE, false);

	gn_leaf_config_handle_t fan_speed = gn_leaf_create(node, "fan",
			gn_pump_hs_config, 4096);
	gn_leaf_param_init_bool(fan_speed, GN_PUMP_HS_PARAM_CHANNEL, 1);
	gn_leaf_param_init_double(fan_speed, GN_PUMP_HS_PARAM_GPIO_POWER, 27);
	gn_leaf_param_init_double(fan_speed, GN_PUMP_HS_PARAM_POWER, 0);
	gn_leaf_param_init_double(fan_speed, GN_PUMP_HS_PARAM_GPIO_TOGGLE, 33);
	gn_leaf_param_init_bool(fan_speed, GN_PUMP_HS_PARAM_TOGGLE, false);

	gn_leaf_config_handle_t bme280 = gn_leaf_create(node, "bme280",
			gn_bme280_config, 4096);
	gn_leaf_param_init_double(bme280, GN_BME280_PARAM_SDA, 21);
	gn_leaf_param_init_double(bme280, GN_BME280_PARAM_SCL, 22);
	gn_leaf_param_init_bool(bme280, GN_BME280_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(bme280, GN_BME280_PARAM_UPDATE_TIME_SEC, 10);

	gn_leaf_config_handle_t ds18b20 = gn_leaf_create(node, "ds18b20",
			gn_ds18b20_config, 4096);
	gn_leaf_param_init_double(ds18b20, GN_DS18B20_PARAM_GPIO, 4);

	//finally, start node
	gn_node_start(node);

	while (true) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGD(TAG, "main loop");
	}

}

int main(int argc, char **argv) {
	app_main();
	return 0;
}
