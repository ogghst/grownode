#include <stdio.h>

#include "esp_event.h"

#include "grownode.h"
#include "gn_pump.h"

//#include "GrowNodeController.h"

#define TASK_STACK_SIZE 8192*4

void app_main(void) {

	//GrowNode::Controller::GrowNodeController c;

	gn_config_handle_t config = gn_init();

	while (config->status != GN_CONFIG_STATUS_OK) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "main loop");
	}

	gn_log_message("initialized");

	char *c = malloc(sizeof(char) * 100);

	for (int i = 0; i < 2; i++) {

		gn_node_config_handle_t node_config = gn_create_node(config, "node");

		if (node_config == NULL)
			break;

		ESP_LOGI("main", "gn_create_node %d, name %s", i, node_config->name);

		//create new leaf, controlling pump
		gn_leaf_config_handle_t pump_config = gn_create_leaf(node_config,
				"pump", gn_pump_callback);

		ESP_LOGI("main", "gn_create_leaf %s on node %i with name %s",
				pump_config->name, i, pump_config->node_config->name);

		gn_pump_init(pump_config);
		gn_log_message("Created Pump");

	}

	free(c);

	while (true) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
		ESP_LOGI("main", "main loop");
	}

}
