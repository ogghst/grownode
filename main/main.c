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

//include the board you want to start here
#include "gn_blink.h"

#define TASK_STACK_SIZE 8192*4

#define TAG "gn_main"

void app_main(void) {

	//set appropriate log level
	esp_log_level_set("*", ESP_LOG_INFO);

	//core
	esp_log_level_set("grownode", ESP_LOG_INFO);
	esp_log_level_set("gn_commons", ESP_LOG_INFO);
	esp_log_level_set("gn_nvs", ESP_LOG_INFO);
	esp_log_level_set("gn_mqtt_protocol", ESP_LOG_INFO);
	esp_log_level_set("gn_network", ESP_LOG_INFO);
	esp_log_level_set("gn_display", ESP_LOG_INFO);

	esp_log_level_set("gn_leaf_relay", ESP_LOG_INFO);

	//boards
	esp_log_level_set("gn_blink", ESP_LOG_INFO);

	gn_config_init_param_t config_init = {
		.provisioning_security = true,
		.provisioning_password = "grownode",
		.server_board_id_topic = false,
		.server_base_topic = "/grownode/test",
		.server_url = "mqtt://discoboy.duckdns.org:11883",
		.server_keepalive_timer_sec = 60,
		.server_discovery = false,
		.server_discovery_prefix = "homeassistant",
		.firmware_url = "http://discoboy.duckdns.org/grownode/hydroboard2/GrowNode_HB2.bin",
		.sntp_url = "pool.ntp.org"
	};

	//creates the config handle
	gn_config_handle_t config = gn_init(&config_init);

	//waits until the config process ends
	while (gn_get_status(config) != GN_CONFIG_STATUS_COMPLETED) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "grownode startup sequence code: %d",
				gn_get_status(config));
	}

	//creates a new node
	gn_node_config_handle_t node = gn_node_create(config, "node");

	gn_configure_blink(node);

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
