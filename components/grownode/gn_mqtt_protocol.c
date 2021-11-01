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

#include "grownode_intl.h"
#include "gn_mqtt_protocol.h"

#define TAG "gn_mqtt_protocol"

#define _GN_MQTT_MAX_TOPIC_LENGTH 80
#define _GN_MQTT_MAX_PAYLOAD_LENGTH 4096

#define _GN_MQTT_COMMAND_MESS "cmd"
#define _GN_MQTT_STATUS_MESS "sts"

#define _GN_MQTT_PAYLOAD_RST "RST"
#define _GN_MQTT_PAYLOAD_OTA "OTA"
#define _GN_MQTT_PAYLOAD_RBT "RBT"

#define _GN_MQTT_DEFAULT_QOS 0

EventGroupHandle_t _gn_event_group_mqtt;
const int _GN_MQTT_CONNECTED_OK_EVENT_BIT = BIT0;
const int _GN_MQTT_CONNECTED_KO_EVENT_BIT = BIT1;

static gn_server_status_t status = GN_SERVER_DISCONNECTED;

gn_config_handle_intl_t _config; //TODO shared pointer, dangerous

char _gn_cmd_topic[_GN_MQTT_MAX_TOPIC_LENGTH],
		_gn_sts_topic[_GN_MQTT_MAX_TOPIC_LENGTH];

char __nodename[13] = "";

typedef struct {
	gn_config_handle_t config;
	char topic[_GN_MQTT_MAX_TOPIC_LENGTH];
} gn_mqtt_startup_message_t;

typedef gn_mqtt_startup_message_t *gn_mqtt_startup_message_handle_t;

typedef struct {
	gn_node_config_handle_t config;
	char topic[_GN_MQTT_MAX_TOPIC_LENGTH];
//char nodeName[30];
} gn_mqtt_node_config_message_t;

typedef gn_mqtt_node_config_message_t *gn_mqtt_node_config_message_handle_t;

inline char* _gn_mqtt_build_node_name(gn_config_handle_intl_t config) {

	if (strcmp(__nodename, "") != 0) {
		return __nodename;
	}

	snprintf(__nodename, 13, "%02X%02X%02X%02X%02X%02X", config->macAddress[0],
			config->macAddress[1], config->macAddress[2], config->macAddress[3],
			config->macAddress[4], config->macAddress[5]);

	return &__nodename[0];

}

/**
 * @brief returns the status of the server
 *
 * @return the current status
 */
gn_server_status_t gn_mqtt_get_status() {
	return status;
}

void _gn_mqtt_build_leaf_command_topic(gn_leaf_config_handle_t _leaf_config,
		char *buf) {

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) _leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	strncpy(buf, config->mqtt_base_topic, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _gn_mqtt_build_node_name(config), 12);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _GN_MQTT_COMMAND_MESS, _GN_MQTT_MAX_TOPIC_LENGTH);
	buf[_GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_leaf_parameter_command_topic(
		const gn_leaf_config_handle_t _leaf_config, const char *param_name,
		char *buf) {

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) _leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	strncpy(buf, config->mqtt_base_topic, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _gn_mqtt_build_node_name(config), 12);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, param_name, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _GN_MQTT_COMMAND_MESS, _GN_MQTT_MAX_TOPIC_LENGTH);
	buf[_GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_leaf_parameter_status_topic(
		gn_leaf_config_handle_t _leaf_config, char *param_name, char *buf) {

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) _leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	strncpy(buf, config->mqtt_base_topic, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _gn_mqtt_build_node_name(config), 12);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, param_name, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _GN_MQTT_STATUS_MESS, _GN_MQTT_MAX_TOPIC_LENGTH);
	buf[_GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_leaf_status_topic(gn_leaf_config_handle_t _leaf_config,
		char *buf) {

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) _leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	strncpy(buf, config->mqtt_base_topic, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _gn_mqtt_build_node_name(config), 12);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, leaf_config->name, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _GN_MQTT_STATUS_MESS, _GN_MQTT_MAX_TOPIC_LENGTH);
	buf[_GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_status_topic(gn_config_handle_intl_t config, char *buf) {

	strncpy(buf, config->mqtt_base_topic, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _gn_mqtt_build_node_name(config), 12);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _GN_MQTT_STATUS_MESS, _GN_MQTT_MAX_TOPIC_LENGTH);
	buf[_GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

void _gn_mqtt_build_command_topic(gn_config_handle_intl_t config, char *buf) {
	strncpy(buf, config->mqtt_base_topic, _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _gn_mqtt_build_node_name(config), 12);
	strncat(buf, "/", _GN_MQTT_MAX_TOPIC_LENGTH);
	strncat(buf, _GN_MQTT_COMMAND_MESS, _GN_MQTT_MAX_TOPIC_LENGTH);
	buf[_GN_MQTT_MAX_TOPIC_LENGTH - 1] = '\0';

}

/*
 * called when a MQTT_EVENT_PUBLISHED is sent.
 * handler_arg is the leaf to be checked against the topic to call his callback
 *
 */
/*
 void _gn_mqtt_on_publish(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {

 esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

 switch (id) {

 case MQTT_EVENT_DATA: {

 gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_arg;

 ESP_LOGD(TAG, "MQTT_EVENT_DATA");
 ESP_LOGD(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
 ESP_LOGD(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
 char *buf = (char*) malloc(sizeof(char)*_GN_MQTT_MAX_TOPIC_LENGTH);
 _gn_mqtt_build_leaf_command_topic(leaf_config, buf);
 if (strncmp(buf, event->topic, event->topic_len) == 0) {
 //callback to leaf
 leaf_config->callback(GN_LEAF_MESSAGE_RECEIVED_EVENT, leaf_config, event); //TODO wrap only event data in a structure
 }
 free(buf);

 break;
 }

 }
 }
 */

/**
 * @brief 	subscribe leaf to the MQTT server in order to receive messages
 *
 * @param leaf_config	the leaf to start
 *
 * @return status of the operation
 */
gn_err_t gn_mqtt_subscribe_leaf(gn_leaf_config_handle_t _leaf_config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) _leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	ESP_LOGD(TAG, "subscribing leaf %s", leaf_config->name);

	char topic[_GN_MQTT_MAX_TOPIC_LENGTH];
	_gn_mqtt_build_leaf_command_topic(leaf_config, topic);

	ESP_LOGD(TAG, "gn_mqtt_subscribe_leaf. topic: %s", topic);

	if (esp_mqtt_client_subscribe(config->mqtt_client, topic, 0) == -1)
		return GN_RET_ERR_MQTT_SUBSCRIBE;

	ESP_LOGD(TAG, "sent subscribe successful, topic = %s", topic);

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_subscribe_leaf_param(gn_leaf_param_handle_t _param) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_leaf_param_handle_intl_t param = (gn_leaf_config_handle_intl_t)_param;
	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) param->leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;


	ESP_LOGD(TAG, "subscribing param %s on %s", param->name, leaf_config->name);

	char topic[_GN_MQTT_MAX_TOPIC_LENGTH];
	_gn_mqtt_build_leaf_parameter_command_topic(leaf_config, param->name,
			topic);

	ESP_LOGD(TAG, "gn_mqtt_subscribe_leaf_param. topic: %s", topic);

	int msg_id = esp_mqtt_client_subscribe(config->mqtt_client, topic, 0);
	ESP_LOGD(TAG, "sent subscribe successful, topic = %s, msg_id=%d", topic,
			msg_id);

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

esp_err_t gn_mqtt_send_node_config(gn_node_config_handle_t _node_config) {

	if (!_node_config)
		return ESP_OK;

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	//TODO merge in just one status message to be sent every time there is a change
	int ret = ESP_OK;

	//build
	gn_mqtt_node_config_message_handle_t msg =
			(gn_mqtt_node_config_message_handle_t) malloc(
					sizeof(gn_mqtt_node_config_message_t));

	msg->config = _node_config;
	strncpy(msg->topic, _gn_sts_topic, _GN_MQTT_MAX_TOPIC_LENGTH);

	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) _node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	//payload
	int msg_id = -1;
	char *buf = (char*) malloc(_GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char));

	cJSON *root, *leaves, *leaf, *leaf_name, *leaf_params, *leaf_param;
	root = cJSON_CreateObject();

	ESP_LOGD(TAG, "gn_mqtt_send_node_config - building node config: %s",
			node_config->name);

	cJSON_AddStringToObject(root, "msgtype", "config");
	cJSON_AddStringToObject(root, "name", node_config->name);
	leaves = cJSON_CreateArray();

	cJSON_AddItemToObject(root, "leaves", leaves);
	for (int i = 0; i < node_config->leaves.last; i++) {
		gn_leaf_config_handle_intl_t _leaf_config =
				(gn_leaf_config_handle_intl_t) node_config->leaves.at[i];
		leaf = cJSON_CreateObject();
		cJSON_AddItemToArray(leaves, leaf);
		ESP_LOGD(TAG, "gn_mqtt_send_node_config - parsing leaf: %s",
				_leaf_config->name);
		leaf_name = cJSON_CreateString(_leaf_config->name);
		cJSON_AddItemToObject(leaf, "name", leaf_name);

		gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
				_leaf_config);
		cJSON_AddStringToObject(leaf, "leaf_type", descriptor->type);

		leaf_params = cJSON_CreateArray();
		cJSON_AddItemToObject(leaf, "params", leaf_params);

		gn_leaf_param_handle_intl_t _param = (gn_leaf_param_handle_intl_t)_leaf_config->params;
		//ESP_LOGD(TAG, "gn_mqtt_send_node_config - check parameter: %p", _param);
		//ESP_LOGD(TAG, "gn_mqtt_send_node_config - building parameter: %s", _param->name);
		while (_param) {

			leaf_param = cJSON_CreateObject();
			cJSON_AddItemToArray(leaf_params, leaf_param);
			//leaf_param_name = cJSON_CreateString(_param->name);
			cJSON_AddStringToObject(leaf_param, "name", _param->name);
			//leaf_param_val = cJSON_CreateString(_param->param_val->val.s);
			switch (_param->param_val->t) {
			case GN_VAL_TYPE_STRING:
				cJSON_AddStringToObject(leaf_param, "type", "string");
				cJSON_AddStringToObject(leaf_param, "val",
						_param->param_val->v.s);
				break;
			case GN_VAL_TYPE_DOUBLE:
				cJSON_AddStringToObject(leaf_param, "type", "number");
				cJSON_AddNumberToObject(leaf_param, "val",
						_param->param_val->v.d);
				break;
			case GN_VAL_TYPE_BOOLEAN:
				cJSON_AddStringToObject(leaf_param, "type", "bool");
				cJSON_AddBoolToObject(leaf_param, "val",
						_param->param_val->v.b);
				break;
			default:
				ESP_LOGE(TAG, "parameter type not handled");
				goto fail;
				break;

			}
			_param = _param->next;
		}

	}
	if (!cJSON_PrintPreallocated(root, buf, _GN_MQTT_MAX_PAYLOAD_LENGTH,
	false)) {
		ESP_LOGE(TAG, "gn_mqtt_send_node_config: cannot print json message");
		goto fail;
		return ESP_FAIL;
	}

	cJSON_Delete(root);

	//publish
	msg_id = esp_mqtt_client_publish(config->mqtt_client, msg->topic, buf, 0, 0,
			1);
	ESP_LOGD(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, msg->topic, buf);

	fail: {
		free(buf);
		free(msg);
		return ((msg_id == -1) ? (ESP_FAIL) : (ESP_OK));
	}

	return ret;

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_send_leaf_param(gn_leaf_param_handle_t _param) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED


	gn_leaf_param_handle_intl_t param = (gn_leaf_param_handle_intl_t)_param;

	if (!param) {
		return GN_RET_ERR_INVALID_ARG;
	}


	ESP_LOGD(TAG, "gn_mqtt_send_leaf_param %s", param->name);

	int ret = GN_RET_OK;

	int msg_id = -1;
	char _topic[_GN_MQTT_MAX_TOPIC_LENGTH];
	char *buf = (char*) malloc(_GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char));
	//char dbuf[30]; //TODO get max double length

	_gn_mqtt_build_leaf_parameter_status_topic(param->leaf_config, param->name,
			_topic);

	switch (param->param_val->t) {
	case GN_VAL_TYPE_BOOLEAN:
		if (param->param_val->v.b) {
			strcpy(buf, "1");
		} else {
			strcpy(buf, "0");
		}
		break;
	case GN_VAL_TYPE_STRING:
		strncpy(buf, param->param_val->v.s, strlen(param->param_val->v.s) + 1);
		break;
	case GN_VAL_TYPE_DOUBLE:

		snprintf(buf, 31, "%f", param->param_val->v.d);
		//strncpy(buf, dbuf, 31);
		break;
	default:
		ESP_LOGE(TAG, "unhandled parameter type");
		goto fail;
		break;
	}

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) param->leaf_config;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	//publish
	msg_id = esp_mqtt_client_publish(config->mqtt_client, _topic, buf, 0, 0, 0);
	ESP_LOGD(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, _topic, buf);

	fail: {
		free(buf);
		return ((msg_id == -1) ? (GN_RET_ERR) : (GN_RET_OK));
	}

	return ret;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_send_startup_message(gn_config_handle_t _config) {

	//TODO change with a better implementation with a specific message type
	//return gn_mqtt_send_node_config(((gn_config_handle_intl_t)_config)->node_config);

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t config = (gn_config_handle_intl_t) _config;

	//build
	gn_mqtt_startup_message_handle_t msg =
			(gn_mqtt_startup_message_handle_t) malloc(
					sizeof(gn_mqtt_startup_message_t));

	msg->config = config;
	strncpy(msg->topic, _gn_sts_topic, _GN_MQTT_MAX_TOPIC_LENGTH);

	//payload
	int msg_id = -1;
	char *buf = (char*) malloc(_GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char));

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "msgtype", "online");
	if (!cJSON_PrintPreallocated(root, buf, _GN_MQTT_MAX_PAYLOAD_LENGTH,
	false)) {
		ESP_LOGE(TAG, "cannot print json message");
		goto fail;
		return GN_RET_ERR;
	}
	cJSON_Delete(root);

	//publish
	msg_id = esp_mqtt_client_publish(config->mqtt_client, msg->topic, buf, 0, 0,
			0);
	ESP_LOGD(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, msg->topic, buf);

	fail: {
		free(buf);
		free(msg);
		return ((msg_id == -1) ? (GN_RET_ERR) : (GN_RET_OK));
	}

#else
	return GN_RET_OK;
#endif // CONFIG_GROWNODE_WIFI_ENABLED

}

gn_err_t gn_mqtt_send_reboot_message(gn_config_handle_t _config) {

	//TODO change with a better implementation with a specific message type
	//return gn_mqtt_send_node_config(((gn_config_handle_intl_t)_config)->node_config);

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t config = (gn_config_handle_intl_t) _config;

	//build
	gn_mqtt_startup_message_handle_t msg =
			(gn_mqtt_startup_message_handle_t) malloc(
					sizeof(gn_mqtt_startup_message_t));

	msg->config = config;
	strncpy(msg->topic, _gn_sts_topic, _GN_MQTT_MAX_TOPIC_LENGTH);

	//payload
	int msg_id = -1;
	char *buf = (char*) malloc(_GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char));

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "msgtype", "rbt");
	if (!cJSON_PrintPreallocated(root, buf, _GN_MQTT_MAX_PAYLOAD_LENGTH,
	false)) {
		ESP_LOGE(TAG, "cannot print json message");
		goto fail;
		return GN_RET_ERR;
	}
	cJSON_Delete(root);

	//publish
	msg_id = esp_mqtt_client_publish(config->mqtt_client, msg->topic, buf, 0, 0,
			0);
	ESP_LOGD(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, msg->topic, buf);

	fail: {
		free(buf);
		free(msg);
		return ((msg_id == -1) ? (GN_RET_ERR) : (GN_RET_OK));
	}

#else
	return GN_RET_OK;
#endif // CONFIG_GROWNODE_WIFI_ENABLED

}

gn_err_t gn_mqtt_send_reset_message(gn_config_handle_t _config) {

	//TODO change with a better implementation with a specific message type
	//return gn_mqtt_send_node_config(((gn_config_handle_intl_t)_config)->node_config);

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t config = (gn_config_handle_intl_t) _config;

	//build
	gn_mqtt_startup_message_handle_t msg =
			(gn_mqtt_startup_message_handle_t) malloc(
					sizeof(gn_mqtt_startup_message_t));

	msg->config = config;
	strncpy(msg->topic, _gn_sts_topic, _GN_MQTT_MAX_TOPIC_LENGTH);

	//payload
	int msg_id = -1;
	char *buf = (char*) malloc(_GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char));

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "msgtype", "rst");
	if (!cJSON_PrintPreallocated(root, buf, _GN_MQTT_MAX_PAYLOAD_LENGTH,
	false)) {
		ESP_LOGE(TAG, "cannot print json message");
		goto fail;
		return GN_RET_ERR;
	}
	cJSON_Delete(root);

	//publish
	msg_id = esp_mqtt_client_publish(config->mqtt_client, msg->topic, buf, 0, 0,
			0);
	ESP_LOGD(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, msg->topic, buf);

	fail: {
		free(buf);
		free(msg);
		return ((msg_id == -1) ? (GN_RET_ERR) : (GN_RET_OK));
	}

#else
	return GN_RET_OK;
#endif // CONFIG_GROWNODE_WIFI_ENABLED

}

gn_err_t gn_mqtt_send_ota_message(gn_config_handle_t _config) {

	//TODO change with a better implementation with a specific message type
	//return gn_mqtt_send_node_config(((gn_config_handle_intl_t)_config)->node_config);

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t config = (gn_config_handle_intl_t) _config;

	//build
	gn_mqtt_startup_message_handle_t msg =
			(gn_mqtt_startup_message_handle_t) malloc(
					sizeof(gn_mqtt_startup_message_t));

	msg->config = config;
	strncpy(msg->topic, _gn_sts_topic, _GN_MQTT_MAX_TOPIC_LENGTH);

	//payload
	int msg_id = -1;
	char *buf = (char*) malloc(_GN_MQTT_MAX_PAYLOAD_LENGTH * sizeof(char));

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "msgtype", "ota");
	if (!cJSON_PrintPreallocated(root, buf, _GN_MQTT_MAX_PAYLOAD_LENGTH,
	false)) {
		ESP_LOGE(TAG, "cannot print json message");
		goto fail;
		return GN_RET_ERR;
	}
	cJSON_Delete(root);

	//publish
	msg_id = esp_mqtt_client_publish(config->mqtt_client, msg->topic, buf, 0, 0,
			0);
	ESP_LOGD(TAG, "sent publish successful, msg_id=%d, topic=%s, payload=%s",
			msg_id, msg->topic, buf);

	fail: {
		free(buf);
		free(msg);
		return ((msg_id == -1) ? (GN_RET_ERR) : (GN_RET_OK));
	}

#else
	return GN_RET_OK;
#endif // CONFIG_GROWNODE_WIFI_ENABLED

}

gn_err_t gn_mqtt_send_leaf_message(gn_leaf_config_handle_t _leaf,
		const char *msg) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) _leaf;
	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) leaf_config->node_config;
	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) node_config->config;

	//get the topic
	char buf[_GN_MQTT_MAX_TOPIC_LENGTH];
	_gn_mqtt_build_command_topic(node_config->config, buf);

	//publish
	ESP_LOGD(TAG, "publish topic %s, msg=%s", buf, msg);
	int msg_id = esp_mqtt_client_publish(config->mqtt_client, buf, msg, 0, 0,
			0);

	if (msg_id == -1)
		return GN_RET_ERR;

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

esp_err_t _gn_mqtt_on_connected(esp_mqtt_client_handle_t client) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	int msg_id = esp_mqtt_client_subscribe(_config->mqtt_client, _gn_cmd_topic,
	_GN_MQTT_DEFAULT_QOS);

	if (msg_id == -1) {
		ESP_LOGE(TAG, "error subscribing default topic %s, msg_id=%d",
				_gn_cmd_topic, msg_id);
		goto fail;
	}
	ESP_LOGD(TAG, "subscribing default topic %s, msg_id=%d", _gn_cmd_topic,
			msg_id);

	//send hello message
	if (ESP_OK != gn_mqtt_send_startup_message(_config)) {
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

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

esp_err_t _gn_mqtt_on_disconnected(esp_mqtt_client_handle_t client) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

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

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

void log_error_if_nonzero(const char *message, int error_code) {
	if (error_code != 0) {
		ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
	}
}

void _gn_mqtt_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
			event_id);
	esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
	esp_mqtt_client_handle_t client = event->client;
	//int msg_id;
	switch ((esp_mqtt_event_id_t) event_id) {
	case MQTT_EVENT_CONNECTED:
		status = GN_SERVER_CONNECTED;
		ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
		_gn_mqtt_on_connected(client); //TODO find a better way to get context, here the event mqtt client is not taken in consideration

		/*
		 msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1,
		 0);
		 ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);

		 msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
		 ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		 msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
		 ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		 msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
		 ESP_LOGD(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
		 */
		break;
	case MQTT_EVENT_DISCONNECTED:
		status = GN_SERVER_DISCONNECTED;
		ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
		_gn_mqtt_on_disconnected(client);
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		//TODO here the code to forward the call to appropriate node/leaf or system handler. start from remote OTA and RST
		ESP_LOGD(TAG, "MQTT_EVENT_DATA");
		ESP_LOGD(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
		ESP_LOGD(TAG, "DATA=%.*s\r\n", event->data_len, event->data);

		//device command topic
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

			} else if (strncmp(event->data, _GN_MQTT_PAYLOAD_RBT,
					event->data_len) == 0) {
				//rst message

				esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
						GN_NET_RBT_START, NULL, 0, portMAX_DELAY);

			}

		} else {
			//forward message to the appropriate leaf
			char leaf_topic[_GN_MQTT_MAX_TOPIC_LENGTH];
			char param_topic[_GN_MQTT_MAX_TOPIC_LENGTH];
			gn_leaf_event_t evt;

			for (int i = 0; i < _config->node_config->leaves.last; i++) {

				//message is for this leaf
				_gn_mqtt_build_leaf_command_topic(
						_config->node_config->leaves.at[i], leaf_topic);
				if (strncmp(leaf_topic, event->topic, event->topic_len) == 0) {

					evt.id = GN_LEAF_MESSAGE_RECEIVED_EVENT;
					strncpy(evt.leaf_name,
							_config->node_config->leaves.at[i]->name,
							GN_LEAF_NAME_SIZE);
					//evt.data = event->data;
					memcpy(&evt.data[0], event->data,
							event->data_len > GN_LEAF_DATA_SIZE ?
									GN_LEAF_DATA_SIZE : event->data_len);
					evt.data_size =
							event->data_len > GN_LEAF_DATA_SIZE ?
									GN_LEAF_DATA_SIZE : event->data_len;

					if (esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
							evt.id, &evt, sizeof(evt), 0) != ESP_OK) {
						ESP_LOGE(TAG, "not possible to send message to leaf %s",
								_config->node_config->leaves.at[i]->name);
					}

					//send message to the interested leaf
					_gn_send_event_to_leaf(_config->node_config->leaves.at[i],
							&evt);

					break;
				}

				gn_leaf_param_handle_intl_t _param =
						(gn_leaf_param_handle_intl_t)_config->node_config->leaves.at[i]->params;
				while (_param) {
					//message is for a parameter of this leaf
					_gn_mqtt_build_leaf_parameter_command_topic(
							_config->node_config->leaves.at[i], _param->name,
							param_topic);
					if (strncmp(param_topic, event->topic, event->topic_len)
							== 0) {

						if (GN_RET_OK
								!= _gn_leaf_parameter_update(
										_config->node_config->leaves.at[i],
										_param->name, event->data,
										event->data_len)) {
							ESP_LOGE(TAG,
									"error in updating parameter %s with value %s to leaf %s",
									_param->name, event->data,
									_config->node_config->leaves.at[i]->name);
							break;
						}

						/*
						 evt.id = GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT;
						 strncpy(evt.leaf_name,
						 _config->node_config->leaves.at[i]->name,
						 GN_LEAF_NAME_SIZE);
						 strncpy(evt.param_name, _param->name,
						 GN_LEAF_PARAM_NAME_SIZE);
						 evt.data = event->data;
						 evt.data_size = event->data_len;

						 //send event to the whole node
						 if (esp_event_post_to(_config->event_loop,
						 GN_BASE_EVENT, evt.id, &evt, sizeof(evt),
						 0) != ESP_OK) {
						 ESP_LOGE(TAG,
						 "not possible to send param message to event loop");
						 }
						 */

						break;
					}
					_param = _param->next;
				}

			}

			break;

		}

		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
			log_error_if_nonzero("reported from esp-tls",
					event->error_handle->esp_tls_last_esp_err);
			log_error_if_nonzero("reported from tls stack",
					event->error_handle->esp_tls_stack_err);
			log_error_if_nonzero("captured as transport's socket errno",
					event->error_handle->esp_transport_sock_errno);
			ESP_LOGD(TAG, "Last errno string (%s)",
					strerror(event->error_handle->esp_transport_sock_errno));
			_gn_mqtt_on_disconnected(client);
		}
		break;
	default:
		ESP_LOGD(TAG, "Other event id:%d", event->event_id);
		break;
	}

#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

esp_err_t gn_mqtt_init(gn_config_handle_t _conf) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t conf = (gn_config_handle_intl_t) _conf;

	_gn_event_group_mqtt = xEventGroupCreate();

	char _topic[_GN_MQTT_MAX_TOPIC_LENGTH];
	_gn_mqtt_build_status_topic(_conf, _topic);

	esp_mqtt_client_config_t mqtt_cfg = {
			.uri = conf->mqtt_url,
			.lwt_topic = _topic,
			.lwt_msg = "{\"msgtype\": \"offline\"}",
			.lwt_qos = 1,
			.lwt_retain = 1,
			.buffer_size = 4096, };

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

		/*
		 //publish server connected event
		 if (ESP_OK
		 != esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
		 GN_SERVER_CONNECTED_EVENT,
		 NULL, 0, portMAX_DELAY)) {
		 ESP_LOGE(TAG, "failed to send GN_SERVER_CONNECTED_EVENT event");
		 goto fail;
		 }
		 */

		return ESP_OK;
	}

	if ((uxBits & _GN_MQTT_CONNECTED_KO_EVENT_BIT) != 0) {
		ESP_LOGE(TAG, "MQTT client handshake error");
		//publish server disconnected event
		/*
		 if (ESP_OK
		 != esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
		 GN_SERVER_DISCONNECTED_EVENT,
		 NULL, 0, portMAX_DELAY)) {
		 ESP_LOGE(TAG, "failed to send GN_SERVER_DISCONNECTED_EVENT event");
		 return ESP_FAIL;
		 }
		 */
	}

	//TODO start keepalive service

	//fail: return ESP_FAIL;
	ESP_LOGE(TAG, "MQTT client error: should never reach here!");//TODO improve flow
	return ESP_FAIL;

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

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

