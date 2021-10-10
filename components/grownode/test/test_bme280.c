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
#include "gn_bme280.h"

//static const char TAG[13] = "test_ds18b20";

//functions hidden to be tested
void _gn_mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data);

void _gn_mqtt_build_leaf_parameter_command_topic(
		const gn_leaf_config_handle_t _leaf_config, const char *param_name, char *buf);

extern gn_config_handle_t config;
extern gn_node_config_handle_t node_config;

gn_leaf_config_handle_t bme280_config;

TEST_CASE("gn_init_add_bme280", "[bme280]") {
	config = gn_init();
	TEST_ASSERT(config != NULL);
	node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL_STRING("node", gn_get_node_config_name(node_config));
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 0);
	bme280_config = gn_leaf_create(node_config, "gn_init_add_bme280", gn_bme280_task, 4096);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 1);
	esp_err_t ret = gn_node_start(node_config);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

}

TEST_CASE("gn_leaf_create bme280", "[bme280]") {

	size_t oldsize = gn_node_get_size(node_config);
	bme280_config = gn_leaf_create(node_config, "bme280", gn_bme280_task, 4096);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), oldsize+1);
	TEST_ASSERT(bme280_config != NULL);

}

TEST_CASE("gn_bme280_mqtt_stress_test", "[bme280]") {

	config = gn_init();
	TEST_ASSERT(config != NULL);
	node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL_STRING("node", gn_get_node_config_name(node_config));
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 0);
	bme280_config = gn_leaf_create(node_config, "bme280", gn_bme280_task, 4096);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 1);
	esp_err_t ret = gn_node_start(node_config);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	event->event_id = event_id;
	char *topic = malloc(100 * sizeof(char));
	esp_event_base_t base = "base";
	void *handler_args = 0;
	char data[10];

	event->topic = (char*) malloc(100 * sizeof(char));
	event->data = (char*) malloc(10 * sizeof(char));

	//set update time
	_gn_mqtt_build_leaf_parameter_command_topic(ds18b20_config,
			GN_BME280_PARAM_UPDATE_TIME_SEC, topic);
	strcpy(data, "5");
	event->topic_len = strlen(topic);
	event->data_len = strlen(data);
	_gn_mqtt_event_handler(handler_args, base, event_id, event);

	free(event->topic);
	free(event->data);
	free(event);
	free(topic);

	vTaskDelay(10000 / portTICK_PERIOD_MS);

	TEST_ASSERT(true);
}
