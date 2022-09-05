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

static const char *TAG = "test_pump";

gn_config_init_param_t config_init = { .provisioning_security = true,
		.provisioning_password = "grownode", .server_board_id_topic = false,
		.server_base_topic = "server_base_topic", .server_url = "server_url",
		.server_keepalive_timer_sec = 60, .server_discovery = false,
		.firmware_url = "firmware_url", .sntp_url = "sntp_url" };

//functions hidden to be tested
void _gn_mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data);

void _gn_mqtt_build_leaf_parameter_command_topic(
		const gn_leaf_handle_t _leaf_config, const char *param_name,
		char *buf);

extern gn_config_handle_t config;
extern gn_node_handle_t node_config;
gn_leaf_handle_t pump_config;

TEST_CASE("gn_init_add_pump", "[pump]") {

	config = gn_init(&config_init);

	TEST_ASSERT(config != NULL);
	node_config = gn_node_create(config, "node");
	char node_name[GN_LEAF_NAME_SIZE];
	gn_node_get_name(node_config, node_name);
	TEST_ASSERT_EQUAL_STRING("node", node_name);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 0);
	pump_config = gn_leaf_create(node_config, "pump", gn_pump_config, 4096, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 1);
	esp_err_t ret = gn_node_start(node_config);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

}

TEST_CASE("gn_leaf_create pump", "[pump]") {

	size_t oldsize = gn_node_get_size(node_config);
	pump_config = gn_leaf_create(node_config, "pump", gn_pump_config, 4096, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), oldsize + 1);
	TEST_ASSERT(pump_config != NULL);

}

TEST_CASE("gn_receive_status_0", "[pump]") {

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	char *topic = calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));

	_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
			GN_PUMP_PARAM_TOGGLE, topic);

	char data[] = GN_LEAF_MESSAGE_FALSE;
	event->topic = (char*) calloc(strlen(topic), sizeof(char));
	event->data = (char*) calloc(strlen(data), sizeof(char));
	event->topic_len = strlen(topic);
	event->data_len = strlen(data);

	strcpy(event->topic, topic);
	strcpy(event->data, data);

	esp_event_base_t base = "base";
	void *handler_args = 0;

	_gn_mqtt_event_handler(handler_args, base, event_id, event);
	TEST_ASSERT(true);

	free(event);
	free(topic);
	free(event->topic);
	free(event->data);

}

TEST_CASE("gn_receive_status_1", "[pump]") {

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	char *topic = calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));

	_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
			GN_PUMP_PARAM_TOGGLE, topic);

	char data[] = GN_LEAF_MESSAGE_TRUE;
	event->topic = (char*) calloc(strlen(topic), sizeof(char));
	event->data = (char*) calloc(strlen(data), sizeof(char));
	event->topic_len = strlen(topic);
	event->data_len = strlen(data);

	strcpy(event->topic, topic);
	strcpy(event->data, data);

	esp_event_base_t base = "base";
	void *handler_args = 0;

	_gn_mqtt_event_handler(handler_args, base, event_id, event);
	TEST_ASSERT(true);

	free(event);
	free(topic);
	free(event->topic);
	free(event->data);

}

TEST_CASE("gn_receive_power_0", "[pump]") {

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	char *topic = calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));

	_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
			GN_PUMP_PARAM_POWER, topic);

	char data[] = GN_LEAF_MESSAGE_FALSE;
	event->topic = (char*) calloc(strlen(topic), sizeof(char));
	event->data = (char*) calloc(strlen(data), sizeof(char));
	event->topic_len = strlen(topic);
	event->data_len = strlen(data);

	strcpy(event->topic, topic);
	strcpy(event->data, data);

	esp_event_base_t base = "base";
	void *handler_args = 0;

	_gn_mqtt_event_handler(handler_args, base, event_id, event);
	TEST_ASSERT(true);

	free(event);
	free(topic);
	free(event->topic);
	free(event->data);

}

TEST_CASE("gn_receive_power_128", "[pump]") {

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	char *topic = calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));

	_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
			GN_PUMP_PARAM_POWER, topic);

	char data[] = "128";
	event->topic = (char*) calloc(strlen(topic), sizeof(char));
	event->data = (char*) calloc(strlen(data), sizeof(char));
	event->topic_len = strlen(topic);
	event->data_len = strlen(data);

	strcpy(event->topic, topic);
	strcpy(event->data, data);

	esp_event_base_t base = "base";
	void *handler_args = 0;

	_gn_mqtt_event_handler(handler_args, base, event_id, event);
	TEST_ASSERT(true);

	free(event);
	free(topic);
	free(event->topic);
	free(event->data);

}

TEST_CASE("gn_receive_power_500", "[pump]") {

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	char *topic = calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));

	_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
			GN_PUMP_PARAM_POWER, topic);

	char data[] = "500";
	event->topic = (char*) calloc(strlen(topic), sizeof(char));
	event->data = (char*) calloc(strlen(data), sizeof(char));
	event->topic_len = strlen(topic);
	event->data_len = strlen(data);

	strcpy(event->topic, topic);
	strcpy(event->data, data);

	esp_event_base_t base = "base";
	void *handler_args = 0;

	_gn_mqtt_event_handler(handler_args, base, event_id, event);
	TEST_ASSERT(true);

	free(event);
	free(topic);
	free(event->topic);
	free(event->data);

}

TEST_CASE("gn_pump_mqtt_stress_test", "[pump]") {

	config = gn_init(&config_init);
	TEST_ASSERT(config != NULL);
	node_config = gn_node_create(config, "node");
	char node_name[GN_LEAF_NAME_SIZE];
	gn_node_get_name(node_config, node_name);
	TEST_ASSERT_EQUAL_STRING("node", node_name);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 0);
	pump_config = gn_leaf_create(node_config, "pump", gn_pump_config, 4096, 1);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config), 1);
	esp_err_t ret = gn_node_start(node_config);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

	esp_mqtt_event_handle_t event = (esp_mqtt_event_t*) malloc(
			sizeof(esp_mqtt_event_t));
	event->client = ((gn_config_handle_intl_t) config)->mqtt_client;
	esp_mqtt_event_id_t event_id = MQTT_EVENT_DATA;
	event->event_id = event_id;
	char *topic = calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));
	esp_event_base_t base = "base";
	void *handler_args = 0;
	char data[10];

	event->topic = (char*) calloc(_GN_MQTT_MAX_TOPIC_LENGTH, sizeof(char));
	event->data = (char*) calloc(10, sizeof(char));

	for (size_t j = 0; j < 100; j++) {

		for (size_t i = 0; i < 1000; i++) {

			if (i % 10 == 0) {

				_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
						GN_PUMP_PARAM_TOGGLE, topic);

				if (i % 20 == 0) {
					strcpy(data, GN_LEAF_MESSAGE_TRUE);
				} else {
					strcpy(data, GN_LEAF_MESSAGE_FALSE);
				}

			} else {

				_gn_mqtt_build_leaf_parameter_command_topic(pump_config,
						GN_PUMP_PARAM_POWER, topic);
				itoa(i, data, 10);

			}

			event->topic_len = strlen(topic);
			event->data_len = strlen(data);

			strcpy(event->topic, topic);
			strcpy(event->data, data);

			ESP_LOGD(TAG, "sending command - topic %s, data %s", topic, data);

			_gn_mqtt_event_handler(handler_args, base, event_id, event);

			vTaskDelay(pdMS_TO_TICKS(100));

		}
	}

	free(event->topic);
	free(event->data);
	free(event);
	free(topic);

	TEST_ASSERT(true);
}
