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

#include "gn_bh1750.h"
#include "gn_ds18b20.h"
#include "gn_leaf_mux_cms.h"
//#include "gn_capacitive_moisture_sensor.h"

#define TASK_STACK_SIZE 8192*4

#define TAG "gn_main"

#include "esp_check.h"

#include "esp_log.h"


void app_main(void) {

	//set default log level
	esp_log_level_set("*", ESP_LOG_INFO);

	//set this file log level
	esp_log_level_set("gn_main", ESP_LOG_INFO);

	//core
	esp_log_level_set("grownode", ESP_LOG_NONE);
	esp_log_level_set("gn_commons", ESP_LOG_INFO);
	esp_log_level_set("gn_nvs", ESP_LOG_INFO);
	esp_log_level_set("gn_mqtt_protocol", ESP_LOG_INFO);
	esp_log_level_set("gn_network", ESP_LOG_INFO);
	esp_log_level_set("gn_display", ESP_LOG_INFO);

	//leaves
	esp_log_level_set("gn_leaf_bh1750", ESP_LOG_DEBUG);
	esp_log_level_set("gn_leaf_ds18b20", ESP_LOG_DEBUG);
	esp_log_level_set("gn_leaf_mux_cms", ESP_LOG_DEBUG);
//	esp_log_level_set("gn_leaf_cms", ESP_LOG_DEBUG);

	gn_config_init_param_t config_init = {
		.provisioning_security = true,
		.provisioning_password = "grownode",
		.wifi_retries_before_reset_provisioning = 5,
		.server_board_id_topic = false,
		.server_base_topic = "grownode",
		.server_url = "mqtt://192.168.43.251:1883",
		.server_keepalive_timer_sec = 3600,
		.server_discovery = false,
		.server_discovery_prefix = "homeassistant",
		.firmware_url = "http://grownode.duckdns.org/grownode/blink/grownode.bin",
		.sntp_url = "pool.ntp.org",
		.wakeup_time_millisec = 5000LL,
		.sleep_delay_millisec = 50LL,
		.sleep_time_millisec = 10000LL,
		.sleep_mode = GN_SLEEP_MODE_NONE,
		.timezone = "CET-1CEST,M3.5.0,M10.5.0/3"
	};

	//creates the config handle
	gn_config_handle_t config = gn_init(&config_init);

	//waits until the config process ends
	while (gn_get_status(config) != GN_NODE_STATUS_READY_TO_START) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "grownode startup status: %s", gn_get_status_description(config));
	}

	//creates a new node
	gn_node_handle_t node = gn_node_create(config, "balcony");

	gn_leaf_handle_t temps = gn_ds18b20_fastcreate(node, "temps", 18, 20);
	gn_leaf_handle_t illuminance = gn_bh1750_fastcreate(node, "illuminance", 21, 22, 10);

	gn_leaf_handle_t mux_cms = gn_leaf_create(node, "mux_cms", gn_mux_cms_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(mux_cms, GN_MUX_CMS_PARAM_N_SENSORS, 10);
	gn_leaf_param_init_double(mux_cms, GN_MUX_CMS_PARAM_UPDATE_TIME_SEC, 20);

//	gn_leaf_handle_t cms = gn_capacitive_moisture_sensor_fastcreate(node, "cms", 3, 15);

	//finally, start node
	gn_node_start(node);

	//handles loop
	gn_node_loop(node);

}

int main(int argc, char **argv) {
	app_main();
	return 0;
}


//gn_leaf_handle_t illuminance2 = gn_leaf_create(node, "illuminance2",
//			gn_bh1750_config, 4096, GN_LEAF_TASK_PRIORITY);
//gn_leaf_param_init_double(illuminance2, GN_BH1750_PARAM_SDA, 21);
//gn_leaf_param_init_double(illuminance2, GN_BH1750_PARAM_SCL, 22);
//gn_leaf_param_init_bool(illuminance2, GN_BH1750_PARAM_ACTIVE, true);
//gn_leaf_param_init_double(illuminance2, GN_BH1750_PARAM_UPDATE_TIME_SEC, 15);
//
//
//gn_leaf_handle_t temps2 = gn_leaf_create(node, "temps2", gn_ds18b20_config, 4096, GN_LEAF_TASK_PRIORITY);
//gn_leaf_param_init_double(temps2, GN_DS18B20_PARAM_GPIO, 18);
//gn_leaf_param_init_bool(temps2, GN_DS18B20_PARAM_ACTIVE, true);
//gn_leaf_param_init_double(temps2, GN_DS18B20_PARAM_UPDATE_TIME_SEC, 15);




