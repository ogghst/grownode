/*
 * gn_mqtt_protocol.c
 *
 *  Created on: 20 apr 2021
 *      Author: muratori.n
 */



#ifdef __cplusplus
extern "C" {
#endif


#include "esp_log.h"
#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "grownode.h"
#include "gn_mqtt_protocol.h"

static const char *TAG = "gn_mqtt";

#define GN_MQTT_DEFAULT_QOS 0

void _gn_mqtt_build_command_topic(gn_leaf_config_handle_t leaf_config,
		char *buf) {

	strncpy(buf, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, GN_MQTT_COMMAND_MESS, GN_MQTT_MAX_TOPIC_LENGTH);
	buf[GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_status_topic(gn_leaf_config_handle_t leaf_config, char *buf) {

	ESP_LOGI(TAG, "_gn_mqtt_build_status_topic");

	strncpy(buf, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, GN_MQTT_STATUS_MESS, GN_MQTT_MAX_TOPIC_LENGTH);
	buf[GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

esp_err_t _gn_mqtt_subscribe_leaf(gn_leaf_config_handle_t leaf_config) {

	ESP_LOGI(TAG, "subscribing leaf");

	char topic[GN_MQTT_MAX_TOPIC_LENGTH];
	_gn_mqtt_build_status_topic(leaf_config, topic);

	ESP_LOGI(TAG, "esp_mqtt_client_subscribe. topic: %s", topic);

	int msg_id = esp_mqtt_client_subscribe(
			leaf_config->node_config->config->mqtt_client, topic, 0);
	ESP_LOGI(TAG, "sent subscribe successful, topic = %s, msg_id=%d", topic,
			msg_id);

	return ESP_OK;

}

esp_err_t _gn_create_startup_message (char* buf, int len, gn_mqtt_startup_message_handle_t msg) {

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "nodeid", msg->nodeid);
	cJSON_AddStringToObject(root, "nodeName", msg->nodeName);

	cJSON_PrintPreallocated(root, buf, len, false);
	//ESP_LOGI(TAG, "my_json_string\n%s", my_json_string);
	cJSON_Delete(root);

	return ESP_OK;

}

esp_err_t _gn_send_startup_message(gn_mqtt_startup_message_handle_t msg) {
	return ESP_OK;
}

esp_err_t _gn_mqtt_on_connected(esp_mqtt_client_handle_t client) {
	int msg_id = esp_mqtt_client_subscribe(client, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_DEFAULT_QOS);

	if (msg_id == -1) {
		ESP_LOGE(TAG, "error subscribing default topic %s, msg_id=%d", CONFIG_GROWNODE_MQTT_BASE_TOPIC, msg_id);
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "subscribing default topic %s, msg_id=%d", CONFIG_GROWNODE_MQTT_BASE_TOPIC, msg_id);
	return ESP_OK;
}

void log_error_if_nonzero(const char *message, int error_code) {
	if (error_code != 0) {
		ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
	}
}

void _gn_mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {

	ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
			event_id);
	esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	switch ((esp_mqtt_event_id_t) event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		gn_log_message("MQTT Connected");
		_gn_mqtt_on_connected(client);
		/*
		msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1,
				0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
		ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
		*/
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		gn_log_message("MQTT Disconnected");
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		/*
		msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0,
				0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		*/
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		printf("DATA=%.*s\r\n", event->data_len, event->data);
		//char buf[30];
		//snprintf(buf, 29, "%.*s\r", event->data_len, event->data);
		//logMessage(buf);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
			log_error_if_nonzero("reported from esp-tls",
					event->error_handle->esp_tls_last_esp_err);
			log_error_if_nonzero("reported from tls stack",
					event->error_handle->esp_tls_stack_err);
			log_error_if_nonzero("captured as transport's socket errno",
					event->error_handle->esp_transport_sock_errno);
			ESP_LOGI(TAG, "Last errno string (%s)",
					strerror(event->error_handle->esp_transport_sock_errno));

		}
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

void _gn_init_mqtt(gn_config_handle_t conf) {

	esp_mqtt_client_config_t mqtt_cfg = { .uri = CONFIG_GROWNODE_MQTT_URL };

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	/* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
	ESP_ERROR_CHECK(
			esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID, _gn_mqtt_event_handler, NULL));
	ESP_ERROR_CHECK(esp_mqtt_client_start(client));

	conf->mqtt_client = client;

}


static void test() {

	for (int i = 0; i < 100000; i++) {

		gn_mqtt_startup_message_handle_t m1 = malloc(sizeof(gn_mqtt_startup_message_t));
		m1->nodeid = i;
		strcpy(m1->nodeName, "test");

		const int len = 100;
		char* deserialize = malloc(sizeof(char)*len);

		_gn_create_startup_message(deserialize, len, m1);

		cJSON *json = cJSON_Parse(deserialize);
		cJSON *_nodeId = cJSON_GetObjectItemCaseSensitive(json, "nodeid");
		cJSON *_nodeName = cJSON_GetObjectItemCaseSensitive(json, "nodeName");

		m1->nodeid = _nodeId->valueint;
		strcpy(m1->nodeName, _nodeName->valuestring);

		cJSON_Delete(json);


		if (i % 1000 == 0) {
			ESP_LOGI(TAG, "%i", i);
			ESP_LOGI(TAG, "-- deserialize '%s'", deserialize);
			ESP_LOGI(TAG, "-- nodeid '%i'", m1->nodeid);
			ESP_LOGI(TAG, "-- nodename '%s'", m1->nodeName);
			vTaskDelay(1);
		}

		free(deserialize);
		free(m1);

		//free(deserialize);

	}

}

#ifdef __cplusplus
}
#endif


