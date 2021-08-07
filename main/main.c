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

#include "esp_event.h"

#include "grownode.h"
#include "gn_pump.h"
#include "gn_ds18b20.h"

#define TASK_STACK_SIZE 8192*4

/*
 * example of main application. configures the node, one simple leaf and starts it.
 */
void app_main(void) {

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

}


int main(int argc, char **argv)
{
    app_main();
    return 0;
}
