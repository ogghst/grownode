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
#include "esp_event.h"
#include "esp_check.h"
#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "grownode.h"
#include "gn_mqtt_protocol.h"

static const char *TAG = "gn_mqtt";

#define _GN_MQTT_DEFAULT_QOS 0

#define _GN_MQTT_PAYLOAD_RST "RST"
#define _GN_MQTT_PAYLOAD_OTA "OTA"

EventGroupHandle_t _gn_event_group_mqtt;
const int _GN_MQTT_CONNECTED_OK_EVENT_BIT = BIT0;
const int _GN_MQTT_CONNECTED_KO_EVENT_BIT = BIT1;

gn_config_handle_t _config; //TODO shared pointer, dangerous

char _gn_cmd_topic[GN_MQTT_MAX_TOPIC_LENGTH],
		_gn_sts_topic[GN_MQTT_MAX_TOPIC_LENGTH];

char __mac[13];

void _gn_mqtt_build_leaf_command_topic(gn_leaf_config_handle_t leaf_config,
		char *buf) {

	strncpy(buf, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	snprintf(__mac, 13, "%02X%02X%02X%02X%02X%02X",
			leaf_config->node_config->config->macAddress[0],
			leaf_config->node_config->config->macAddress[1],
			leaf_config->node_config->config->macAddress[2],
			leaf_config->node_config->config->macAddress[3],
			leaf_config->node_config->config->macAddress[4],
			leaf_config->node_config->config->macAddress[5]);
	strncat(buf, __mac, 12);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, GN_MQTT_COMMAND_MESS, GN_MQTT_MAX_TOPIC_LENGTH);
	buf[GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_leaf_status_topic(gn_leaf_config_handle_t leaf_config,
		char *buf) {
	strncpy(buf, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	snprintf(__mac, 13, "%02X%02X%02X%02X%02X%02X",
			leaf_config->node_config->config->macAddress[0],
			leaf_config->node_config->config->macAddress[1],
			leaf_config->node_config->config->macAddress[2],
			leaf_config->node_config->config->macAddress[3],
			leaf_config->node_config->config->macAddress[4],
			leaf_config->node_config->config->macAddress[5]);
	strncat(buf, __mac, 12);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, GN_MQTT_STATUS_MESS, GN_MQTT_MAX_TOPIC_LENGTH);
	buf[GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_status_topic(gn_config_handle_t config, char *buf) {
	strncpy(buf, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	snprintf(__mac, 13, "%02X%02X%02X%02X%02X%02X", config->macAddress[0],
			config->macAddress[1], config->macAddress[2], config->macAddress[3],
			config->macAddress[4], config->macAddress[5]);
	strncat(buf, __mac, 12);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, GN_MQTT_STATUS_MESS, GN_MQTT_MAX_TOPIC_LENGTH);
	buf[GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_command_topic(gn_config_handle_t config, char *buf) {
	strncpy(buf, CONFIG_GROWNODE_MQTT_BASE_TOPIC, GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	snprintf(__mac, 13, "%02X%02X%02X%02X%02X%02X", config->macAddress[0],
			config->macAddress[1], config->macAddress[2], config->macAddress[3],
			config->macAddress[4], config->macAddress[5]);
	strncat(buf, __mac, 12);
	strncat(buf, "/", GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, GN_MQTT_COMMAND_MESS, GN_MQTT_MAX_TOPIC_LENGTH);
	buf[GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

esp_err_t _gn_mqtt_subscribe_leaf(gn_leaf_config_handle_t leaf_config) {

	ESP_LOGI(TAG, "subscribing leaf");

	char topic[GN_MQTT_MAX_TOPIC_LENGTH];
	_gn_mqtt_build_leaf_status_topic(leaf_config, topic);

	ESP_LOGI(TAG, "esp_mqtt_client_subscribe. topic: %s", topic);

	int msg_id = esp_mqtt_client_subscribe(
			leaf_config->node_config->config->mqtt_client, topic, 0);
	ESP_LOGI(TAG, "sent subscribe successful, topic = %s, msg_id=%d", topic,
			msg_id);

	return ESP_OK;

}

esp_err_t _gn_mqtt_send_startup_message(gn_config_handle_t config) {

	//build
	gn_mqtt_startup_message_handle_t msg =
			(gn_mqtt_startup_message_handle_t) malloc(
					sizeof(gn_mqtt_startup_message_t));
	msg->config = config;
	strncpy(msg->topic, _gn_sts_topic, GN_MQTT_MAX_TOPIC_LENGTH);

	//payload
	int msg_id = -1;
	char *buf = (char*) malloc(GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char)); //TODO guess, depending on mqtt message. maybe better go with internal allocation?

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "deviceName", msg->config->deviceName); //TODO grab network name
	if (!cJSON_PrintPreallocated(root, buf, GN_MQTT_MAX_PAYLOAD_LENGTH,
	false)) {
		ESP_LOGE(TAG, "_gn_create_startup_message: cannot print json message");
		goto fail;
		return ESP_FAIL;
	}
	cJSON_Delete(root);

	//publish
	msg_id = esp_mqtt_client_publish(msg->config->mqtt_client, msg->topic, buf,
			0, 2, 0);
	ESP_LOGI(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, msg->topic, buf);

	fail: {
		free(buf);
		free(msg);
		return ((msg_id == -1) ? (ESP_FAIL) : (ESP_OK));
	}
}

esp_err_t _gn_mqtt_on_connected(gn_config_handle_t config) {

	int msg_id = esp_mqtt_client_subscribe(config->mqtt_client, _gn_cmd_topic,
	_GN_MQTT_DEFAULT_QOS);

	if (msg_id == -1) {
		ESP_LOGE(TAG, "error subscribing default topic %s, msg_id=%d",
				CONFIG_GROWNODE_MQTT_BASE_TOPIC, msg_id);
		goto fail;
	}
	ESP_LOGI(TAG, "subscribing default topic %s, msg_id=%d",
			CONFIG_GROWNODE_MQTT_BASE_TOPIC, msg_id);

	//send hello message
	if (ESP_OK != _gn_mqtt_send_startup_message(config)) {
		ESP_LOGE(TAG, "failed to send startup message");
		goto fail;
	}

	if (ESP_OK
			!= esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
					GN_SERVER_CONNECTED_EVENT,
					NULL, 0, portMAX_DELAY)) {
		ESP_LOGE(TAG, "failed to send GN_SERVER_CONNECTED_EVENT event");
		goto fail;
	}

	//stop waiting for mqtt
	return xEventGroupSetBits(_gn_event_group_mqtt,
			_GN_MQTT_CONNECTED_OK_EVENT_BIT);

	fail:
	//stop waiting for mqtt
	xEventGroupSetBits(_gn_event_group_mqtt, _GN_MQTT_CONNECTED_KO_EVENT_BIT);
	return ESP_FAIL;
}

esp_err_t _gn_mqtt_on_disconnected(esp_mqtt_client_handle_t client) {

	if (ESP_OK
			!= esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
					GN_SERVER_DISCONNECTED_EVENT,
					NULL, 0, portMAX_DELAY)) {
		ESP_LOGE(TAG, "failed to send GN_SERVER_DISCONNECTED_EVENT event");
		goto fail;
	}

	fail:
	//stop waiting for mqtt
	xEventGroupSetBits(_gn_event_group_mqtt, _GN_MQTT_CONNECTED_KO_EVENT_BIT);
	return ESP_FAIL;

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
	//int msg_id;
	switch ((esp_mqtt_event_id_t) event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		_gn_mqtt_on_connected(_config); //TODO find a better way to get context, here the event mqtt client is not taken in consideration

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
		_gn_mqtt_on_disconnected(client);
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		//TODO here the code to forward the call to appropriate node/leaf or system handler. start from remote OTA and RST
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
		ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);

		if (strncmp(event->topic, _gn_cmd_topic, event->topic_len) == 0) {
			//device message
			if (strncmp(event->data, _GN_MQTT_PAYLOAD_OTA, event->data_len)
					== 0) {
				//ota message

				esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
						GN_NET_OTA_START, NULL, 0, portMAX_DELAY);

			} else if (strncmp(event->data, _GN_MQTT_PAYLOAD_RST,
					event->data_len) == 0) {
				//rst message

				esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
						GN_NET_RST_START, NULL, 0, portMAX_DELAY);

			}
		}

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
			_gn_mqtt_on_disconnected(client);
		}
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}

}

esp_err_t _gn_mqtt_init(gn_config_handle_t conf) {

	_gn_event_group_mqtt = xEventGroupCreate();

	esp_mqtt_client_config_t mqtt_cfg = { .uri = CONFIG_GROWNODE_MQTT_URL };

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

	if (client == NULL) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_init");
		return ESP_FAIL;
	}

	/* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
	ESP_ERROR_CHECK(
			esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID, _gn_mqtt_event_handler, NULL));

	ESP_LOGI(TAG, "Connecting MQTT server at %s", mqtt_cfg.uri);

	if (esp_mqtt_client_start(client) != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_start");
		return ESP_FAIL;
	}

	conf->mqtt_client = client;

//TODO dangerous, better share config data through events
	_config = conf;

	_gn_mqtt_build_command_topic(_config, _gn_cmd_topic);
	_gn_mqtt_build_status_topic(_config, _gn_sts_topic);

	EventBits_t uxBits;
	uxBits = xEventGroupWaitBits(_gn_event_group_mqtt,
			_GN_MQTT_CONNECTED_OK_EVENT_BIT | _GN_MQTT_CONNECTED_KO_EVENT_BIT,
			pdTRUE,
			pdFALSE, portMAX_DELAY);

	if ((uxBits & _GN_MQTT_CONNECTED_OK_EVENT_BIT) != 0) {
		ESP_LOGI(TAG, "MQTT client handshake successful");
		//publish server connected event
		if (ESP_OK
				!= esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
						GN_SERVER_CONNECTED_EVENT,
						NULL, 0, portMAX_DELAY)) {
			ESP_LOGE(TAG, "failed to send GN_SERVER_CONNECTED_EVENT event");
			goto fail;
		}

		return ESP_OK;
	}

	if ((uxBits & _GN_MQTT_CONNECTED_KO_EVENT_BIT) != 0) {
		ESP_LOGE(TAG, "MQTT client handshake error");
		//publish server disconnected event
		if (ESP_OK
				!= esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
						GN_SERVER_DISCONNECTED_EVENT,
						NULL, 0, portMAX_DELAY)) {
			ESP_LOGE(TAG, "failed to send GN_SERVER_DISCONNECTED_EVENT event");
			return ESP_FAIL;
		}
	}

	fail: return ESP_FAIL;

}

/*
 static void test() {

 for (int i = 0; i < 100000; i++) {

 gn_mqtt_startup_message_handle_t m1 = malloc(
 sizeof(gn_mqtt_startup_message_t));
 m1->topic
 strcpy(m1->nodeName, "test");

 const int len = 100;
 char *deserialize = malloc(sizeof(char) * len);

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
 */

#ifdef __cplusplus
}
#endif

