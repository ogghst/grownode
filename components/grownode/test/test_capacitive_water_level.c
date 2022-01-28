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
//#include "gn_mqtt_protocol.h"
#include "gn_capacitive_water_level.h"

//static const char TAG[13] = "test_ds18b20";

//functions hidden to be tested
void _gn_mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data);

void _gn_mqtt_build_leaf_parameter_command_topic(
		const gn_leaf_handle_t _leaf_config, const char *param_name, char *buf);

extern gn_config_handle_t config;
extern gn_node_handle_t node_config;

gn_leaf_handle_t cwl_config;

TEST_CASE("gn_init_add_cwl", "[cwl]") {

	config = gn_init();
	TEST_ASSERT(config != NULL);
	node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL_STRING("node", gn_get_node_config_name(node_config));
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 0);
	cwl_config = gn_leaf_create(node_config, "cwl", gn_capacitive_water_level_task, 4096);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 1);
	esp_err_t ret = gn_node_start(node_config);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

}

TEST_CASE("gn_leaf_create cwl", "[cwl]") {

	size_t oldsize = gn_node_get_size(node_config);
	cwl_config = gn_leaf_create(node_config, "cwl", gn_capacitive_water_level_task, 4096);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), oldsize+1);
	TEST_ASSERT(cwl_config != NULL);

}

