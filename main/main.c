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
#include "gn_easypot1.h"

#define TASK_STACK_SIZE 8192*4

#define TAG "gn_main"

void app_main(void) {

	//set default log level
	esp_log_level_set("*", ESP_LOG_INFO);

	//set this file log level
	esp_log_level_set("gn_main", ESP_LOG_INFO);

	//core
	esp_log_level_set("grownode", ESP_LOG_INFO);
	esp_log_level_set("gn_commons", ESP_LOG_INFO);
	esp_log_level_set("gn_nvs", ESP_LOG_INFO);
	esp_log_level_set("gn_mqtt_protocol", ESP_LOG_INFO);
	esp_log_level_set("gn_network", ESP_LOG_INFO);
	esp_log_level_set("gn_display", ESP_LOG_INFO);

	//boards
	esp_log_level_set("gn_easypot1", ESP_LOG_INFO);

	gn_config_init_param_t config_init = {
		.provisioning_security = true,
		.provisioning_password = "grownode",
		.wifi_retries_before_reset_provisioning = 5,
		.server_board_id_topic = false,
		.server_base_topic = "/grownode/test",
		.server_url = "mqtt://192.168.1.10:1883",
		.server_keepalive_timer_sec = 60,
		.server_discovery = false,
		.server_discovery_prefix = "homeassistant",
		.firmware_url = "http://myserver/myfirmware.bin",
		.sntp_url = "pool.ntp.org",
		.sleep_delay_millisec = 10000LL,
		.wakeup_time_millisec = 30000LL,
		.sleep_mode = GN_SLEEP_MODE_DEEP
	};

	//creates the config handle
	gn_config_handle_t config = gn_init(&config_init);

	//waits until the config process ends
	while (gn_get_status(config) != GN_NODE_STATUS_READY_TO_START) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "grownode startup status: %s",
				gn_get_status_description(config));
	}

	//creates a new node
	gn_node_handle_t node = gn_node_create(config, "node");

	//the board to start
	gn_configure_easypot1(node);

	//finally, start node
	gn_node_start(node);

	//handles loop
	gn_node_loop(node);


}

int main(int argc, char **argv) {
	app_main();
	return 0;
}
