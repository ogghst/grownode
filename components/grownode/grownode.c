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

#include "grownode_intl.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_check.h"

//switch from LOGI to LOGD
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_system.h"
#include "esp_event.h"
//#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#if CONFIG_GROWNODE_WIFI_ENABLED

#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "mqtt_client.h"

#include "lwip/apps/sntp.h"

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

#endif
//#include "esp_smartconfig.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "driver/timer.h"
#include "driver/gpio.h"

#include "gn_event_source.h"
#include "gn_commons.h"
#include "grownode_intl.h"
#include "gn_leaf_context.h"
#include "gn_network.h"
#include "gn_mqtt_protocol.h"
#include "gn_display.h"

#define TAG "grownode"
#define TAG_EVENT "gn_event"
#define TAG_NVS "gn_nvs"

esp_event_loop_handle_t gn_event_loop = NULL;

gn_config_handle_intl_t _gn_default_conf = NULL;

//SemaphoreHandle_t _gn_xEvtSemaphore;

ESP_EVENT_DEFINE_BASE(GN_BASE_EVENT);
ESP_EVENT_DEFINE_BASE(GN_LEAF_EVENT);

/**
 * @brief 	start the leaf by starting a new task and subscribing to network messages
 *
 * @param 	leaf_config	the leaf to start
 *
 * @return 	status of the operation
 */
gn_err_t _gn_leaf_start(gn_leaf_config_handle_intl_t leaf_config) {

	int ret = GN_RET_OK;
	ESP_LOGI(TAG, "_gn_leaf_start %s", leaf_config->name);

	if (xTaskCreate((void*) leaf_config->leaf_descriptor->callback,
			leaf_config->name, leaf_config->task_size, leaf_config, 1, //configMAX_PRIORITIES - 1,
			NULL) != pdPASS) {
		gn_log(TAG, GN_LOG_ERROR, "failed to create lef task for %s",
				leaf_config->name);
		goto fail;
	}

	vTaskDelay(pdMS_TO_TICKS(100));

	//gn_display_leaf_start(leaf_config);

	//notice network of the leaf added
	ret = gn_mqtt_publish_leaf(leaf_config);
	ESP_LOGI(TAG, "_gn_start_leaf %s completed", leaf_config->name);
	return ret;

	fail: return GN_RET_ERR_LEAF_NOT_STARTED;

}

gn_err_t _gn_init_flash(gn_config_handle_t conf) {

	ESP_LOGD(TAG, "_gn_init_flash");

	esp_err_t ret = ESP_OK;
	/* Initialize NVS partition */
	ESP_GOTO_ON_ERROR(nvs_flash_init(), err, TAG, "error init flash: %s",
			esp_err_to_name(ret));

#ifdef CONFIG_GROWNODE_RESET_FLASH
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

		/* NVS partition was truncated
		 * and needs to be erased */
		ESP_GOTO_ON_ERROR(nvs_flash_erase(), err, TAG,
				"error erasing flash: %s", esp_err_to_name(ret));
		/* Retry nvs_flash_init */
		ESP_GOTO_ON_ERROR(nvs_flash_init(), err, TAG, "error init flash: %s",
				esp_err_to_name(ret));

	}
#endif

	return GN_RET_OK;
	err: return GN_RET_ERR;

}

esp_err_t _gn_init_spiffs(gn_config_handle_intl_t conf) {

	ESP_LOGD(TAG, "_gn_init_spiffs");

	esp_vfs_spiffs_conf_t vfs_conf;
	vfs_conf.base_path = "/spiffs";
	vfs_conf.partition_label = NULL;
	vfs_conf.max_files = 6;
	vfs_conf.format_if_mount_failed = true;

	conf->spiffs_conf = vfs_conf;

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret = ESP_OK;

	ret = esp_vfs_spiffs_register(&vfs_conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			gn_log(TAG, GN_LOG_ERROR, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			gn_log(TAG, GN_LOG_ERROR, "Failed to find SPIFFS partition");
		} else {
			gn_log(TAG, GN_LOG_ERROR, "Failed to initialize SPIFFS (%s)",
					esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"Failed to get SPIFFS partition information (%s)",
				esp_err_to_name(ret));
	} else {
		ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	return ret;

}

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

static bool IRAM_ATTR _gn_timer_callback_isr(void *args) {
	BaseType_t high_task_awoken = pdFALSE;
	if (!gn_event_loop)
		return high_task_awoken;
	esp_event_isr_post_to(gn_event_loop, GN_BASE_EVENT,
			GN_SRV_KEEPALIVE_TRIGGERED_EVENT, NULL, 0, &high_task_awoken);
	return high_task_awoken == pdTRUE;
}

void _gn_keepalive_start() {
	ESP_LOGD(TAG, "timer started");
	timer_start(TIMER_GROUP_0, TIMER_0);
}

void _gn_keepalive_stop() {
	ESP_LOGD(TAG, "timer paused");
	timer_pause(TIMER_GROUP_0, TIMER_0);
}

gn_leaf_config_handle_intl_t _gn_leaf_get_by_name(char *leaf_name) {

	gn_leaves_list leaves = _gn_default_conf->node_config->leaves;

	for (int i = 0; i < leaves.last; i++) {
		if (strcmp(leaves.at[i]->name, leaf_name) == 0) {
			return leaves.at[i];
		}
	}
	return NULL;

}

/**
 * @brief	send event to leaf using xQueueSend. the data will be null terminated.
 *
 *	@param	leaf_config the leaf from where the event is sent
 *	@param	evt			the event to send
 *
 * @return 	GN_ERR_EVENT_NOT_SENT if not possible to send event
 */
gn_err_t _gn_send_event_to_leaf(gn_leaf_config_handle_intl_t leaf_config,
		gn_leaf_parameter_event_handle_t evt) {

	ESP_LOGD(TAG_EVENT,
			"_gn_send_event_to_leaf - id: %d, param %s, leaf %s, data %.*s",
			evt->id, evt->param_name, evt->leaf_name, evt->data_size,
			evt->data);

	//make sure data will end with terminating char
	evt->data[evt->data_size] = '\0';

	if (xQueueSend(leaf_config->event_queue, evt, portMAX_DELAY) != pdTRUE) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to send message to leaf %s",
				leaf_config->name);
		return GN_RET_ERR_EVENT_NOT_SENT;
	}
	ESP_LOGD(TAG_EVENT, "_gn_send_event_to_leaf OK");
	return GN_RET_OK;
}

void _gn_evt_handler(void *handler_args, esp_event_base_t base, int32_t id,
		void *event_data) {

	//if (pdTRUE == xSemaphoreTake(_gn_xEvtSemaphore, portMAX_DELAY)) {

	ESP_LOGD(TAG_EVENT, "_gn_evt_handler event: %d", id);

	switch (id) {

	case GN_NET_RBT_START:
		gn_reboot();
		break;

	case GN_NET_OTA_START:
		gn_firmware_update();
		break;

	case GN_NET_RST_START:
		gn_reset();
		break;

	case GN_NET_CONNECTED_EVENT:

		break;
	case GN_NET_DISCONNECTED_EVENT:

		break;
	case GN_SRV_CONNECTED_EVENT:
		//start keepalive service
		if (!_gn_default_conf)
			break;

		//from unknown status: reboot - to keep consistency
		if (_gn_default_conf->status != GN_NODE_STATUS_READY_TO_START
				&& _gn_default_conf->status != GN_NODE_STATUS_STARTED) {
			gn_reboot();
			break;
		}

		_gn_keepalive_start();

		break;
	case GN_SRV_DISCONNECTED_EVENT:
		//stop keepalive service
		_gn_keepalive_stop();
		break;

		/*
		 case GN_NODE_STARTED_EVENT:

		 //start keepalive service
		 if (!_gn_default_conf)
		 break;

		 //from unknown status: reboot - to keep consistency
		 if (_gn_default_conf != NULL
		 && _gn_default_conf->status != GN_CONFIG_STATUS_STARTED) {
		 gn_reboot();
		 }

		 if (_gn_default_conf != NULL
		 && _gn_default_conf->status == GN_CONFIG_STATUS_STARTED) {
		 _gn_keepalive_start();
		 }
		 break;
		 */

	case GN_SRV_KEEPALIVE_TRIGGERED_EVENT:
		//publish node
		if (gn_mqtt_send_node_config(_gn_default_conf->node_config)
				!= GN_RET_OK) {
			ESP_LOGE(TAG, "Error in sending node config message");
		}
		break;

	case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT: {

		gn_leaf_parameter_event_handle_t evt =
				(gn_leaf_parameter_event_handle_t) event_data;

		gn_leaf_config_handle_intl_t leaf_config = _gn_leaf_get_by_name(
				evt->leaf_name);

		if (leaf_config != NULL) {

			//send message to the interested leaf
			_gn_send_event_to_leaf(leaf_config, evt);

		}
	}

		break;

	default:
		break;
	}

	//xSemaphoreGive(_gn_xEvtSemaphore);
	//}
}

esp_err_t _gn_evt_handlers_register(gn_config_handle_intl_t conf) {

	ESP_LOGD(TAG, "_gn_evt_handlers_register");

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, _gn_evt_handler, conf, NULL));

	return ESP_OK;

}

esp_err_t _gn_init_keepalive_timer(gn_config_handle_intl_t conf) {

	ESP_LOGD(TAG, "_gn_init_keepalive_timer");

	timer_config_t config = { .divider = TIMER_DIVIDER, .counter_dir =
			TIMER_COUNT_UP, .counter_en = TIMER_PAUSE, .alarm_en =
			TIMER_ALARM_EN, .auto_reload = 1, }; // default clock source is APB
	timer_init(TIMER_GROUP_0, TIMER_0, &config);
	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_0,
			conf->config_init_params->server_keepalive_timer_sec * TIMER_SCALE);
	timer_enable_intr(TIMER_GROUP_0, TIMER_0);
	return timer_isr_callback_add(TIMER_GROUP_0, TIMER_0,
			_gn_timer_callback_isr,
			NULL, 0);

}

/**
 * @brief	initialize config
 *
 * @return 	the configuration with its state (GN_CONFIG_STATUS_NOT_INITIALIZED as default)
 */
gn_config_handle_intl_t _gn_config_create(gn_config_init_param_t *config_init) {

	ESP_LOGD(TAG, "_gn_config_create");

	gn_config_handle_intl_t _conf = (gn_config_handle_intl_t) malloc(
			sizeof(struct gn_config_t));
	_conf->status = GN_NODE_STATUS_INITIALIZING;

	_conf->config_init_params = config_init;

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (config_init->provisioning_security
			&& !config_init->provisioning_password) {
		_conf->status = GN_NDOE_STATUS_ERROR_MISSING_PROVISIONING_PASSWORD;
		return _conf;
	}

	if (!config_init->server_base_topic) {
		_conf->status = GN_NODE_STATUS_ERROR_MISSING_SERVER_BASE_TOPIC;
		return _conf;
	}

	if (!config_init->server_url) {
		_conf->status = GN_NODE_STATUS_ERROR_MISSING_SERVER_URL;
		return _conf;
	}

	if (config_init->server_keepalive_timer_sec <= 0
			|| config_init->server_keepalive_timer_sec
					> GN_CONFIG_MAX_SERVER_KEEPALIVE_SEC) {
		_conf->status = GN_NODE_STATUS_ERROR_BAD_SERVER_KEEPALIVE_SEC;
		return _conf;
	}

	if (config_init->server_discovery
			&& !config_init->server_discovery_prefix) {
		_conf->status = GN_NODE_STATUS_ERROR_MISSING_SERVER_DISCOVERY_PREFIX;
		return _conf;
	}

	if (!config_init->firmware_url) {
		_conf->status = GN_NODE_STATUS_ERROR_MISSING_FIRMWARE_URL;
		return _conf;
	}

	if (!config_init->sntp_url) {
		_conf->status = GN_NODE_STATUS_ERROR_MISSING_SNTP_URL;
		return _conf;
	}

#endif

	/*
	 #ifdef CONFIG_GROWNODE_WIFI_ENABLED

	 _conf->mqtt_base_topic = (char*) calloc(
	 sizeof(CONFIG_GROWNODE_MQTT_BASE_TOPIC) + 1, sizeof(char));
	 strncpy(_conf->mqtt_base_topic, CONFIG_GROWNODE_MQTT_BASE_TOPIC,
	 sizeof(CONFIG_GROWNODE_MQTT_BASE_TOPIC) + 1);

	 _conf->mqtt_url = (char*) calloc(sizeof(CONFIG_GROWNODE_MQTT_URL) + 1,
	 sizeof(char));
	 strncpy(_conf->mqtt_url, CONFIG_GROWNODE_MQTT_URL,
	 sizeof(CONFIG_GROWNODE_MQTT_URL) + 1);

	 _conf->mqtt_keepalive_timer_sec = CONFIG_GROWNODE_KEEPALIVE_TIMER_SEC;

	 _conf->ota_url = (char*) calloc(sizeof(CONFIG_GROWNODE_FIRMWARE_URL) + 1,
	 sizeof(char));
	 strncpy(_conf->ota_url, CONFIG_GROWNODE_FIRMWARE_URL,
	 sizeof(CONFIG_GROWNODE_FIRMWARE_URL) + 1);

	 _conf->sntp_server_name = (char*) calloc(
	 sizeof(CONFIG_GROWNODE_SNTP_SERVER_NAME) + 1, sizeof(char));
	 strncpy(_conf->sntp_server_name, CONFIG_GROWNODE_SNTP_SERVER_NAME,
	 sizeof(CONFIG_GROWNODE_SNTP_SERVER_NAME) + 1);

	 #else
	 */

	/*
	 _conf->mqtt_base_topic = (char*) calloc(
	 strlen(config_init.server_base_topic) + 1, sizeof(char));
	 strncpy(_conf->mqtt_base_topic, config_init.server_base_topic,
	 strlen(config_init.server_base_topic));

	 _conf->mqtt_url = (char*) calloc(strlen(config_init.server_url) + 1,
	 sizeof(char));
	 strncpy(_conf->mqtt_url, config_init.server_url,
	 strlen(config_init.server_url));

	 _conf->mqtt_keepalive_timer_sec = config_init.server_keepalive_timer_sec;

	 _conf->ota_url = (char*) calloc(strlen(config_init.firmware_url) + 1,
	 sizeof(char));
	 strncpy(_conf->ota_url, config_init.firmware_url,
	 strlen(config_init.firmware_url));

	 _conf->sntp_server_name = (char*) calloc(strlen(config_init.sntp_url) + 1,
	 sizeof(char));
	 strncpy(_conf->sntp_server_name, config_init.sntp_url,
	 strlen(config_init.sntp_url));
	 */
//#endif
	return _conf;

}

esp_err_t _gn_init_event_loop(gn_config_handle_intl_t config) {

	ESP_LOGD(TAG, "_gn_init_event_loop");

	esp_err_t ret = ESP_OK;

	//default event loop
	ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), fail, TAG,
			"error creating system event loop: %s", esp_err_to_name(ret));

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 25, .task_name =
			"gn_evt_loop", // task will be created
			.task_priority = 0, .task_stack_size = 4096, .task_core_id = 1 };
	ESP_GOTO_ON_ERROR(esp_event_loop_create(&event_loop_args, &gn_event_loop),
			fail, TAG, "error creating grownode event loop: %s",
			esp_err_to_name(ret));

	config->event_loop = gn_event_loop;

	fail: return ret;

}

gn_node_config_handle_intl_t _gn_node_config_create() {

	gn_node_config_handle_intl_t _conf = (gn_node_config_handle_intl_t) malloc(
			sizeof(struct gn_node_config_t));
	_conf->config = NULL;
	//_conf->event_loop = NULL;
	strcpy(_conf->name, "");
	return _conf;

}

/**
 * 	@brief retrieves the configuration status
 *
 * 	@param config	the configuration handle to check
 *
 * 	@return GN_CONFIG_STATUS_ERROR if config is NULL
 * 	@return the configuration status
 */
inline gn_node_status_t gn_get_status(gn_config_handle_t config) {

	if (!config)
		return GN_NODE_STATUS_ERROR;
	return ((gn_config_handle_intl_t) config)->status;

}

/**
 * 	@brief retrieves the configuration status description
 *
 * 	@param config	the configuration handle to check
 *
 * 	@return GN_CONFIG_STATUS_ERROR if config is NULL
 * 	@return the configuration status
 */
inline const char* gn_get_status_description(gn_config_handle_t config) {

	if (!config)
		return gn_config_status_descriptions[GN_NODE_STATUS_ERROR];
	return gn_config_status_descriptions[((gn_config_handle_intl_t) config)->status];

}

/**
 * 	@brief retrieves the event loop starting from the config handle
 *
 * 	@param config	the config handle
 *
 * 	@return the event loop
 * 	@return NULL if config not valid
 */
esp_event_loop_handle_t gn_get_event_loop(gn_config_handle_t config) {

	if (!config)
		return NULL;
	return ((gn_config_handle_intl_t) config)->event_loop;

}

/**
 * 	@brief retrieves the event loop starting from the leaf config handle
 *
 * 	@param leaf_config	the leaf config handle
 *
 * 	@return the event loop
 * 	@return NULL if leaf config not valid
 */
esp_event_loop_handle_t gn_leaf_get_event_loop(
		gn_leaf_handle_t leaf_config) {

	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->node_config->config->event_loop;

}

/**
 * 	@brief		create a new node with specified configuration and name
 *
 * 	@param		config	the config handle to use
 * 	@param		name	name of the node. MUST BE null terminated
 *
 * 	@return		the node handle created.
 */
gn_node_handle_t gn_node_create(gn_config_handle_t config,
		const char *name) {

	if (config == NULL
			|| ((gn_config_handle_intl_t) config)->mqtt_client == NULL
			|| name == NULL) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_create_node failed. parameters not correct");
		return NULL;
	}

	gn_node_config_handle_intl_t n_c = _gn_node_config_create();

	strncpy(n_c->name, name, GN_NODE_NAME_SIZE);
	//n_c->event_loop = config->event_loop;
	n_c->config = config;

	//create leaves
	gn_leaves_list leaves = { .size = GN_NODE_LEAVES_MAX_SIZE, .last = 0 };

	n_c->leaves = leaves;
	((gn_config_handle_intl_t) config)->node_config = n_c;

	return n_c;
}

/**
 * @brief		number of leaves into the node
 *
 * @param		node_config 	the node to be inspected
 *
 * @return		number of leaves into the node, -1 in case node_config is NULL
 */
size_t gn_node_get_size(gn_node_handle_t node_config) {

	if (node_config == NULL)
		return -1;

	return ((gn_node_config_handle_intl_t) node_config)->leaves.last;
}

/**
 * @brief		removes the node from the config
 *
 * @param		node 	the node to be removed
 *
 * @return		GN_RET_OK if operation had succeded
 */
gn_err_t gn_node_destroy(gn_node_handle_t node) {

	//free(((gn_node_config_handle_intl_t) node)->leaves->at); //TODO implement free of leaves
	free((gn_node_config_handle_intl_t) node);

	return GN_RET_OK;
}

/**
 * @brief		starts the node by starting the leaves tasks
 *
 * At the end of the process, it sets the node status to GN_CONFIG_STATUS_STARTED and sends a GN_NODE_STARTED_EVENT event
 *
 * @param		node 	the node to be started
 *
 * @return		GN_RET_OK if operation had succeded, GN_RET_ERR_NODE_NOT_STARTED in case of issues
 */
gn_err_t gn_node_start(gn_node_handle_t node) {

	gn_node_config_handle_intl_t _node = (gn_node_config_handle_intl_t) node;

	gn_err_t ret = GN_RET_OK;

	ESP_LOGD(TAG, "gn_start_node: %s, leaves: %d", _node->name,
			_node->leaves.last);

	//publish node
	//if (gn_mqtt_send_node_config(node) != ESP_OK)
	//return ESP_FAIL;

	//run leaves
	for (int i = 0; i < _node->leaves.last; i++) {
		//ESP_LOGD(TAG, "starting leaf: %d", i);
		if (_gn_leaf_start(_node->leaves.at[i]) != GN_RET_OK)
			return GN_RET_ERR_NODE_NOT_STARTED;
	}

	_node->config->status = GN_NODE_STATUS_STARTED;

	if (ESP_OK
			!= esp_event_post_to(_node->config->event_loop, GN_BASE_EVENT,
					GN_NODE_STARTED_EVENT,
					NULL, 0, portMAX_DELAY)) {
		gn_log(TAG, GN_LOG_ERROR,
				"failed to send GN_SERVER_CONNECTED_EVENT event");
		return GN_RET_ERR_EVENT_LOOP_ERROR;
	}

	ret = gn_send_node_leaf_param_status(node);

	return ret;

}

gn_leaf_config_handle_intl_t _gn_leaf_config_create() {

	gn_leaf_config_handle_intl_t _conf = (gn_leaf_config_handle_intl_t) malloc(
			sizeof(struct gn_leaf_config_t));
	//_conf->callback = NULL;
	strcpy(_conf->name, "");
	_conf->node_config = NULL;
	_conf->leaf_descriptor = NULL;
	_conf->params = NULL;
	return _conf;

}

/**
 *	@brief		gets the name of the node referenced by the handle
 *
 *	@param		node_config		the handle to be queried
 *	@param		name			the pointer where the name will be set. set lenght to GN_LEAF_NAME_SIZE
 *
 *	@return		GN_RET_ERR_INVALID_ARG if the handle is not valid
 *	@return 	GN_RET_OK if everything OK
 *
 */
gn_err_t gn_node_get_name(gn_node_handle_t node_config, char *name) {

	if (!node_config)
		return GN_RET_ERR_INVALID_ARG;
	strncpy(name, ((gn_node_config_handle_intl_t) node_config)->name,
			strlen(((gn_node_config_handle_intl_t) node_config)->name) + 1);

	return GN_RET_OK;

}

/**
 *	@brief		creates the leaf
 *
 *	initializes the leaf structure. the returned handle is not active and need to be started by the gn_node_start() function
 *  @see gn_node_start()
 *	@param		node_config	the configuration handle to create the leaf to
 *	@param		name		the name of the leaf to be created
 *	@param		task		callback function of the leaf task
 *	@param		task_size	the size of the task to be memory allocated
 *
 *	@return		an handle to the leaf config
 *	@return		NULL if the handle cannot be created
 *
 */
gn_leaf_handle_t gn_leaf_create(gn_node_handle_t node_config,
		const char *name, gn_leaf_config_callback leaf_config, size_t task_size) { //, gn_leaf_display_task_t display_task) {

	gn_node_config_handle_intl_t node_cfg =
			(gn_node_config_handle_intl_t) node_config;

	if (node_cfg == NULL || node_cfg->config == NULL || name == NULL
			|| leaf_config == NULL || node_cfg->config->mqtt_client == NULL) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_create failed. parameters not correct");
		return NULL;
	}

	gn_leaf_config_handle_intl_t l_c = _gn_leaf_config_create();
	gn_node_config_handle_intl_t n_c = node_cfg;

	strncpy(l_c->name, name, GN_LEAF_NAME_SIZE);
	l_c->node_config = node_cfg;
	//l_c->task_cb = task;
	l_c->task_size = task_size;
	l_c->leaf_context = gn_leaf_context_create();
	l_c->display_container = NULL;
	//l_c->display_task = display_task;
	l_c->event_queue = xQueueCreate(1, sizeof(gn_leaf_parameter_event_t));
	if (l_c->event_queue == NULL) {
		return NULL;
	}
	//l_c->event_loop = gn_event_loop;

	//configures leaf and get descriptor
	l_c->leaf_descriptor = leaf_config(l_c);

	//TODO add leaf to node. implement dynamic array
	if (n_c->leaves.last >= n_c->leaves.size - 1) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_create failed. not possible to add more than %d leaves to a node",
				n_c->leaves.size);
		return NULL;
	}

	n_c->leaves.at[n_c->leaves.last] = l_c;
	n_c->leaves.last++;

	ESP_LOGD(TAG, "gn_create_leaf success");
	return l_c;

}

/**
 * returns the descriptor handle for the corresponding leaf
 */
gn_leaf_descriptor_handle_t gn_leaf_get_descriptor(
		gn_leaf_handle_t leaf_config) {
	return ((gn_leaf_config_handle_intl_t) leaf_config)->leaf_descriptor;
}

gn_err_t _gn_leaf_destroy(gn_leaf_handle_t leaf_config) {

	gn_leaf_config_handle_intl_t _leaf_config =
			((gn_leaf_config_handle_intl_t) leaf_config);
	gn_leaf_context_destroy(_leaf_config->leaf_context);
	vQueueDelete(_leaf_config->event_queue);
	;
	free(leaf_config);
	return GN_RET_OK;

}

/**
 *	@brief		gets the name of the leaf referenced by the handle
 *
 *	@param		leaf_config		the handle to be queried
 *	@param		name			the pointer where the name will be set. set lenght to GN_LEAF_NAME_SIZE
 *
 *	@return		GN_RET_ERR_INVALID_ARG if the handle is not valid
 *	@return 	GN_RET_OK if everything OK
 *
 */
gn_err_t gn_leaf_get_name(gn_leaf_handle_t leaf_config, char *name) {

	if (!leaf_config)
		return GN_RET_ERR_INVALID_ARG;
	strncpy(name, ((gn_leaf_config_handle_intl_t) leaf_config)->name,
			strlen(((gn_leaf_config_handle_intl_t) leaf_config)->name) + 1);

	return GN_RET_OK;

}

/*
 * @brief gets the handle corresponding to the name
 *
 * @param		node	the node to search within
 * @param 		name 	name of the leaf, null terminated
 *
 * @return 		the leaf handle
 * @return		NULL if not found
 */
gn_leaf_handle_t gn_leaf_get_config_handle(gn_node_handle_t node,
		const char *name) {

	if (!name || !node)
		return NULL;

	gn_node_config_handle_intl_t _node = (gn_node_config_handle_intl_t) node;

	for (size_t i = 0; i < _node->leaves.last; i++) {

		gn_leaf_config_handle_intl_t _lc =
				(gn_leaf_config_handle_intl_t) _node->leaves.at[i];

		if (strncmp(name, _lc->name, sizeof(_lc->name)) == 0) {
			return _lc;
		}

	}

	return NULL;

}

/*
 * @brief get the node of the specified leaf
 *
 * @param	leaf_config		the leaf to search within
 * @return					the node
 * @return					NULL if not present or leaf config is NULL
 */
gn_node_handle_t gn_leaf_get_node(gn_leaf_handle_t leaf_config) {

	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->node_config;

}

/**
 * 	@brief gets the leaf queue handle
 *
 * 	@param leaf_config the leaf to be queried
 *
 * 	@return	the queue handle
 * 	@return NULL if leaf not found
 */
QueueHandle_t gn_leaf_get_event_queue(gn_leaf_handle_t leaf_config) {
//TODO find a way to hide the freertos structure
	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->event_queue;

}

/**
 * send event to leaf, by converting the event to gn_leaf_parameter_event_handle_t struct and pass in leaf event queue.
 * if the event is a leaf parameter event, event_data will be passed in the queue.
 * if the event is different, event_data will be copied and null terminated in the character array data.
 */
void _gn_leaf_evt_handler(void *handler_args, esp_event_base_t base, int32_t id,
		void *event_data) {

	//if (pdTRUE == xSemaphoreTake(_gn_xEvtSemaphore, portMAX_DELAY)) {

	ESP_LOGD(TAG_EVENT, "_gn_leaf_evt_handler event: %d", id);

	//gets leaf
	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) handler_args;

	if (id == GN_LEAF_PARAM_CHANGED_EVENT
			|| id == GN_LEAF_PARAM_INITIALIZED_EVENT
			|| id == GN_LEAF_PARAM_CHANGE_REQUEST_EVENT) {

		gn_leaf_parameter_event_handle_t evt =
				(gn_leaf_parameter_event_handle_t) event_data;

		if (_gn_send_event_to_leaf(leaf_config, evt) == GN_RET_OK) {
			//ESP_LOGD(TAG, "_gn_leaf_evt_handler OK");
		} else {
			gn_log(TAG, GN_LOG_ERROR, "_gn_leaf_evt_handler ERROR");
		}

	} else {

		gn_leaf_parameter_event_t evt;

		evt.id = id;
		strncpy(evt.leaf_name, leaf_config->name,
		GN_LEAF_NAME_SIZE);
		strncpy(evt.param_name, "", sizeof(char));

		if (event_data) {
			//memcpy(&evt.data[0], event_data, GN_LEAF_DATA_SIZE);
			evt.data_size = GN_LEAF_DATA_SIZE;
			strncpy(evt.data, event_data, GN_LEAF_NAME_SIZE);
		}

		if (_gn_send_event_to_leaf(leaf_config, &evt) == GN_RET_OK) {
			//ESP_LOGD(TAG, "_gn_leaf_evt_handler OK");
		} else {
			gn_log(TAG, GN_LOG_ERROR, "_gn_leaf_evt_handler ERROR");
		}

	}

}

/**
 * @brief subscribe the leaf to the event id.
 *
 * @return GN_RET_OK if successful
 */
gn_err_t gn_leaf_event_subscribe(gn_leaf_handle_t leaf_config,
		gn_event_id_t event_id) {

	if (!leaf_config)
		return GN_RET_ERR;
	esp_event_loop_handle_t event_loop = gn_leaf_get_event_loop(leaf_config);

	if (!event_loop)
		return GN_RET_ERR;

	ESP_LOGD(TAG_EVENT, "gn_leaf_event_subscribe event: %d, leaf %s", event_id,
			((gn_leaf_config_handle_intl_t ) leaf_config)->name);

	//register for events
	esp_err_t ret = esp_event_handler_instance_register_with(event_loop,
			GN_BASE_EVENT, event_id, _gn_leaf_evt_handler, leaf_config, NULL);

	return ret == ESP_OK ? GN_RET_OK : GN_RET_ERR;

}

/**
 * @brief unsubscribe the leaf to the event id.
 *
 * @return GN_RET_OK if successful
 */
gn_err_t gn_leaf_event_unsubscribe(gn_leaf_handle_t leaf_config,
		gn_event_id_t event_id) {

	if (!leaf_config)
		return GN_RET_ERR;
	esp_event_loop_handle_t event_loop = gn_leaf_get_event_loop(leaf_config);

	if (!event_loop)
		return GN_RET_ERR;

	ESP_LOGD(TAG_EVENT, "gn_leaf_event_unsubscribe event: %d, leaf %s",
			event_id, ((gn_leaf_config_handle_intl_t ) leaf_config)->name);

	//unregister for events
	esp_err_t ret = esp_event_handler_instance_unregister_with(event_loop,
			GN_BASE_EVENT, event_id, NULL);

	return ret == ESP_OK ? GN_RET_OK : GN_RET_ERR;

}

/**
 * 	@brief	creates a parameter on the leaf
 *
 * 	NOTE: if parameter is stored, the value is overridden
 *
 *	@param	leaf_config		the leaf to be queried
 *	@param	name			the name of the parameter (null terminated char array)
 *	@param	type			the type of parameter
 *	@param	val				the value of parameter
 *	@param	access			access type of parameter
 *	@param	storage			storage type of parameter

 * @return 	the new parameter handle
 * @return	NULL in case of errors
 */
gn_leaf_param_handle_t gn_leaf_param_create(gn_leaf_handle_t leaf_config,
		const char *name, const gn_val_type_t type, gn_val_t val,
		gn_leaf_param_visibility_t access, gn_leaf_param_storage_t storage,
		gn_validator_t validator) {

	if (!name) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_param_create incorrect parameters");
		return NULL;
	}

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	ESP_LOGD(TAG, "gn_leaf_param_create %s ", name);
	//ESP_LOGD(TAG, "building storage tag..");

	if (storage == GN_LEAF_PARAM_STORAGE_PERSISTED) {
		//check parameter stored
		int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
		//ESP_LOGD(TAG, "..len: %i", _len);
		char *_buf = (char*) calloc(_len, sizeof(char));

		memcpy(_buf, _leaf_config->name,
				strlen(_leaf_config->name) * sizeof(char));
		memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
				1 * sizeof(char));
		memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
				strlen(name) * sizeof(char));

		_buf[_len - 1] = '\0';

		//ESP_LOGD(TAG, ".. storage tag: %s", _buf);

		char *value = 0;
		//check if existing
		ESP_LOGD(TAG, "check stored value for key %s", _buf);

		if (gn_storage_get(_buf, (void**) &value) == ESP_OK) {
			ESP_LOGD(TAG, "found stored value for key %s", _buf);

			switch (type) {
			case GN_VAL_TYPE_STRING:
				free(val.s);
				val.s = strdup(value);
				ESP_LOGD(TAG, ".. value: %s", val.s);
				free(value);
				break;
			case GN_VAL_TYPE_BOOLEAN:
				val.b = *((bool*) value);
				ESP_LOGD(TAG, ".. value: %d", val.b);
				free(value);
				break;
			case GN_VAL_TYPE_DOUBLE:
				val.d = *((double*) value);
				ESP_LOGD(TAG, ".. value: %f", val.d);
				free(value);
				break;
			default:
				gn_log(TAG, GN_LOG_ERROR, "param type not handled");
				free(value);
				free(_buf);
				return NULL;
				break;
			}

		}

		free(_buf);
	}

	gn_leaf_param_handle_intl_t _ret = (gn_leaf_param_handle_intl_t) malloc(
			sizeof(gn_leaf_param_t));
	_ret->next = NULL;

	char *_name = strdup(name);
	//(char*) calloc(sizeof(char)*strlen(name));
	//strncpy(_name, name, strlen(name));
	_ret->name = _name;

	gn_param_val_handle_t _param_val = (gn_param_val_handle_t) malloc(
			sizeof(gn_param_val_t));
	gn_val_t _val;

	switch (type) {
	case GN_VAL_TYPE_STRING:
		if (!val.s) {
			gn_log(TAG, GN_LOG_ERROR,
					"gn_leaf_param_create incorrect string parameter");
			return NULL;
		}
		_val.s = strdup(val.s);
		break;
	case GN_VAL_TYPE_BOOLEAN:
		_val.b = val.b;
		break;
	case GN_VAL_TYPE_DOUBLE:
		_val.d = val.d;
		break;
	default:
		gn_log(TAG, GN_LOG_ERROR, "param type not handled");
		return NULL;
		break;
	}

	memcpy(&_param_val->t, &type, sizeof(type));
	memcpy(&_param_val->v, &_val, sizeof(_val));

	_ret->param_val = _param_val;
	_ret->access = access;
	_ret->storage = storage;
	_ret->validator = validator;

	return _ret;

}

/**
 * 	@brief	init the parameter with new value and stores in NVS flash, overwriting previous values
 *
 *	the leaf must be not initialized to have an effect.
 * 	the parameter value will be copied to the corresponding handle.

 * 	@param leaf_config	the leaf handle to be queried
 * 	@param name			the name of the parameter (null terminated)
 * 	@param val			the value to set
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 */
gn_err_t gn_leaf_param_init_string(const gn_leaf_handle_t leaf_config,
		const char *name, const char *val) {

	if (!leaf_config || !name || !val)
		return ESP_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "gn_leaf_param_init_string - param:%s value:%s", name, val);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
	char *_buf = (char*) calloc(_len, sizeof(char));

	memcpy(_buf, _leaf_config->name, strlen(_leaf_config->name) * sizeof(char));
	memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
			1 * sizeof(char));
	memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
			strlen(name) * sizeof(char));

	_buf[_len - 1] = '\0';

	//if already set keep old value
	if (gn_storage_get(_buf, (void**) &val) == ESP_OK) {
		ESP_LOGD(TAG, ".. value already found: (%s) - skipping", val);
		return GN_RET_OK;
	}

	gn_param_val_handle_int_t _val =
			(gn_param_val_handle_int_t) _param->param_val;

	if (_param->validator) {
		void **validate = (void**) &val;
		gn_leaf_param_validator_result_t ret = _param->validator(_param,
				validate);
		if (ret != GN_LEAF_PARAM_VALIDATOR_ERROR
				&& ret != GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED)
			strncpy(_val->v.s, *validate, strlen(*validate));
		ESP_LOGD(TAG, "processing validator - result: %d", (int ) ret);
	} else {
		strncpy(_val->v.s, val, strlen(val));
	}

	//store the parameter
	if (gn_storage_set(_buf, (void**) &val, strlen(val)) != ESP_OK) {
		ESP_LOGW(TAG,
				"not possible to store leaf parameter value - key %s value %s",
				_buf, val);
		free(_buf);
		return GN_RET_ERR;
	}

	//notify event loop
	gn_leaf_parameter_event_t evt;
	strcpy(evt.leaf_name, _leaf_config->name);
	strcpy(evt.param_name, _param->name);
	evt.id = GN_LEAF_PARAM_INITIALIZED_EVENT;
	//evt.data = calloc((strlen(_param->param_val->v.s) + 1) * sizeof(char));
	strncpy(evt.data, _param->param_val->v.s, GN_LEAF_DATA_SIZE);

	esp_err_t ret = esp_event_post_to(
			_leaf_config->node_config->config->event_loop, GN_BASE_EVENT,
			evt.id, &evt, sizeof(evt), portMAX_DELAY);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_init_string - not possible to send param message to event loop - id:%d, size:%d - result: %d",
				evt.id, sizeof(evt), (int) ret);
		return GN_RET_ERR;
	}

	free(_buf);

	return GN_RET_OK;

}

/**
 * 	@brief	updates the parameter with new value
 *
 * 	the parameter value will be copied to the corresponding handle.
 * 	after the change the parameter change will be propagated to the event system through a GN_LEAF_PARAM_CHANGED_EVENT and to the server.
 *
 * 	@param leaf_config	the leaf handle to be queried
 * 	@param name			the name of the parameter (null terminated)
 * 	@param val			the value to set (null terminated)
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 * 	@return GN_RET_ERR in case of messaging error
 */
gn_err_t gn_leaf_param_set_string(const gn_leaf_handle_t leaf_config,
		const char *name, char *val) {

	if (!leaf_config || !name || !val)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param)
		return GN_RET_ERR_INVALID_ARG;

	//ESP_LOGD(TAG, "gn_leaf_param_set_string - param:%s value:%s", name, val);
	//ESP_LOGD(TAG, "	old value %s", _param->param_val->v.s);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	if (_param->storage == GN_LEAF_PARAM_STORAGE_PERSISTED) {

		int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
		char *_buf = (char*) calloc(_len, sizeof(char));

		memcpy(_buf, _leaf_config->name,
				strlen(_leaf_config->name) * sizeof(char));
		memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
				1 * sizeof(char));
		memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
				strlen(name) * sizeof(char));

		_buf[_len] = '\0';

		if (gn_storage_set(_buf, (void**) &val, strlen(val)) != ESP_OK) {
			ESP_LOGW(TAG,
					"not possible to store leaf parameter value - key %s value %s",
					_buf, val);
			free(_buf);
			return GN_RET_ERR;
		}

		free(_buf);

	}

	if (_param->validator) {
		char **validate = &val;
		gn_leaf_param_validator_result_t ret = _param->validator(_param,
				(void**) validate);
		if (ret != GN_LEAF_PARAM_VALIDATOR_ERROR
				&& ret != GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED) {
			_param->param_val->v.s = (char*) realloc(_param->param_val->v.s,
					sizeof(char) * (strlen(*validate) + 1));
			memset(_param->param_val->v.s, 0,
					sizeof(char) * (strlen(*validate) + 1));
			strncpy(_param->param_val->v.s, *validate, strlen(*validate));
		}
		//ESP_LOGD(TAG, "processing validator - result: %d", (int )ret);
	} else {
		_param->param_val->v.s = (char*) realloc(_param->param_val->v.s,
				sizeof(char) * (strlen(val) + 1));
		memset(_param->param_val->v.s, 0, sizeof(char) * (strlen(val) + 1));
		strncpy(_param->param_val->v.s, val, strlen(val));
	}

	//ESP_LOGD(TAG, "gn_leaf_param_set - result %s", _param->param_val->v.s);

	//notify event loop
	gn_leaf_parameter_event_t evt;
	strcpy(evt.leaf_name, _leaf_config->name);
	strcpy(evt.param_name, _param->name);
	evt.id = GN_LEAF_PARAM_CHANGED_EVENT;
	//evt.data = calloc((strlen(_param->param_val->v.s) + 1) * sizeof(char));
	strncpy(evt.data, _param->param_val->v.s, GN_LEAF_DATA_SIZE);

	esp_err_t ret = esp_event_post_to(
			_leaf_config->node_config->config->event_loop, GN_BASE_EVENT,
			evt.id, &evt, sizeof(evt), portMAX_DELAY);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_set_string - not possible to send param message to event loop - id:%d, size:%d - result: %d",
				evt.id, sizeof(evt), (int) ret);
		return GN_RET_ERR;
	}

	return gn_mqtt_send_leaf_param(_param);

}

/**
 * @brief gets the parameter value
 *
 * @param 	leaf_config	the leaf to get the parameter from
 * @param 	name		the name of the parameter, null terminated
 * @param	val			pointer where the parameter is put
 * @param	max_lenght	the maximum lenght of the parameter value to be copied
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG if the parameter is not found
 */
gn_err_t gn_leaf_param_get_string(const gn_leaf_handle_t leaf_config,
		const char *name, char *val, size_t max_lenght) {

	if (!leaf_config || !name)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		return GN_RET_ERR_INVALID_ARG;
	}

	gn_param_val_handle_int_t _val =
			(gn_param_val_handle_int_t) _param->param_val;
	if (!_val) {
		return GN_RET_ERR_INVALID_ARG;
	}

	strncpy(val, _val->v.s, max_lenght);

	return GN_RET_OK;

}

/**
 * 	@brief	init the parameter with new value and stores in NVS flash, overwriting previous values
 *
 *	the leaf must be not initialized to have an effect.
 * 	the parameter value will be copied to the corresponding handle.

 * 	@param leaf_config	the leaf handle to be queried
 * 	@param name			the name of the parameter (null terminated)
 * 	@param val			the value to set
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 */
gn_err_t gn_leaf_param_init_bool(const gn_leaf_handle_t leaf_config,
		const char *name, bool val) {

	if (!leaf_config || !name)
		return ESP_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "gn_leaf_param_init_bool %s %d", name, val);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
	char *_buf = (char*) calloc(_len, sizeof(char));

	memcpy(_buf, _leaf_config->name, strlen(_leaf_config->name) * sizeof(char));
	memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
			1 * sizeof(char));
	memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
			strlen(name) * sizeof(char));

	_buf[_len - 1] = '\0';

	//if already set keep old value
	if (gn_storage_get(_buf, (void**) &val) == ESP_OK) {
		ESP_LOGD(TAG, ".. value already found: (%d) - skipping", val);
		return GN_RET_OK;
	}

	if (_param->validator) {
		bool *p = &val;
		bool **validate = &p;
		gn_leaf_param_validator_result_t ret = _param->validator(_param,
				(void**) validate);
		if (ret != GN_LEAF_PARAM_VALIDATOR_ERROR
				&& ret != GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED)
			_param->param_val->v.b = **validate;
		ESP_LOGD(TAG, "processing validator - result: %d", (int ) ret);
	} else {
		_param->param_val->v.b = val;
	}

	//store the parameter
	if (gn_storage_set(_buf, (void**) &val, sizeof(bool)) != ESP_OK) {
		ESP_LOGW(TAG,
				"not possible to store leaf parameter value - key %s value %d",
				_buf, val);
		free(_buf);
		return GN_RET_ERR;
	}

	//gn_param_val_handle_int_t _val =
	//		(gn_param_val_handle_int_t) _param->param_val;

	//notify event loop
	gn_leaf_parameter_event_t evt;
	strcpy(evt.leaf_name, _leaf_config->name);
	strcpy(evt.param_name, _param->name);
	evt.id = GN_LEAF_PARAM_INITIALIZED_EVENT;
	evt.data_size = 1;
	if (!_param->param_val->v.b) {
		evt.data[0] = '0';
	} else {
		evt.data[0] = '1';
	}

	esp_err_t ret = esp_event_post_to(
			_leaf_config->node_config->config->event_loop, GN_BASE_EVENT,
			evt.id, &evt, sizeof(evt), portMAX_DELAY);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_init_bool - not possible to send param message to event loop - id:%d, size:%d - result: %d",
				evt.id, sizeof(evt), (int) ret);
		return GN_RET_ERR;
	}

	free(_buf);

	return GN_RET_OK;

}

/**
 * 	@brief	updates the parameter with new value
 *
 * 	the parameter value will be copied to the corresponding handle.
 * 	after the change the parameter change will be propagated to the event system through a GN_LEAF_PARAM_CHANGED_EVENT and to the server.
 *
 * 	@param leaf_config	the leaf handle to be queried
 * 	@param name			the name of the parameter (null terminated)
 * 	@param val			the value to set (null terminated)
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 */
gn_err_t gn_leaf_param_set_bool(const gn_leaf_handle_t leaf_config,
		const char *name, bool val) {

	if (!leaf_config || !name)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param)
		return GN_RET_ERR_INVALID_ARG;

	//ESP_LOGD(TAG, "gn_leaf_param_set_bool %s %d", name, val);
	//ESP_LOGD(TAG, "	old value %d", _param->param_val->v.b);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	if (_param->storage == GN_LEAF_PARAM_STORAGE_PERSISTED) {

		int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
		char *_buf = (char*) calloc(_len, sizeof(char));

		memcpy(_buf, _leaf_config->name,
				strlen(_leaf_config->name) * sizeof(char));
		memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
				1 * sizeof(char));
		memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
				strlen(name) * sizeof(char));

		_buf[_len - 1] = '\0';

		//check if existing
		if (gn_storage_set(_buf, (void**) &val, sizeof(bool)) != ESP_OK) {
			ESP_LOGW(TAG,
					"not possible to store leaf parameter value - key %s value %i",
					_buf, val);
			free(_buf);
			return GN_RET_ERR;
		}

		free(_buf);

	}

	if (_param->validator) {
		bool *p = &val;
		bool **validate = &p;
		gn_leaf_param_validator_result_t ret = _param->validator(_param,
				(void**) validate);
		if (ret != GN_LEAF_PARAM_VALIDATOR_ERROR
				&& ret != GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED)
			_param->param_val->v.b = **validate;
		//ESP_LOGD(TAG, "processing validator - result: %d", (int )ret);
	} else {
		_param->param_val->v.b = val;
	}
	//ESP_LOGD(TAG, "gn_leaf_param_set - result %d", _param->param_val->v.b);

	//notify event loop
	gn_leaf_parameter_event_t evt;
	strcpy(evt.leaf_name, _leaf_config->name);
	strcpy(evt.param_name, _param->name);
	evt.id = GN_LEAF_PARAM_CHANGED_EVENT;
	evt.data_size = 1;
	if (!_param->param_val->v.b) {
		evt.data[0] = '0';
	} else {
		evt.data[0] = '1';
	}

	esp_err_t ret = esp_event_post_to(
			_leaf_config->node_config->config->event_loop, GN_BASE_EVENT,
			evt.id, &evt, sizeof(evt), portMAX_DELAY);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_set_bool - not possible to send param message to event loop - id:%d, size:%d - result: %d",
				evt.id, sizeof(evt), (int) ret);
		return GN_RET_ERR;
	}

	return gn_mqtt_send_leaf_param(_param);

}

/**
 * 	@brief	updates the parameter with new value
 *
 * this is calling the gn_leaf_parameter_set_XXX depending on the param handle type, so be careful in order to avoid memory leaks
 *
 * 	@param leaf_config	the leaf handle to be queried
 * 	@param value		the pointer to value to set ( in case of string, null terminated)
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 */
gn_err_t gn_leaf_param_set_value(const gn_leaf_param_handle_t param_handle,
		const void *value) {

	if (!param_handle || !value)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) param_handle;

	//ESP_LOGD(TAG, "gn_leaf_param_set_bool %s %d", name, val);
	//ESP_LOGD(TAG, "	old value %d", _param->param_val->v.b);

	gn_leaf_config_handle_intl_t _leaf_config = _param->leaf_config;

	switch (_param->param_val->t) {

	case GN_VAL_TYPE_STRING:
		return gn_leaf_param_set_string(_leaf_config, _param->name,
				(char*) value);
		break;
	case GN_VAL_TYPE_BOOLEAN:
		return gn_leaf_param_set_bool(_leaf_config, _param->name,
				*(bool*) value);
		break;
	case GN_VAL_TYPE_DOUBLE:
		return gn_leaf_param_set_double(_leaf_config, _param->name,
				*(double*) value);
		break;
	default:
		return GN_RET_ERR_INVALID_ARG;

	}

	return GN_RET_ERR_INVALID_ARG;

}

gn_err_t gn_leaf_param_get_bool(const gn_leaf_handle_t leaf_config,
		const char *name, bool *val) {

	if (leaf_config == NULL || name == NULL)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		return GN_RET_ERR;
	}

	gn_param_val_handle_int_t _val =
			(gn_param_val_handle_int_t) _param->param_val;
	if (!_val) {
		return GN_RET_ERR;
	}

	*val = _val->v.b;

	return GN_RET_OK;

}

/**
 * 	@brief	init the parameter with new value and stores in NVS flash, overwriting previous values
 *
 *	the leaf must be not initialized to have an effect.
 * 	the parameter value will be copied to the corresponding handle.

 * 	@param leaf_config	the leaf handle to be queried
 * 	@param name			the name of the parameter (null terminated)
 * 	@param val			the value to set
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 */
gn_err_t gn_leaf_param_init_double(const gn_leaf_handle_t leaf_config,
		const char *name, double val) {

	if (!leaf_config || !name) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_init_double - wrong parameters");
		return ESP_ERR_INVALID_ARG;
	}

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_init_double - cannot find parameter %s", name);
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "gn_leaf_param_init_double %s %g", name, val);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
	char *_buf = (char*) calloc(_len, sizeof(char));

	memcpy(_buf, _leaf_config->name, strlen(_leaf_config->name) * sizeof(char));
	memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
			1 * sizeof(char));
	memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
			strlen(name) * sizeof(char));

	_buf[_len - 1] = '\0';

	//if already set keep old value
	if (gn_storage_get(_buf, (void**) &val) == ESP_OK) {
		ESP_LOGD(TAG, ".. value already found: (%f) - skipping", val);
		return GN_RET_OK;
	}

	if (_param->validator) {
		double *p = &val;
		double **validate = &p;
		gn_leaf_param_validator_result_t ret = _param->validator(_param,
				(void**) validate);
		if (ret != GN_LEAF_PARAM_VALIDATOR_ERROR
				&& ret != GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED)
			_param->param_val->v.d = **validate;
		ESP_LOGD(TAG, "processing validator - result: %d", (int ) ret);
	} else {
		_param->param_val->v.d = val;
	}

	//store the parameter
	if (gn_storage_set(_buf, (void**) &val, sizeof(double)) != ESP_OK) {
		ESP_LOGW(TAG,
				"not possible to store leaf parameter value - key %s value %f",
				_buf, val);
		free(_buf);
		return GN_RET_ERR;
	}

	gn_param_val_handle_int_t _val =
			(gn_param_val_handle_int_t) _param->param_val;

	_val->v.d = val;

	//notify event loop
	gn_leaf_parameter_event_t evt;
	strcpy(evt.leaf_name, _leaf_config->name);
	strcpy(evt.param_name, _param->name);
	evt.id = GN_LEAF_PARAM_INITIALIZED_EVENT;
	evt.data_size = sprintf(&evt.data[0], "%f", _param->param_val->v.d) + 1;

	esp_err_t ret = esp_event_post_to(
			_leaf_config->node_config->config->event_loop, GN_BASE_EVENT,
			evt.id, &evt, sizeof(evt), portMAX_DELAY);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_init_double - not possible to send param message to event loop - id:%d, size:%d - result: %d",
				evt.id, sizeof(evt), (int) ret);
		return GN_RET_ERR;
	}

	free(_buf);

	return GN_RET_OK;

}

/**
 * 	@brief	updates the parameter with new value
 *
 *	the leaf must be already initialized to have an effect.
 * 	the parameter value will be copied to the corresponding handle.
 * 	after the change the parameter change will be propagated to the event system through a GN_LEAF_PARAM_CHANGED_EVENT and to the server.
 *
 * 	@param leaf_config	the leaf handle to be queried
 * 	@param name			the name of the parameter (null terminated)
 * 	@param val			the value to set
 *
 * 	@return GN_RET_OK if the parameter is set
 * 	@return GN_RET_ERR_INVALID_ARG in case of input errors
 */
gn_err_t gn_leaf_param_set_double(const gn_leaf_handle_t leaf_config,
		const char *name, double val) {

	if (!leaf_config || !name)
		return ESP_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		return ESP_ERR_INVALID_ARG;
	}

	//ESP_LOGD(TAG, "gn_leaf_param_set_double %s %g", name, val);
	//ESP_LOGD(TAG, "	old value %g", _param->param_val->v.d);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	if (_param->storage == GN_LEAF_PARAM_STORAGE_PERSISTED) {

		int _len = (strlen(_leaf_config->name) + strlen(name) + 2);
		char *_buf = (char*) calloc(_len, sizeof(char));

		memcpy(_buf, _leaf_config->name,
				strlen(_leaf_config->name) * sizeof(char));
		memcpy(_buf + strlen(_leaf_config->name) * sizeof(char), "_",
				1 * sizeof(char));
		memcpy(_buf + (strlen(_leaf_config->name) + 1) * sizeof(char), name,
				strlen(name) * sizeof(char));

		_buf[_len - 1] = '\0';

		//check if existing
		if (gn_storage_set(_buf, (void**) &val, sizeof(double)) != ESP_OK) {
			ESP_LOGW(TAG,
					"not possible to store leaf parameter value - key %s value %f",
					_buf, val);
			free(_buf);
			return GN_RET_ERR;
		}

		free(_buf);

	}

	if (_param->validator) {
		double *p = &val;
		double **validate = &p;
		gn_leaf_param_validator_result_t ret = _param->validator(_param,
				(void**) validate);
		if (ret != GN_LEAF_PARAM_VALIDATOR_ERROR
				&& ret != GN_LEAF_PARAM_VALIDATOR_NOT_ALLOWED)
			_param->param_val->v.d = **validate;
		//ESP_LOGD(TAG, "processing validator - result: %d", (int )ret);
	} else {
		_param->param_val->v.d = val;
	}

	//ESP_LOGD(TAG, "gn_leaf_param_set - result %g", _param->param_val->v.d);

//notify event loop
	gn_leaf_parameter_event_t evt;
	strcpy(evt.leaf_name, _leaf_config->name);
	strcpy(evt.param_name, _param->name);
	evt.id = GN_LEAF_PARAM_CHANGED_EVENT;
	evt.data_size = sprintf(&evt.data[0], "%f", _param->param_val->v.d) + 1;

	esp_err_t ret = esp_event_post_to(
			_leaf_config->node_config->config->event_loop, GN_BASE_EVENT,
			evt.id, &evt, sizeof(evt), portMAX_DELAY);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_set_double - not possible to send param message to event loop - id:%d, size:%d - result: %d",
				evt.id, sizeof(evt), (int) ret);
		return GN_RET_ERR;
	}

	return gn_mqtt_send_leaf_param(_param);

}

/**
 * @brief gets the value pointed by the parameter
 * @param param the parameter handle to look at
 * @val the value returned
 */
gn_err_t gn_leaf_param_get_value(const gn_leaf_param_handle_t param, void *val) {

	if (!param || !val)
		return GN_RET_ERR_INVALID_ARG;

	gn_param_val_handle_int_t _val =
			((gn_leaf_param_handle_intl_t) param)->param_val;
	if (!_val) {
		return GN_RET_ERR;
	}

	switch (_val->t) {

	case GN_VAL_TYPE_STRING:
		val = &_val->v.s;
		break;
	case GN_VAL_TYPE_BOOLEAN:
		val = &_val->v.b;
		break;
	case GN_VAL_TYPE_DOUBLE:
		val = &_val->v.d;
		break;
	default:
		return GN_RET_ERR;

	}

	return GN_RET_OK;

}

gn_err_t gn_leaf_param_get_double(const gn_leaf_handle_t leaf_config,
		const char *name, double *val) {

	if (!leaf_config || !name)
		return GN_RET_ERR_INVALID_ARG;

	gn_leaf_param_handle_intl_t _param =
			(gn_leaf_param_handle_intl_t) gn_leaf_param_get_param_handle(
					leaf_config, name);
	if (!_param) {
		return GN_RET_ERR;
	}

	gn_param_val_handle_int_t _val =
			(gn_param_val_handle_int_t) _param->param_val;
	if (!_val) {
		return GN_RET_ERR;
	}

	*val = _val->v.d;

	return GN_RET_OK;

}

/**
 * update the parameter value from the event supplied.
 * this is called from event handling system. hence, the parameter value can be changed here only if it has WRITE access
 *
 * @return ESP_OK if parameter is changed,
 */
gn_err_t _gn_leaf_parameter_update(const gn_leaf_handle_t leaf_config,
		const char *param, const void *data, const int data_len) {

	if (!leaf_config || !param || !data || data_len == 0)
		return GN_RET_ERR_INVALID_ARG;

	ESP_LOGD(TAG, "gn_leaf_parameter_update. param=%s", param);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	gn_leaf_param_handle_intl_t leaf_params =
			(gn_leaf_param_handle_intl_t) _leaf_config->params;

	while (leaf_params != NULL) {

		//check param name
		if (strcmp(param, leaf_params->name) == 0) {
			//param is the one to update

			//check if has write access
			if (leaf_params->access != GN_LEAF_PARAM_ACCESS_NETWORK
					&& leaf_params->access != GN_LEAF_PARAM_ACCESS_ALL) {
				gn_log(TAG, GN_LOG_ERROR,
						"gn_leaf_parameter_update - paramater has no WRITE access, change discarded");
				return GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION;
			}

			//build event
			gn_leaf_parameter_event_t evt;
			evt.id = GN_LEAF_PARAM_CHANGE_REQUEST_EVENT;
			strncpy(evt.leaf_name, _leaf_config->name,
			GN_LEAF_NAME_SIZE);
			strncpy(evt.param_name, param,
			GN_LEAF_PARAM_NAME_SIZE);

			memcpy(&evt.data[0], data, data_len);
			evt.data_size = data_len;

			//send message to the interested leaf
			return _gn_send_event_to_leaf(_leaf_config, &evt);

		}

		leaf_params = leaf_params->next;
	}

	return GN_RET_OK;

}

gn_err_t _gn_leaf_param_destroy(gn_leaf_param_handle_t param) {

	gn_leaf_param_handle_intl_t new_param = (gn_leaf_param_handle_intl_t) param;

	if (!new_param)
		return GN_RET_ERR_INVALID_ARG;

	free(new_param->param_val->v.s);
	free(new_param->param_val);
	free(new_param->name);
	free(new_param);

	return GN_RET_OK;

}

/**
 * @brief 	add a parameter to the leaf.
 *
 * the parameter will then listen to server changes
 *
 * @param leaf 			the leaf handle
 * @param new_param		the param to add to the leaf. the leaf will point at it upon method return
 *
 * @return   	GN_RET_ERR_INVALID_ARG	in case of parameter errors
 * @return		GN_RET_OK				upon success
 */
gn_err_t gn_leaf_param_add_to_leaf(const gn_leaf_handle_t leaf,
		const gn_leaf_param_handle_t param) {

	gn_leaf_param_handle_intl_t new_param = (gn_leaf_param_handle_intl_t) param;

	if (!leaf || !new_param) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_param_add incorrect parameters");
		return GN_RET_ERR_INVALID_ARG;
	}

	gn_leaf_param_handle_intl_t _param =
			((gn_leaf_config_handle_intl_t) leaf)->params;

	while (_param) {
		if (strcmp(_param->name, new_param->name) == 0) {
			gn_log(TAG, GN_LOG_ERROR,
					"Parameter with name %s already exists in Leaf %s",
					new_param->name,
					((gn_leaf_config_handle_intl_t) leaf)->name);
			return GN_RET_ERR_INVALID_ARG;
		}
		if (_param->next) {
			_param = _param->next;
		} else {
			break;
		}
	}

	new_param->leaf_config = leaf;
	if (_param) {
		_param->next = new_param;
	} else {
		((gn_leaf_config_handle_intl_t) leaf)->params = new_param;
	}

	gn_err_t ret = GN_RET_OK;

	ret = gn_mqtt_subscribe_leaf_param(new_param);
	if (ret != GN_RET_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_add failed to subscribe param %s of leaf %s",
				new_param->name, ((gn_leaf_config_handle_intl_t) leaf)->name);
		return ret;
	}

	ret = gn_mqtt_send_leaf_param(new_param);
	if (ret != GN_RET_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_param_add failed to send param configuration %s of leaf %s",
				new_param->name, ((gn_leaf_config_handle_intl_t) leaf)->name);
		return ret;
	}

	ESP_LOGD(TAG, "Param %s added in %s", new_param->name,
			((gn_leaf_config_handle_intl_t ) leaf)->name);
	return GN_RET_OK;

}

/**
 * @brief send parameter status per each parameter
 *
 * @param _node_config the config
 *
 *
 * @return		GN_RET_OK				upon success
 */
gn_err_t gn_send_node_leaf_param_status(
		const gn_node_handle_t _node_config) {

	gn_node_config_handle_intl_t node_config =
			(gn_node_config_handle_intl_t) _node_config;

	//run leaves
	for (int i = 0; i < node_config->leaves.last; i++) {

		gn_leaf_config_handle_intl_t leaf_config = node_config->leaves.at[i];

		gn_leaf_param_handle_intl_t _param =
				(gn_leaf_param_handle_intl_t) leaf_config->params;
		while (_param) {

			if (_param->storage == GN_LEAF_PARAM_STORAGE_PERSISTED) {

				gn_err_t ret = gn_mqtt_send_leaf_param(_param);
				if (ret != GN_RET_OK) {
					gn_log(TAG, GN_LOG_ERROR,
							"gn_leaf_param_add failed to send param configuration %s of leaf %s",
							_param->name, leaf_config->name);
					return ret;
				}
				//vTaskDelay(200 / portTICK_PERIOD_MS);
			}

			_param = _param->next;
		}
	}

	return GN_RET_OK;
}

/**
 * 	@brief send a request to change a parameter name
 *
 * 	It sends a GN_LEAF_PARAM_CHANGE_REQUEST_EVENT to the leaf parameter, if the parameter is modifiable
 *
 * 	@param leaf_name 	the leaf name (null terminated) to send the request to
 * 	@param param_name	the parameter name to change (null terminated)
 * 	@param message		a pointer to the payload
 * 	@param message_len	size of the payload
 *
 * 	@return GN_RET_ERR_LEAF_NOT_FOUND if the leaf is not found
 * 	@return GN_RET_ERR_INVALID_ARG in case of input parameter error
 * 	@return GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION in case the parameter access is not write enable
 *
 */
gn_err_t gn_send_leaf_param_change_message(const char *leaf_name,
		const char *param_name, const void *message, size_t message_len) {

	if (leaf_name == NULL || param_name == NULL || message == NULL) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_send_leaf_param_change_message - invalid args");
		return GN_RET_ERR_INVALID_ARG;
	}

	gn_leaves_list leaves = _gn_default_conf->node_config->leaves;

	for (int i = 0; i < leaves.last; i++) {

		if (strcmp(leaves.at[i]->name, leaf_name) == 0) {

			return _gn_leaf_parameter_update(leaves.at[i], param_name, message,
					message_len);

		}
	}
	gn_log(TAG, GN_LOG_ERROR,
			"gn_send_leaf_param_change_message(%s) - leaf not found",
			leaf_name);
	return GN_RET_ERR_LEAF_NOT_FOUND;
}

/**
 * @brief generate a request to update the parameter to the leaf.
 *
 * This is different from the corresponding 'set' method as it inform the leaf
 * that a parameter should be changed. Think of it as it would be requested
 * by the network. It is the basis of inter-leaves messaging.
 *
 * @param	leaf_config	the leaf to ask
 * @param	name	the parameter name to change
 * @param	val		the value to change
 *
 * 	@return GN_RET_ERR_LEAF_NOT_FOUND if the leaf is not found
 * 	@return GN_RET_ERR_INVALID_ARG in case of input parameter error
 * 	@return GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION in case the parameter access is not write enable

 */
gn_err_t gn_leaf_param_update_bool(const gn_leaf_handle_t leaf_config,
		const char *name, bool val) {
	if (leaf_config == NULL || name == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_param_send_bool - invalid args");
		return GN_RET_ERR_INVALID_ARG;
	}
	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;
	char val_ptr[2] = "";
	strcpy(val_ptr, val ? "1" : "0");
	gn_err_t ret = gn_send_leaf_param_change_message(_leaf_config->name, name,
			val_ptr, 2);
	return ret;
}

/**
 * @brief generate a request to update the parameter to the leaf.
 *
 * This is different from the corresponding 'set' method as it inform the leaf
 * that a parameter should be changed. Think of it as it would be requested
 * by the network. It is the basis of inter-leaves messaging.
 *
 * @param	leaf_config	the leaf to ask
 * @param	name	the parameter name to change
 * @param	val		the value to change
 *
 * 	@return GN_RET_ERR_LEAF_NOT_FOUND if the leaf is not found
 * 	@return GN_RET_ERR_INVALID_ARG in case of input parameter error
 * 	@return GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION in case the parameter access is not write enable

 */
gn_err_t gn_leaf_param_update_double(const gn_leaf_handle_t leaf_config,
		const char *name, double val) {
	if (leaf_config == NULL || name == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_param_send_bool - invalid args");
		return GN_RET_ERR_INVALID_ARG;
	}
	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;
	char val_ptr[32] = "";
	sprintf(val_ptr, "%f", val);
	gn_err_t ret = gn_send_leaf_param_change_message(_leaf_config->name, name,
			val_ptr, sizeof(double));
	return ret;
}

/**
 * @brief generate a request to update the parameter to the leaf.
 *
 * This is different from the corresponding 'set' method as it inform the leaf
 * that a parameter should be changed. Think of it as it would be requested
 * by the network. It is the basis of inter-leaves messaging.
 *
 * @param	leaf_config	the leaf to ask
 * @param	name	the parameter name to change
 * @param	val		the value to change
 *
 * 	@return GN_RET_ERR_LEAF_NOT_FOUND if the leaf is not found
 * 	@return GN_RET_ERR_INVALID_ARG in case of input parameter error
 * 	@return GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION in case the parameter access is not write enable

 */
gn_err_t gn_leaf_param_update_string(const gn_leaf_handle_t leaf_config,
		const char *name, char *val) {
	if (leaf_config == NULL || name == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_param_send_bool - invalid args");
		return GN_RET_ERR_INVALID_ARG;
	}
	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;
	char *val_ptr = (char*) calloc(strlen(val) + 1, sizeof(char));
	strncpy(val_ptr, val, sizeof(val) + 1);
	gn_err_t ret = gn_send_leaf_param_change_message(_leaf_config->name, name,
			val_ptr, sizeof(val) + 1);
	free(val_ptr);
	return ret;
}

/**
 * 	@brief returns the first parameter associated to the leaf
 *
 *	@param leaf_config the leaf handle to search within
 *
 *	@return NULL if leaf_config is not found
 *	@return the first parameter handle
 */
gn_leaf_param_handle_t gn_leaf_get_params(gn_leaf_handle_t leaf_config) {

	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->params;

}

/**
 * @brief returns the specific parameter associated to the leaf
 *
 *	@param 	leaf_config the leaf handle to search within
 *	@param	param_name the name of the parameter (null terminated)
 *
 *	@return NULL if leaf_config or the parameter is not found
 *	@return the found parameter handle
 */
gn_leaf_param_handle_t gn_leaf_param_get_param_handle(
		const gn_leaf_handle_t leaf_config, const char *param_name) {

	if (leaf_config == NULL || param_name == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_param_get incorrect parameters");
		return NULL;
	}
	gn_leaf_param_handle_intl_t param =
			((gn_leaf_config_handle_intl_t) leaf_config)->params;

	while (param) {
		//ESP_LOGD(TAG,
		//		"gn_leaf_param_get_param_handle - comparing %s (%d) and checking %s (%d)",
		//		param_name, strlen(param_name), param->name,
		//		strlen(param->name));
		if (strncmp(param->name, param_name, strlen(param_name)) == 0) {
			//ESP_LOGD(TAG, "found!");
			return param;
		}
		param = param->next;
	}

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;
	ESP_LOGW(TAG, "param '%s' on leaf '%s' not found", param_name,
			_leaf_config->name);
	return NULL;

}

void* _gn_leaf_context_add_to_leaf(const gn_leaf_handle_t leaf,
		char *key, void *value) {

	if (!leaf || !key || !value) {
		gn_log(TAG, GN_LOG_ERROR, "gn_leaf_context_add incorrect parameters");
		return NULL;
	}

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) leaf;
	if (!leaf_config->leaf_context)
		return NULL;

	return gn_leaf_context_set(leaf_config->leaf_context, key, value);
}

void* _gn_leaf_context_remove_to_leaf(const gn_leaf_handle_t leaf,
		char *key) {

	if (!leaf || !key) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_context_remove_to_leaf incorrect parameters");
		return NULL;
	}

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) leaf;
	if (!leaf_config->leaf_context)
		return NULL;

	return gn_leaf_context_delete(leaf_config->leaf_context, key);
}

void* _gn_leaf_context_get_key_to_leaf(const gn_leaf_handle_t leaf,
		char *key) {

	if (!leaf || !key) {
		gn_log(TAG, GN_LOG_ERROR,
				"gn_leaf_context_remove_to_leaf incorrect parameters");
		return NULL;
	}

	gn_leaf_config_handle_intl_t leaf_config =
			(gn_leaf_config_handle_intl_t) leaf;
	if (!leaf_config->leaf_context)
		return NULL;

	return gn_leaf_context_get(leaf_config->leaf_context, key);
}

/*
 void _gn_leaf_task(void *pvParam) {

 //wait for events, if not raise an execute event to cycle execution
 gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) pvParam;

 //make sure the init event is processed before anything else
 gn_event_handle_t _init_evt = (gn_event_handle_t) calloc(
 sizeof(gn_event_t));
 _init_evt->id = GN_LEAF_INIT_REQUEST_EVENT;
 _init_evt->data = NULL;
 _init_evt->data_size = 0;

 leaf_config->callback(_init_evt, leaf_config);

 free(_init_evt);

 //notice network of the leaf added
 _gn_mqtt_subscribe_leaf(leaf_config);

 gn_event_t evt;

 while (true) {
 //wait for events, otherwise run execution
 if (xQueueReceive(leaf_config->xLeafTaskEventQueue, &evt,
 (TickType_t) 10) == pdPASS) {
 ESP_LOGD(TAG, "_gn_leaf_task %s event received %d",
 leaf_config->name, evt.id);
 //event received
 leaf_config->callback(&evt, leaf_config);
 } else {
 //run
 //ESP_LOGD(TAG, "_gn_leaf_task %s loop", leaf_config->name);
 leaf_config->loop(leaf_config);
 }

 vTaskDelay(1);
 }

 }
 */

/**
 * 	@brief send a message to the display
 *
 *	implemented by sending an internal GN_DISPLAY_LOG_EVENT event
 * 	NOTE: data will be truncated depending on display size
 *
 * 	@param	message	the message to send (null terminated)
 *
 * 	@return GN_RET_OK if event is dispatched
 * 	@return GN_RET_ERR if the event dispatch encounters a problem
 * 	@return GN_RET_ERR_INVALID_ARG if message is NULL or zero length
 */
/*
 gn_err_t gn_display_log(char *message) {

 if (!message || strlen(message) == 0)
 return GN_RET_ERR_INVALID_ARG;

 esp_err_t ret = ESP_OK;

 //void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
 ESP_LOGI(TAG, "gn_log: %s", message);

 //char *ptr = calloc(sizeof(char) * strlen(message) + 1);

 ret = esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_GUI_LOG_EVENT,
 message, strlen(message) + 1,
 portMAX_DELAY);

 //free(ptr);
 return ret == ESP_OK ? GN_RET_OK : GN_RET_ERR;

 }
 */

/*
 esp_err_t gn_event_send_internal(gn_config_handle_t conf,
 gn_leaf_parameter_event_handle_t event) {

 gn_config_handle_intl_t _conf = (gn_config_handle_intl_t) conf;

 esp_err_t ret = ESP_OK;

 ret = esp_event_post_to(_conf->event_loop, GN_BASE_EVENT, event->id, event,
 sizeof(event),
 portMAX_DELAY);

 if (ret != ESP_OK) {
 gn_log(TAG, GN_LOG_ERROR, "failed to send internal event");
 }
 return ret;

 }
 */

/**
 * @brief		starts the OTA firmware upgrade
 *
 * it starts the OTA tasks so it returns immediately
 *
 * @return		GN_RET_OK
 */
gn_err_t gn_firmware_update() {

#if CONFIG_GROWNODE_WIFI_ENABLED
	if (gn_mqtt_send_ota_message(_gn_default_conf) != GN_RET_OK) {
		ESP_LOGE(TAG, "OTA message not sent");
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	xTaskCreate(_gn_ota_task, "gn_ota_task", 8196, NULL, 10, NULL);
#endif
	return GN_RET_OK;

}

/**
 * @brief		reset the flash content and restart the board immediately
 *
 * @return		GN_RET_OK
 */
gn_err_t gn_reset() {

	gn_mqtt_send_reset_message(_gn_default_conf);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	nvs_flash_erase();
	gn_reboot();
	return GN_RET_OK;

}

/**
 * @brief write ESP log, send log in the event queue and publish in network
 *
 * @param	log_tag		log level, will be the TAG in ESP logging framework
 * @param	level		grownode log level
 * @param	message		the null terminated message to log
 *
 * 	@return GN_RET_OK if event is dispatched
 * 	@return GN_RET_ERR if the event dispatch encounters a problem
 * 	@return GN_RET_ERR_INVALID_ARG if message is NULL or zero length
 *
 */
gn_err_t gn_log(char *log_tag, gn_log_level_t level, const char *message, ...) {

	if (!log_tag || !message)
		return GN_RET_ERR_INVALID_ARG;

	char formatted_message[512];
	va_list argptr;
	va_start(argptr, message);
	vsnprintf(formatted_message, 511, message, argptr);
	va_end(argptr);

	switch (level) {
	case GN_LOG_DEBUG:
		ESP_LOGD(log_tag, "%s", formatted_message);
		break;
	case GN_LOG_INFO:
		ESP_LOGI(log_tag, "%s", formatted_message);
		break;
	case GN_LOG_WARNING:
		ESP_LOGW(log_tag, "%s", formatted_message);
		break;
	case GN_LOG_ERROR:
		ESP_LOGE(log_tag, "%s", formatted_message);
		break;
	default:
		ESP_LOGD(log_tag, "%s", formatted_message);
		break;
	}

	//TODO pass the log level as well
	gn_err_t ret = esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_LOG_EVENT,
			formatted_message, 256, portMAX_DELAY);

	if (ret != GN_RET_OK) {
		gn_log(TAG, GN_LOG_ERROR, "Not possible to post log event: %s",
				formatted_message);
		return ret;
	}

	return gn_mqtt_send_log_message(_gn_default_conf, log_tag, level,
			formatted_message);

}

/**
 * @brief		reboot the board
 *
 * @return		GN_RET_OK
 */
gn_err_t gn_reboot() {

	gn_mqtt_send_reboot_message(_gn_default_conf);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
	return GN_RET_OK;

}

/**
 * 	@brief		performs the initialization workflow
 *
 * 	- creates the configuration handle
 * 	- initializes hardware (flash, storage)
 * 	- initializes event loop and handlers
 * 	- initializes display if configured
 * 	- initializes network if configured (starting provisioning is not set)
 * 	- initializes server connection
 *
 * 	this is a process that will continue even after the function returns, eg for network/server connection
 *
 * 	when everything is OK it sets the status of the config handle to GN_CONFIG_STATUS_ERROR
 *
 * 	NOTE: if called several times, it returns always the same handle
 *
 * 	@return		an handle to the config data structure
 *
 */
gn_config_handle_t gn_init(gn_config_init_param_t *config_init) {

	gn_err_t ret = GN_RET_OK;

	if (_gn_default_conf)
		return _gn_default_conf;

	ESP_LOGD(TAG, "gn_init");

//_gn_xEvtSemaphore = xSemaphoreCreateMutex();

	_gn_default_conf = _gn_config_create(config_init);

	if (_gn_default_conf->status != GN_NODE_STATUS_INITIALIZING) {
		return _gn_default_conf;
	}

//init flash
	ESP_GOTO_ON_ERROR(_gn_init_flash(_gn_default_conf), err, TAG,
			"error init flash: %s", esp_err_to_name(ret));

//init spiffs
	ESP_GOTO_ON_ERROR(_gn_init_spiffs(_gn_default_conf), err, TAG,
			"error init spiffs: %s", esp_err_to_name(ret));

//init event loop
	ESP_GOTO_ON_ERROR(_gn_init_event_loop(_gn_default_conf), err, TAG,
			"error init_event_loop: %s", esp_err_to_name(ret));

//register to events
	ESP_GOTO_ON_ERROR(_gn_evt_handlers_register(_gn_default_conf), err, TAG,
			"error _gn_register_event_handlers: %s", esp_err_to_name(ret));

//heartbeat to check network comm and send periodical system watchdog to the network
	ESP_GOTO_ON_ERROR(_gn_init_keepalive_timer(_gn_default_conf), err, TAG,
			"error on timer init: %s", esp_err_to_name(ret));

//init display
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	ESP_GOTO_ON_ERROR(gn_init_display(_gn_default_conf), err, TAG,
			"error on display init: %s", esp_err_to_name(ret));
#endif

#if CONFIG_GROWNODE_WIFI_ENABLED
//init wifi
	ESP_GOTO_ON_ERROR(_gn_init_wifi(_gn_default_conf), err_net, TAG,
			"error on wifi init: %s", esp_err_to_name(ret));

//init time sync. note: if bad, continue
	ESP_GOTO_ON_ERROR(_gn_init_time_sync(_gn_default_conf), err_timesync, TAG,
			"error on time sync init: %s", esp_err_to_name(ret));

	err_timesync:
//init mqtt system
	ESP_GOTO_ON_ERROR(gn_mqtt_init(_gn_default_conf), err_srv, TAG,
			"error on server init: %s", esp_err_to_name(ret));
#endif

	ESP_LOGI(TAG, "grownode startup sequence completed!");
	_gn_default_conf->status = GN_NODE_STATUS_READY_TO_START;
	return _gn_default_conf;

	err: _gn_default_conf->status = GN_NODE_STATUS_ERROR;
	return _gn_default_conf;

#if CONFIG_GROWNODE_WIFI_ENABLED
	err_net: _gn_default_conf->status = GN_NODE_STATUS_NETWORK_ERROR;
	return _gn_default_conf;
	err_srv: _gn_default_conf->status = GN_NODE_STATUS_SERVER_ERROR;
	return _gn_default_conf;
#endif

}

#define STORAGE_NAMESPACE "grownode"

/**
 *	@brief stores the key into the NVS flash
 *
 *	internally, this is implemented by copying raw bytes to the flash storage
 *
 *	@param key name (null terminated)
 *	@param value	pointer to data
 *	@param required_size	bytes to write
 *
 *	@return GN_RET_ERR_INVALID_ARG if input params are not valid
 *	@return GN_RET_OK if key is stored successfully
 *
 */
gn_err_t gn_storage_set(const char *key, const void *value,
		size_t required_size) {

	if (!key || !value || required_size == 0)
		return GN_RET_ERR_INVALID_ARG;

	esp_err_t err = GN_RET_OK;

	/*
	 if (strlen(key) >= NVS_KEY_NAME_MAX_SIZE - 1) {
	 gn_log(TAG, GN_LOG_ERROR, "gn_storage_set() failed - key too long");
	 goto fail;
	 }
	 */

	nvs_handle_t my_handle;

// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG_NVS, "gn_storage_set() failed -nvs_open() error: %d", err);
		goto fail;
	}

	int len = NVS_KEY_NAME_MAX_SIZE - 1;
	char _hashedkey[NVS_KEY_NAME_MAX_SIZE];
	gn_hash_str(key, _hashedkey, len);

	err = nvs_set_blob(my_handle, _hashedkey, value, required_size);

	if (err != ESP_OK) {
		ESP_LOGE(TAG_NVS, "gn_storage_set() failed -nvs_set_blob() error: %d",
				err);
		goto fail;
	}

// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG_NVS, "gn_storage_set() failed -nvs_commit() error: %d",
				err);
		goto fail;
	}

// Close
	nvs_close(my_handle);
	ESP_LOGD(TAG_NVS, "gn_storage_set(%s) - ESP_OK", key);
	return GN_RET_OK;

	fail: ESP_LOGD(TAG_NVS, "gn_storage_set(%s) - FAIL", key);

	nvs_close(my_handle);
	return err == GN_RET_OK || ESP_OK ? GN_RET_OK : GN_RET_ERR;

}

/**
 *	@brief retrieves the key from the NVS flash
 *
 *	internally, this is implemented by retrieving raw bytes to the flash storage
 *
 *	@param key name (null terminated)
 *	@param value	pointer where the pointer of the data acquired will be stored
 *
 *	@return GN_RET_ERR_INVALID_ARG if input params are not valid
 *	@return GN_RET_OK if key is stored successfully
 *
 */
gn_err_t gn_storage_get(const char *key, void **value) {

	if (!key || !value)
		return GN_RET_ERR_INVALID_ARG;

	nvs_handle_t my_handle;
	esp_err_t err = GN_RET_OK;

// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG_NVS, "nvs_open(%s, READWRITE, handle) - %d",
				STORAGE_NAMESPACE, err);
		goto fail;
	}

	int len = NVS_KEY_NAME_MAX_SIZE - 1;
	char _hashedkey[NVS_KEY_NAME_MAX_SIZE];
	gn_hash_str(key, _hashedkey, len);

// Read the size of memory space required for blob
	size_t required_size = 0; // value will default to 0, if not set yet in NVS
	err = nvs_get_blob(my_handle, _hashedkey, NULL, &required_size);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGD(TAG_NVS, "nvs_get_blob(handle, %s, NULL, %d) - %d", key,
				required_size, err);
		goto fail;
	}

	if (required_size > 0) {

		// Read previously saved blob if available
		*value = malloc(required_size + sizeof(uint32_t));

		err = nvs_get_blob(my_handle, _hashedkey, *value, &required_size);

		if (err != ESP_OK) {
			ESP_LOGD(TAG_NVS, "nvs_get_blob(handle, %s, %s, %d) - %d", key,
					(char* ) *value, required_size, err);
			free(*value);
			goto fail;
		}
		ESP_LOGD(TAG_NVS, "gn_storage_get(%s) - %s - ESP_OK", key,
				(char* ) *value);
	} else
		goto fail;

// Close
	nvs_close(my_handle);
	return GN_RET_OK;

	fail:
// Close
	ESP_LOGD(TAG, "gn_storage_get(%s) - FAIL", key);
	nvs_close(my_handle);
	return err == GN_RET_OK || ESP_OK ? GN_RET_OK : GN_RET_ERR;

}

#ifdef __cplusplus
}
#endif //__cplusplus
