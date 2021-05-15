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

	gn_node_config_handle_t node_config = gn_create_node(config, "node");

	if (node_config == NULL) {
		ESP_LOGE("main", "node creation error");
		goto fail;
	}

	ESP_LOGI("main", "gn_create_node, name %s", node_config->name);

	gn_log_message("initialized");

	char buf[20];

	sprintf(buf, "pump");

	//create new leaf, controlling pump
	gn_leaf_config_handle_t pump_config = gn_create_leaf(node_config, buf,
			gn_pump_callback, gn_pump_loop); //TODO add callback for loop?

	if (pump_config == NULL) {
		ESP_LOGE("main", "leaf creation error");
		goto fail;
	}

	ESP_LOGI("main", "gn_create_leaf number - name %s on node %s",
			pump_config->name, pump_config->node_config->name);

	sprintf(buf, "ds18b20");

	//create new leaf, rading temp
	gn_leaf_config_handle_t temp_config = gn_create_leaf(node_config, buf,
			gn_ds18b20_callback, gn_ds18b20_loop); //TODO add callback for loop?

	if (temp_config == NULL) {
		ESP_LOGE("main", "leaf creation error");
		goto fail;
	}

	ESP_LOGI("main", "gn_create_leaf number - name %s on node %s",
			temp_config->name, temp_config->node_config->name);


	//finally, start node
	ESP_ERROR_CHECK(gn_start_node(node_config));


	fail:

	while (true) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "main loop");
	}

}
