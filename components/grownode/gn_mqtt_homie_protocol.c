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

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_check.h"

#include "grownode_intl.h"
#include "gn_commons.h"
#include "gn_mqtt_protocol.h"
#include "gn_network.h"

#ifdef CONFIG_GROWNODE_MQTT_HOMIE_PROTOCOL

#define TAG "gn_mqtt_homie_protocol"

static const char *_GN_HOMIE_VERSION = "4.0.0";
static const int _GN_HOMIE_REFRESH_INTERVAL_MS = 30000;

static const char *_GN_HOMIE_INIT = "init";
static const char *_GN_HOMIE_READY = "ready";
static const char *_GN_HOMIE_DISCONNECTED = "disconnected";
static const char *_GN_HOMIE_SLEEPING = "sleeping";
static const char *_GN_HOMIE_LOST = "lost";
static const char *_GN_HOMIE_ALERT = "alert";

static const char *_GN_HOMIE_DATATYPE_INTEGER = "integer";
static const char *_GN_HOMIE_DATATYPE_FLOAT = "float";
static const char *_GN_HOMIE_DATATYPE_BOOLEAN = "boolean";
static const char *_GN_HOMIE_DATATYPE_STRING = "string";
static const char *_GN_HOMIE_DATATYPE_ENUM = "enum";
static const char *_GN_HOMIE_DATATYPE_COLOR = "color";

static const char *_GN_HOMIE_FALSE = "false";
static const char *_GN_HOMIE_TRUE = "true";

EventGroupHandle_t _gn_event_group_mqtt;

const int _GN_MQTT_CONNECTED_EVENT_BIT = BIT0;
const int _GN_MQTT_DISCONNECT_EVENT_BIT = BIT1;

const int _GN_MQTT_DEBUG_WAIT_MS = 500;

/*
 void _gn_homie_topic_node(char *topic, const gn_node_handle_t node) {
 snprintf(topic, _GN_MQTT_MAX_TOPIC_LENGTH, "homie/%s", ((gn_node_handle_intl_t)node)->config->node_handle->name);
 }
 */

void _gn_homie_mk_topic_node_attribute(char *topic,
		const gn_node_handle_intl_t node_handle, char *attribute) {

	//ESP_LOGD(TAG,
	//		"_gn_homie_mk_topic_node_attribute - topic: %s, name = %s, attribute = %s",
	//		topic, node_handle->name, attribute);

	snprintf(topic, _GN_MQTT_MAX_TOPIC_LENGTH, "homie/%s/%s", node_handle->name,
			attribute);

}

/*
 void _gn_homie_topic_leaf(char *topic, const gn_leaf_handle_t leaf) {
 //snprintf(topic, _GN_MQTT_MAX_TOPIC_LENGTH, "homie/%s", ((gn_node_handle_intl_t)node)->config->node_handle->name);
 }
 */

void _gn_homie_mk_topic_leaf_attribute(char *topic,
		const gn_leaf_handle_intl_t leaf, const char *attribute) {

	snprintf(topic, _GN_MQTT_MAX_TOPIC_LENGTH, "homie/%s/%s/%s",
			leaf->node->name, leaf->name, attribute);

}

void _gn_homie_mk_topic_param_attribute(char *topic,
		const gn_leaf_param_handle_intl_t param, const char *attribute) {

	gn_leaf_handle_intl_t _leaf = (gn_leaf_handle_intl_t) param->leaf;
	snprintf(topic, _GN_MQTT_MAX_TOPIC_LENGTH, "homie/%s/%s/%s/%s",
			_leaf->node->name, _leaf->name, param->name, attribute);

}

inline gn_err_t _gn_homie_publish(gn_node_handle_intl_t node, const char *topic,
		int qos, int retain, const char *payload, int len) {

	ESP_LOGD(TAG, "publishing: '%s' -> '%.*s', qos=%d, retained=%d", topic, len,
			payload, qos, retain);

	//vTaskDelay(5000 / portTICK_PERIOD_MS);

	return (esp_mqtt_client_publish(node->config->mqtt_client, topic, payload,
			len, qos, retain) == -1 ? GN_RET_ERR : GN_RET_OK);

}

gn_err_t _gn_homie_publish_int(gn_node_handle_intl_t node, const char *topic,
		int qos, int retain, int payload) {

	char buf[32];
	sprintf(buf, "%d", payload);
	return _gn_homie_publish(node, topic, qos, retain, buf, strlen(buf));

}

gn_err_t _gn_homie_publish_double(gn_node_handle_intl_t node, const char *topic,
		int qos, int retain, double payload) {

	char buf[32];
	sprintf(buf, "%g", payload);
	return _gn_homie_publish(node, topic, qos, retain, buf, strlen(buf));

}

gn_err_t _gn_homie_publish_bool(gn_node_handle_intl_t node, const char *topic,
		int qos, int retain, bool payload) {

	char buf[8];
	strcpy(buf, payload ? "true" : "false");
	return _gn_homie_publish(node, topic, qos, retain, buf, strlen(buf));

}

gn_err_t _gn_homie_publish_str(gn_node_handle_intl_t node, const char *topic,
		int qos, int retain, const char *payload) {

	return _gn_homie_publish(node, topic, qos, retain, payload, strlen(payload));

}

gn_err_t _gn_homie_on_disconnected(gn_config_handle_t config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	ESP_LOGD(TAG, "_gn_homie_on_disconnected");

	if (!config) {
		return GN_RET_ERR_INVALID_ARG;
	}

	gn_config_handle_intl_t _config = (gn_config_handle_intl_t) config;

	if (ESP_OK
			!= esp_event_post_to(_config->event_loop, GN_BASE_EVENT,
					GN_SRV_DISCONNECTED_EVENT,
					NULL, 0, portMAX_DELAY)) {
		ESP_LOGE(TAG, "failed to send GN_SERVER_DISCONNECTED_EVENT event");
	}

	return GN_RET_OK;

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_leaf_param_handle_intl_t _gn_homie_param_from_set_topic(
		gn_node_handle_intl_t node, char *topic, int topic_len) {

	char param_topic[_GN_MQTT_MAX_TOPIC_LENGTH];
	//per each leaf
	for (int i = 0; i < node->leaves.last; i++) {
		gn_leaf_handle_intl_t leaf = (gn_leaf_handle_intl_t) node->leaves.at[i];

		//per each parameter
		gn_leaf_param_handle_intl_t _param_enum = leaf->params;
		while (_param_enum) {

			//check if topic matches the mask and return it
			_gn_homie_mk_topic_param_attribute(param_topic, _param_enum, "set");

			ESP_LOGD(TAG,
					"_gn_homie_param_from_set_topic: comparing topic '%.*s' with param '%s'..",
					topic_len, topic, param_topic);

			if (strncmp(param_topic, topic, topic_len) == 0) {
				ESP_LOGD(TAG, "..found");
				return _param_enum;
			}

			if (_param_enum->next) {
				_param_enum = _param_enum->next;
			} else {
				break;
			}

		}

	}

	return NULL;
}

void log_error_if_nonzero(const char *message, int error_code) {
	if (error_code != 0) {
		ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
	}
}

gn_err_t _gn_mqtt_homie_payload_to_boolean(bool *_ret,
		esp_mqtt_event_handle_t evt) {

	ESP_LOGD(TAG,
			"_gn_mqtt_homie_payload_to_boolean: mqtt_payload='%.*s', len = %d",
			evt->data_len, evt->data, evt->data_len);

	if (strncasecmp(evt->data, "true", 4) == 0) {
		//memcpy(_ret, &true, sizeof(bool));
		*_ret = true;
	} else if (strncasecmp(evt->data, "false", 5) == 0) {
		//memcpy(_ret, &false, sizeof(bool));
		*_ret = false;
	} else {
		ESP_LOGW(TAG, "_gn_mqtt_homie_payload_to_boolean: invalid payload");
		return GN_RET_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "_gn_mqtt_homie_payload_to_boolean: ret = %d", *_ret);
	return GN_RET_OK;

}

gn_err_t _gn_mqtt_homie_payload_to_double(double *_ret,
		esp_mqtt_event_handle_t evt) {

	ESP_LOGD(TAG,
			"_gn_mqtt_homie_payload_to_double: mqtt_payload='%.*s', len = %d",
			evt->data_len, evt->data, evt->data_len);

	char *payload = calloc(evt->data_len, sizeof(char));
	strncpy(payload, evt->data, evt->data_len);

	char *eptr;
	double result = strtod(payload, &eptr);

	if (result == 0) {
		//If the value provided was out of range, display a warning message
		if (errno == ERANGE) {
			ESP_LOGW(TAG, "_gn_mqtt_homie_payload_to_double: invalid payload");
			free(payload);
			return GN_RET_ERR_INVALID_ARG;
		}
	}

	//memcpy(_ret, evt->data, sizeof(double));
	free(payload);
	return GN_RET_OK;

}

gn_err_t _gn_mqtt_homie_payload_to_string(char *_ret, int _ret_len,
		esp_mqtt_event_handle_t evt) {
	ESP_LOGD(TAG,
			"_gn_mqtt_homie_payload_to_string: mqtt_payload='%.*s', len = %d",
			evt->data_len, evt->data, evt->data_len);
	strncpy(_ret, evt->data, _ret_len);
	return GN_RET_OK;
}

void _gn_homie_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	//ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
	//		event_id);

	esp_mqtt_event_handle_t mqtt_event = (esp_mqtt_event_handle_t) event_data;

	gn_config_handle_intl_t config =
			(gn_config_handle_intl_t) mqtt_event->user_context;

	switch ((esp_mqtt_event_id_t) event_id) {

	case MQTT_EVENT_CONNECTED:
		ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
		xEventGroupSetBits(_gn_event_group_mqtt, _GN_MQTT_CONNECTED_EVENT_BIT);
		break;

	case MQTT_EVENT_DISCONNECTED: {

		EventBits_t uxBits;
		uxBits = xEventGroupWaitBits(_gn_event_group_mqtt,
				_GN_MQTT_CONNECTED_EVENT_BIT,
				pdFALSE,
				pdFALSE, 0);

		if ((uxBits & _GN_MQTT_CONNECTED_EVENT_BIT) == 0) {
			ESP_LOGW(TAG,
					"MQTT_EVENT_DISCONNECTED but MQTT is not connected, discarding");
			break;
		}

		ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
		xEventGroupSetBits(_gn_event_group_mqtt, _GN_MQTT_DISCONNECT_EVENT_BIT);
		_gn_homie_on_disconnected(config);
	}
		break;

	case MQTT_EVENT_DATA:

		ESP_LOGD(TAG, "MQTT_EVENT_DATA");
		//parameter set
		gn_leaf_param_handle_intl_t param = _gn_homie_param_from_set_topic(
				config->node_handle, mqtt_event->topic, mqtt_event->topic_len);

		if (param) {

			//check access
			if (param->access != GN_LEAF_PARAM_ACCESS_ALL) {
				ESP_LOGW(TAG,
						"_gn_homie_event_handler - paramater is not accessible from network, change discarded");
				break;
			}

			gn_leaf_parameter_event_t leaf_event;
			leaf_event.id = GN_LEAF_MESSAGE_RECEIVED_EVENT;
			strncpy(leaf_event.leaf_name,
					((gn_leaf_handle_intl_t) param->leaf)->name,
					GN_LEAF_NAME_SIZE);

			switch (param->param_val->t) {

			case GN_VAL_TYPE_BOOLEAN: {

				bool ret = false;
				if (_gn_mqtt_homie_payload_to_boolean(&ret, mqtt_event)
						!= GN_RET_OK) {
					ESP_LOGW(TAG,
							"_gn_homie_event_handler - boolean payload not allowed");
					break;
				}
				gn_bool_to_event_payload(ret, &leaf_event);

			}

				break;
			case GN_VAL_TYPE_DOUBLE: {

				double ret = 0;
				if (_gn_mqtt_homie_payload_to_double(&ret, mqtt_event)
						!= GN_RET_OK) {
					ESP_LOGW(TAG,
							"_gn_homie_event_handler - double payload not allowed");
					break;
				}
				gn_double_to_event_payload(ret, &leaf_event);
			}

				break;
			case GN_VAL_TYPE_STRING: {

				char *ret = calloc(_GN_MQTT_MAX_PAYLOAD_LENGTH, sizeof(char));
				if (_gn_mqtt_homie_payload_to_string(ret,
						mqtt_event->data_len, mqtt_event) != GN_RET_OK) {
					ESP_LOGW(TAG,
							"_gn_homie_event_handler - string payload not allowed");
					free(ret);
					break;
				}
				gn_string_to_event_payload(ret, &leaf_event, leaf_event.data_len);
				free(ret);
			}
				break;
			default:
				ESP_LOGE(TAG, "parameter type not handled: %d", param->param_val->t);
				return;
			}

			ESP_LOGD(TAG,
					"built event - id: %d, param %s, leaf %s, data '%.*s', len=%d",
					leaf_event.id, leaf_event.param_name, leaf_event.leaf_name,
					leaf_event.data_len, leaf_event.data, leaf_event.data_len);

			if (esp_event_post_to(config->event_loop, GN_BASE_EVENT,
					leaf_event.id, &leaf_event, sizeof(leaf_event),
					portMAX_DELAY) != ESP_OK) {
				ESP_LOGE(TAG, "not possible to send message to leaf %s",
						((gn_leaf_handle_intl_t ) param->leaf)->name);
			}

			//send message to the interested leaf
			//_gn_send_event_to_leaf(((gn_leaf_handle_intl_t) param->leaf),
			//		&leaf_event);

			if (GN_RET_OK
					!= _gn_leaf_parameter_update(param->leaf, param->name,
							leaf_event.data, leaf_event.data_len)) {
				ESP_LOGE(TAG,
						"error in updating parameter %s with value %s to leaf %s",
						param->name, leaf_event.data,
						((gn_leaf_handle_intl_t ) param->leaf)->name);

			}
			break;

		}
		break;

		//TODO
		//ota

		//reset

		//reboot

		//broadcast

		break;

	case MQTT_EVENT_ERROR:
		ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
		if (mqtt_event->error_handle->error_type
				== MQTT_ERROR_TYPE_TCP_TRANSPORT) {
			log_error_if_nonzero("reported from esp-tls",
					mqtt_event->error_handle->esp_tls_last_esp_err);
			log_error_if_nonzero("reported from tls stack",
					mqtt_event->error_handle->esp_tls_stack_err);
			log_error_if_nonzero("captured as transport's socket errno",
					mqtt_event->error_handle->esp_transport_sock_errno);
			ESP_LOGD(TAG, "Last errno string (%s)",
					strerror(
							mqtt_event->error_handle->esp_transport_sock_errno));
			//_gn_mqtt_on_disconnected(config);
		}
		break;

	default:
		//ESP_LOGD(TAG, "Other event id:%d", event->event_id);
		break;
	}

#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

static int _clamp(int n, int lower, int upper) {

	return n <= lower ? lower : n >= upper ? upper : n;

}

//gn_err_t gn_mqtt_publish_node(gn_config_handle_t config) { }

gn_err_t _gn_homie_on_connected(gn_config_handle_intl_t _config,
		char *_topic_buf) {

	ESP_LOGD(TAG, "_gn_homie_on_connected");

//presentation messages
	int ret;

//homie version
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$homie");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			(char*) _GN_HOMIE_VERSION);
	if (ret != GN_RET_OK)
		return ret;

//name
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$name");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			_config->node_handle->name);
	if (ret != GN_RET_OK)
		return ret;

//state
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$state");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			(char*) _GN_HOMIE_INIT);
	if (ret != GN_RET_OK)
		return ret;

//nodes
	char *msg_buf = calloc(
			(_config->node_handle->leaves.last + 1) * GN_LEAF_NAME_SIZE,
			sizeof(char));

	for (int i = 0; i < _config->node_handle->leaves.last; i++) {
		gn_leaf_handle_intl_t leaf =
				(gn_leaf_handle_intl_t) _config->node_handle->leaves.at[i];

		strcat(msg_buf, leaf->name);
		strcat(msg_buf, ",");

	}

	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$nodes");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			msg_buf);
	free(msg_buf);
	if (ret != GN_RET_OK)
		return ret;

//online
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$online");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1, "true");
	if (ret != GN_RET_OK)
		return ret;

//implementation
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$implementation");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			"esp32-idf");
	if (ret != GN_RET_OK)
		return ret;

//implementation
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$implementation/version");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			esp_get_idf_version());
	if (ret != GN_RET_OK)
		return ret;

//extensions
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$extensions");
	ret =
			_gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
					"org.homie.legacy-firmware:0.1.1:[4.x],org.homie.legacy-stats:0.1.1:[4.x]");
	if (ret != GN_RET_OK)
		return ret;

//TODO firmware version
//homie_publish("$fw/name", 0, 1, config->firmware_name, 0);
//homie_publish("$fw/version", 0, 1, config->firmware_version, 0);

//TODO ota
//const esp_partition_t *running_partition = esp_ota_get_running_partition();

//TODO logging

	char mac_address[18];
	char ip_address[16];
	gn_wifi_get_mac(mac_address);
	gn_wifi_get_ip(ip_address);

	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$localip");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			ip_address);
	if (ret != GN_RET_OK)
		return ret;

	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle, "$mac");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			mac_address);
	if (ret != GN_RET_OK)
		return ret;

//node attributes
	for (int i = 0; i < _config->node_handle->leaves.last; i++) {
		gn_leaf_handle_intl_t leaf =
				(gn_leaf_handle_intl_t) _config->node_handle->leaves.at[i];

		//name
		_gn_homie_mk_topic_leaf_attribute(_topic_buf, leaf, "$name");
		ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
				leaf->name);
		if (ret != GN_RET_OK)
			return ret;

		//type
		_gn_homie_mk_topic_leaf_attribute(_topic_buf, leaf, "$type");
		ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
				leaf->leaf_descriptor->type);
		if (ret != GN_RET_OK)
			return ret;

		//properties

		char *p_msg_buf = calloc(_GN_MQTT_MAX_PAYLOAD_LENGTH, sizeof(char));

		gn_leaf_param_handle_intl_t _param_enum = leaf->params;
		while (_param_enum) {

			strcat(p_msg_buf, _param_enum->name);
			strcat(p_msg_buf, ",");

			if (_param_enum->next) {
				_param_enum = _param_enum->next;
			} else {
				break;
			}

		}

		_gn_homie_mk_topic_leaf_attribute(_topic_buf, leaf, "$properties");
		ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
				p_msg_buf);
		free(p_msg_buf);

		if (ret != GN_RET_OK)
			return ret;

		gn_leaf_param_handle_intl_t _param = leaf->params;
		while (_param) {

			if (_param->access != GN_LEAF_PARAM_ACCESS_NODE_INTERNAL) {

				//name
				_gn_homie_mk_topic_param_attribute(_topic_buf, _param, "$name");
				ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1,
						1, _param->name);
				if (ret != GN_RET_OK)
					return ret;

				//datatype
				_gn_homie_mk_topic_param_attribute(_topic_buf, _param,
						"$datatype");

				switch (_param->param_val->t) {
				case GN_VAL_TYPE_BOOLEAN:
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_DATATYPE_BOOLEAN);
					break;
				case GN_VAL_TYPE_DOUBLE:
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_DATATYPE_FLOAT);
					break;
				case GN_VAL_TYPE_STRING:
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_DATATYPE_STRING);
					break;
				default:
					ret = GN_RET_ERR;
					break;
				}
				if (ret != GN_RET_OK)
					return ret;

				//format
				_gn_homie_mk_topic_param_attribute(_topic_buf, _param,
						"$format");
				ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1,
						1, _param->format);
				if (ret != GN_RET_OK)
					return ret;

				//unit
				_gn_homie_mk_topic_param_attribute(_topic_buf, _param, "$unit");
				ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1,
						1, _param->unit);
				if (ret != GN_RET_OK)
					return ret;

				//settable
				_gn_homie_mk_topic_param_attribute(_topic_buf, _param,
						"$settable");
				if (_param->access == GN_LEAF_PARAM_ACCESS_ALL) {
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_TRUE);
				} else {
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_FALSE);
				}
				if (ret != GN_RET_OK)
					return ret;

				//retained
				_gn_homie_mk_topic_param_attribute(_topic_buf, _param,
						"$retained");
				if (_param->storage == GN_LEAF_PARAM_STORAGE_PERSISTED) {
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_TRUE);
				} else {
					ret = _gn_homie_publish_str(_config->node_handle,
							_topic_buf, 1, 1, _GN_HOMIE_FALSE);
				}
				if (ret != GN_RET_OK)
					return ret;
			}

			if (_param->next) {
				_param = _param->next;
			} else {
				break;
			}
		}

	}

//state ready
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$state");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			(char*) _GN_HOMIE_READY);
	if (ret != GN_RET_OK)
		return ret;

	return GN_RET_OK;

}

/*
 set_state_init()

 set_state_ready()

 set_state_sleep()

 set_state_alert()

 publish_node()

 get_parameter()

 set_parameter()

 receive_broadcast()

 send_broadcast()

 connect()

 disconnect()
 */

gn_err_t gn_mqtt_subscribe_leaf(gn_leaf_handle_t _leaf) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (!_leaf)
		return GN_RET_ERR;

	gn_leaf_handle_intl_t leaf = (gn_leaf_handle_intl_t) _leaf;

//subscribe each parameter
	gn_leaf_param_handle_intl_t _param_enum = leaf->params;
	while (_param_enum) {

		int ret = gn_mqtt_subscribe_leaf_param(_param_enum);

		if (ret != GN_RET_OK) {
			return ret;
			break;
		}

		if (_param_enum->next) {
			_param_enum = _param_enum->next;
		} else {
			break;
		}

	}

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

esp_err_t gn_mqtt_subscribe_leaf_param(gn_leaf_param_handle_t _param) {

	if (!_param)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t param = (gn_leaf_param_handle_intl_t) _param;

	gn_leaf_handle_intl_t leaf = (gn_leaf_handle_intl_t) param->leaf;

	if (!leaf)
		return GN_RET_ERR;

	char _topic_buf[_GN_MQTT_MAX_TOPIC_LENGTH];

	_gn_homie_mk_topic_param_attribute(_topic_buf, _param, "set");

	int msg_id = esp_mqtt_client_subscribe(leaf->node->config->mqtt_client,
			_topic_buf, 0);

	if (esp_log_level_get(TAG) == ESP_LOG_DEBUG) {
		ESP_LOGD(TAG,
				"gn_mqtt_subscribe_leaf_param, topic = %s, msg_id=%d. now waiting %d ms",
				_topic_buf, msg_id, _GN_MQTT_DEBUG_WAIT_MS);
		vTaskDelay(_GN_MQTT_DEBUG_WAIT_MS / portTICK_PERIOD_MS);
	}

	return msg_id == -1 ? GN_RET_ERR : GN_RET_OK;

}

gn_err_t gn_mqtt_start(gn_config_handle_t config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

//TODO check if naming of nodes leaf etc are compliant with homie specs. maybe add warnings in node and leaf creations?

	gn_config_handle_intl_t _config = (gn_config_handle_intl_t) config;

	_gn_event_group_mqtt = xEventGroupCreate();

	xEventGroupClearBits(_gn_event_group_mqtt, _GN_MQTT_DISCONNECT_EVENT_BIT);
	xEventGroupClearBits(_gn_event_group_mqtt, _GN_MQTT_CONNECTED_EVENT_BIT);

	char _topic_buf[_GN_MQTT_MAX_TOPIC_LENGTH] = "";
	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$state");

	esp_mqtt_client_config_t mqtt_cfg = { .uri =
			_config->config_init_params->server_url, .lwt_topic = _topic_buf,
			.lwt_msg = "lost", .lwt_qos = 1, .lwt_retain = 1, .buffer_size =
			CONFIG_GROWNODE_MQTT_BUFFER_SIZE, .user_context = _config };

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

	if (client == NULL) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_init");
		return GN_RET_ERR_INVALID_ARG;
	}

	ESP_ERROR_CHECK(
			esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID, _gn_homie_event_handler, NULL));

	ESP_LOGI(TAG, "Connecting MQTT server at %s", mqtt_cfg.uri);

	esp_err_t esp_ret;
	esp_ret = esp_mqtt_client_start(client);

	if (esp_ret != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_start: %s",
				esp_err_to_name(esp_ret));
		return GN_RET_ERR;
	}

	_config->mqtt_client = client;

	ESP_LOGI(TAG, "gn_mqtt_init waiting to connect");

	EventBits_t uxBits;
	uxBits = xEventGroupWaitBits(_gn_event_group_mqtt,
			_GN_MQTT_CONNECTED_EVENT_BIT | _GN_MQTT_DISCONNECT_EVENT_BIT,
			pdFALSE,
			pdFALSE, portMAX_DELAY);

	if ((uxBits & _GN_MQTT_CONNECTED_EVENT_BIT) != 0) {

		ESP_LOGD(TAG, "gn_mqtt_init connection successful. returning");
		return _gn_homie_on_connected(config, _topic_buf);

	}

	else if ((uxBits & _GN_MQTT_DISCONNECT_EVENT_BIT) != 0) {

		ESP_LOGE(TAG, "gn_mqtt_init connection error. returning");
		return GN_RET_ERR_MQTT_ERROR;

	} else {
		//should never reach here
		ESP_LOGE(TAG, "gn_mqtt_init control flow error. returning");
		return GN_RET_ERR;
	}

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_stop(gn_config_handle_t config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t _config = (gn_config_handle_intl_t) config;

	if (!_config || !_config->mqtt_client)
		return GN_RET_ERR_INVALID_ARG;

	gn_mqtt_disconnect(config);

	esp_err_t esp_ret;
	esp_ret = esp_mqtt_client_stop(_config->mqtt_client);
	if (esp_ret != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_stop: %s",
				esp_err_to_name(esp_ret));
		return GN_RET_ERR;
	}

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_disconnect(gn_config_handle_t config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_config_handle_intl_t _config = (gn_config_handle_intl_t) config;

	char _topic_buf[_GN_MQTT_MAX_TOPIC_LENGTH];
	int ret;

	_gn_homie_mk_topic_node_attribute(_topic_buf, _config->node_handle,
			"$state");
	ret = _gn_homie_publish_str(_config->node_handle, _topic_buf, 1, 1,
			(char*) _GN_HOMIE_DISCONNECTED);
	if (ret != GN_RET_OK)
		return ret;

	esp_err_t esp_ret;
	esp_ret = esp_mqtt_client_disconnect(_config->mqtt_client);
	if (esp_ret != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_disconnect: %s",
				esp_err_to_name(esp_ret));
		return GN_RET_ERR;
	}

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_reconnect(gn_config_handle_t config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (!config)
		return GN_RET_ERR_INVALID_ARG;

	ESP_LOGD(TAG, "gn_mqtt_reconnect");

	gn_config_handle_intl_t _config = (gn_config_handle_intl_t) config;

	if (!_config->mqtt_client)
		return GN_RET_ERR_INVALID_ARG;

	esp_err_t esp_ret;
	esp_ret = esp_mqtt_client_reconnect(_config->mqtt_client);
	if (esp_ret != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_mqtt_client_reconnect: %s",
				esp_err_to_name(esp_ret));
		return GN_RET_ERR;
	}

	ESP_LOGD(TAG, "gn_mqtt_reconnect waiting to connect");

	xEventGroupWaitBits(_gn_event_group_mqtt, _GN_MQTT_CONNECTED_EVENT_BIT,
	pdFALSE,
	pdFALSE, portMAX_DELAY);

	ESP_LOGD(TAG, "gn_mqtt_reconnect returning");

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

gn_err_t gn_mqtt_send_keepalive(gn_node_handle_t _node) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	gn_node_handle_intl_t node = (gn_node_handle_intl_t) _node;

	while (1) {
		_gn_homie_publish_int(node, "$stats/uptime", 0, 0,
				esp_timer_get_time() / 1000000);

		_gn_homie_publish_int(node, "$stats/interval", 0, 0,
				_GN_HOMIE_REFRESH_INTERVAL_MS / 1000);

		int rssi = gn_wifi_get_rssi();
		_gn_homie_publish_int(node, "$stats/rssi", 0, 0, rssi);

		// Translate to "signal" percentage, assuming RSSI range of (-100,-50)
		_gn_homie_publish_int(node, "$stats/signal", 0, 0,
				_clamp((rssi + 100) * 2, 0, 100));

		_gn_homie_publish_int(node, "$stats/freeheap", 0, 0,
				esp_get_free_heap_size());

		vTaskDelay(30000 / portTICK_PERIOD_MS);
	}

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */
}

gn_err_t gn_mqtt_send_leaf_message(gn_leaf_handle_t leaf, const char *msg) {
	return GN_RET_ERR;
}

gn_err_t gn_mqtt_send_leaf_param(gn_leaf_param_handle_t _param) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (!_param) {
		return GN_RET_ERR_INVALID_ARG;
	}

	gn_leaf_param_handle_intl_t param = (gn_leaf_param_handle_intl_t) _param;
	gn_leaf_handle_intl_t leaf = (gn_leaf_handle_intl_t) param->leaf;

	if (leaf->node->config->status != GN_NODE_STATUS_STARTED)
		return GN_RET_OK;

	ESP_LOGD(TAG, "gn_mqtt_send_leaf_param %s", param->name);

	int ret = GN_RET_OK;

	char _topic[_GN_MQTT_MAX_TOPIC_LENGTH];
	char *buf = (char*) calloc(_GN_MQTT_MAX_PAYLOAD_LENGTH, sizeof(char));

	_gn_homie_mk_topic_leaf_attribute(_topic, leaf, param->name);

	switch (param->param_val->t) {
	case GN_VAL_TYPE_BOOLEAN:
		ret = _gn_homie_publish_bool(leaf->node, _topic, 1, 1,
				param->param_val->v.b);
		break;
	case GN_VAL_TYPE_STRING:
		ret = _gn_homie_publish_str(leaf->node, _topic, 1, 1,
				param->param_val->v.s);
		break;
	case GN_VAL_TYPE_DOUBLE:
		ret = _gn_homie_publish_double(leaf->node, _topic, 1, 1,
				param->param_val->v.d);
		break;
	default:
		ESP_LOGE(TAG, "unhandled parameter type");
		goto fail;
		break;
	}

	if (ret != GN_RET_OK)
		goto fail;

	if (esp_log_level_get(TAG) == ESP_LOG_DEBUG) {
		ESP_LOGD(TAG, "sent publish successful, topic=%s. now waiting %d ms",
				_topic, _GN_MQTT_DEBUG_WAIT_MS);
		vTaskDelay(_GN_MQTT_DEBUG_WAIT_MS / portTICK_PERIOD_MS);
	}

	fail: {
		free(buf);
		return ret;
	}

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */
}

gn_err_t gn_mqtt_send_startup_message(gn_config_handle_t _config) {
	return GN_RET_ERR;
}

gn_err_t gn_mqtt_send_reboot_message(gn_config_handle_t _config) {
	return GN_RET_ERR;
}

gn_err_t gn_mqtt_send_reset_message(gn_config_handle_t _config) {
	return GN_RET_ERR;
}

gn_err_t gn_mqtt_send_ota_message(gn_config_handle_t _config) {
	return GN_RET_ERR;
}

gn_err_t gn_mqtt_send_log_message(gn_config_handle_t _config, char *log_tag,
		gn_log_level_t level, char *message) {
	return GN_RET_ERR;
}

#endif //CONFIG_GROWNODE_MQTT_HOMIE_PROTOCOL

#ifdef __cplusplus
}
#endif
