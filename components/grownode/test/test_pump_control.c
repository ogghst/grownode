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

#include "unity.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "grownode.h"
#include "grownode_intl.h"
#include "gn_mqtt_protocol.h"
#include "gn_pump.h"
#include "gn_ds18b20.h"
#include "gn_capacitive_water_level.h"
#include "gn_pump_control.h"

//static const char TAG[10] = "test_pump";

gn_config_init_param_t config_init = { .provisioning_security = true,
		.provisioning_password = "grownode", .server_board_id_topic = false,
		.server_base_topic = "server_base_topic", .server_url = "server_url",
		.server_keepalive_timer_sec = 60, .server_discovery = false,
		.firmware_url = "firmware_url", .sntp_url = "sntp_url" };

//functions hidden to be tested
void _gn_mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data);

void _gn_mqtt_build_leaf_parameter_command_topic(
		gn_leaf_handle_t _leaf_config, char *param_name, char *buf);

extern gn_config_handle_t config;
extern gn_node_handle_t node_config;

gn_leaf_handle_t pump_config;
gn_leaf_handle_t ds18b20_config;
gn_leaf_handle_t pump_control_config;
gn_leaf_handle_t cwl_config;

TEST_CASE("gn_pump_control_stress_test", "[pump_control]") {

	config = gn_init(&config_init);

	TEST_ASSERT(config != NULL);
	node_config = gn_node_create(config, "node");
	char node_name[GN_LEAF_NAME_SIZE];
	gn_node_get_name(node_config, node_name);
	TEST_ASSERT_EQUAL_STRING("node", node_name);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 0);

	pump_config = gn_leaf_create(node_config, "pump", gn_pump_config, 4096, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 1);

	ds18b20_config = gn_leaf_create(node_config, "ds18b20", gn_ds18b20_config,
			4096, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 2);

	ds18b20_config = gn_leaf_create(node_config, "cwl",
			gn_capacitive_water_level_config, 8192, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 3);

	pump_control_config = gn_leaf_create(node_config, "pump_control",
			gn_pump_control_config, 4096, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 4);

	esp_err_t ret = gn_node_start(node_config);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

	while (true) {

		heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
		vTaskDelay(pdMS_TO_TICKS(30000));

	}

	TEST_ASSERT(true);
}
