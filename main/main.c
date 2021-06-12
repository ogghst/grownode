#include <stdio.h>

#include "esp_event.h"

#include "grownode.h"
#include "gn_pump.h"
#include "gn_ds18b20.h"

//#include "GrowNodeController.h"

#define TASK_STACK_SIZE 8192*4

void app_main(void) {

	//GrowNode::Controller::GrowNodeController c;

	gn_config_handle_t config = gn_init();

	while (config->status != GN_CONFIG_STATUS_OK) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "grownode status not OK: %d", config->status);
	}

	gn_node_config_handle_t node_config = gn_node_create(config, "node");

	if (node_config == NULL) {
		ESP_LOGE("main", "node creation error");
		goto fail;
	}

	ESP_LOGI("main", "gn_create_node, name %s", node_config->name);

	gn_message_display("initialized");

	char buf[20];

	sprintf(buf, "pump");

	//create new leaf, controlling pump
	gn_leaf_config_handle_t pump_config = gn_leaf_create(node_config, buf,
			gn_pump_loop, gn_pump_display_config, gn_pump_display_task);

	if (pump_config == NULL) {
		ESP_LOGE("main", "leaf creation error");
		goto fail;
	}

	ESP_LOGI("main", "gn_create_leaf number - name %s on node %s",
			pump_config->name, pump_config->node_config->name);

	sprintf(buf, "ds18b20");

	//create new leaf, rading temp
	gn_leaf_config_handle_t temp_config = gn_leaf_create(node_config, buf,
			gn_ds18b20_loop, NULL, NULL);

	if (temp_config == NULL) {
		ESP_LOGE("main", "leaf creation error");
		goto fail;
	}

	ESP_LOGI("main", "gn_create_leaf number - name %s on node %s",
			temp_config->name, temp_config->node_config->name);


	//finally, start node
	ESP_ERROR_CHECK(gn_node_start(node_config));


	fail:

	while (true) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "main loop");
	}

}
