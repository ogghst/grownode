/*
 * grownode.c
 *
 *  Created on: 17 apr 2021
 *      Author: muratori.n
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_check.h"

//TODO switch from LOGI to LOGD
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
//#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
//#include "esp_smartconfig.h"

#include "nvs_flash.h"

#include "driver/timer.h"
#include "driver/gpio.h"

#include "mqtt_client.h"

#include "lwip/apps/sntp.h"

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

#include "gn_commons.h"
#include "gn_network.h"
#include "gn_event_source.h"
#include "gn_mqtt_protocol.h"
#include "gn_display.h"

static const char *TAG = "grownode";

esp_event_loop_handle_t gn_event_loop;

gn_config_handle_t _gn_default_conf;

SemaphoreHandle_t _gn_xEvtSemaphore;

bool initialized = false;

ESP_EVENT_DEFINE_BASE(GN_BASE_EVENT);
ESP_EVENT_DEFINE_BASE(GN_LEAF_EVENT);

esp_err_t _gn_start_leaf(gn_leaf_config_handle_t leaf_config);

gn_config_handle_t _gn_create_config() {
	gn_config_handle_t _conf = (gn_config_handle_t) malloc(sizeof(gn_config_t));
	_conf->status = GN_CONFIG_STATUS_NOT_INITIALIZED;

	return _conf;
}

esp_err_t _gn_init_event_loop(gn_config_handle_t conf) {

	esp_err_t ret = ESP_OK;

	//default event loop
	ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), fail, TAG,
			"error creating system event loop: %s", esp_err_to_name(ret));

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5, .task_name =
			"gn_evt_loop", // task will be created
			.task_priority = 0, .task_stack_size = 2048, .task_core_id = 1 };
	ESP_GOTO_ON_ERROR(esp_event_loop_create(&event_loop_args, &gn_event_loop),
			fail, TAG, "error creating grownode event loop: %s",
			esp_err_to_name(ret));

	conf->event_loop = gn_event_loop;

	fail: return ret;

}

gn_node_config_handle_t _gn_create_node_config() {
	gn_node_config_handle_t _conf = (gn_node_config_handle_t) malloc(
			sizeof(gn_node_config_t));
	_conf->config = NULL;
	//_conf->event_loop = NULL;
	_conf->name = NULL;
	return _conf;
}

gn_node_config_handle_t gn_create_node(gn_config_handle_t config,
		const char *name) {

	if (config == NULL || config->mqtt_client == NULL || name == NULL) {
		ESP_LOGE(TAG, "gn_create_node failed. parameters not correct");
		return NULL;
	}

	gn_node_config_handle_t n_c = _gn_create_node_config();

	//n_c->name = bf;
	n_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(n_c->name, name, GN_MEM_NAME_SIZE);
	//n_c->event_loop = config->event_loop;
	n_c->config = config;

	//create leaves
	gn_leaves_list leaves = { .size = 5, .last = 0 };

	n_c->leaves = leaves;
	config->node_config = n_c;

	return n_c;
}

esp_err_t gn_destroy_node(gn_node_config_handle_t node) {

	free(node->leaves.at);
	free(node->name);
	free(node);

	return ESP_OK;
}

esp_err_t gn_start_node(gn_node_config_handle_t node) {

	esp_err_t ret = ESP_OK;
	ESP_LOGD(TAG, "gn_start_node: %s", node->name);

	//publish node
	if (_gn_mqtt_send_node_config(node) != ESP_OK)
		goto fail;

	//run leaves
	for (int i = 0; i < node->leaves.last; i++) {
		ESP_LOGD(TAG, "starting leaf: %d", i);
		_gn_start_leaf(node->leaves.at[i]);
	}

	return ret;
	fail: return ESP_FAIL;

}

gn_leaf_config_handle_t _gn_create_leaf_config() {
	gn_leaf_config_handle_t _conf = (gn_leaf_config_handle_t) malloc(
			sizeof(gn_leaf_config_t));
	_conf->callback = NULL;
	_conf->name = NULL;
	_conf->node_config = NULL;
	_conf->loop = NULL;
	return _conf;
}

gn_leaf_config_handle_t gn_create_leaf(gn_node_config_handle_t node_cfg,
		const char *name, gn_leaf_event_callback_t callback,
		gn_leaf_loop_callback_t loop) {

	if (node_cfg == NULL || node_cfg->config == NULL
			|| node_cfg->config->mqtt_client == NULL || name == NULL
			|| callback == NULL || loop == NULL) {
		ESP_LOGE(TAG, "gn_create_leaf failed. parameters not correct");
		return NULL;
	}

	gn_leaf_config_handle_t l_c = _gn_create_leaf_config();

	l_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(l_c->name, name, GN_MEM_NAME_SIZE);
	l_c->node_config = node_cfg;
	l_c->callback = callback;
	l_c->loop = loop;
	l_c->xLeafTaskEventQueue = xQueueCreate(10, sizeof(gn_event_t));
	if (l_c->xLeafTaskEventQueue == NULL) {
		return NULL;
	}

	//TODO add leaf to node. implement dynamic array
	if (l_c->node_config->leaves.last >= l_c->node_config->leaves.size - 1) {
		ESP_LOGE(TAG,
				"gn_create_leaf failed. not possible to add more than 5 leaves to a node");
		return NULL;
	}

	l_c->node_config->leaves.at[l_c->node_config->leaves.last] = l_c;
	l_c->node_config->leaves.last++;

	return l_c;
}

esp_err_t gn_destroy_leaf(gn_leaf_config_handle_t leaf) {

	vQueueDelete(leaf->xLeafTaskEventQueue);
	free(leaf->name);
	free(leaf);

	return ESP_OK;

}

/*
void _gn_leaf_task(void *pvParam) {

	//wait for events, if not raise an execute event to cycle execution
	gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) pvParam;

	//make sure the init event is processed before anything else
	gn_event_handle_t _init_evt = (gn_event_handle_t) malloc(
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

esp_err_t _gn_start_leaf(gn_leaf_config_handle_t leaf_config) {

	int ret = ESP_OK;
	ESP_LOGI(TAG, "_gn_start_leaf %s", leaf_config->name);
//TODO not valid to pass the entire context, as the leaf can do everything. better pass only name and us grownode functions to protect context
	if (xTaskCreate(leaf_config->loop, leaf_config->name, 2048, leaf_config, 1,
	NULL) != pdPASS) {
		ESP_LOGE(TAG, "failed to create lef task for %s", leaf_config->name);
		goto fail;
	}


	//notice network of the leaf added
	_gn_mqtt_subscribe_leaf(leaf_config);

	return ret;

	fail: return ESP_FAIL;

}

esp_err_t _gn_init_flash(gn_config_handle_t conf) {

	esp_err_t ret = ESP_OK;
	/* Initialize NVS partition */
	ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		/* NVS partition was truncated
		 * and needs to be erased */
		ESP_GOTO_ON_ERROR(nvs_flash_erase(), err, TAG,
				"error erasing flash: %s", esp_err_to_name(ret));
		/* Retry nvs_flash_init */
		ESP_GOTO_ON_ERROR(nvs_flash_init(), err, TAG, "error init flash: %s",
				esp_err_to_name(ret));

	}

#ifdef CONFIG_GROWNODE_RESET_PROVISIONED
	ESP_GOTO_ON_ERROR(nvs_flash_erase(), fail, TAG, "error erasing flash", esp_err_to_name(ret));
	/* Retry nvs_flash_init */
	ESP_GOTO_ON_ERROR(nvs_flash_init(), fail, TAG, "error init flash", esp_err_to_name(ret));
#endif

	err: return ret;

}

esp_err_t _gn_init_spiffs(gn_config_handle_t conf) {

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
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",
					esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
				esp_err_to_name(ret));
	} else {
		ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	return ret;

}

esp_err_t gn_log_message(char *message) {

	esp_err_t ret = ESP_OK;

	//void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
	ESP_LOGD(TAG, "gn_log_message '%s' (%i)", message, strlen(message));

	//char *ptr = malloc(sizeof(char) * strlen(message) + 1);

	ret = esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_DISPLAY_LOG_EVENT,
			message, strlen(message) + 1, portMAX_DELAY);

	//free(ptr);
	return ret;
}

esp_err_t gn_send_text_message(gn_leaf_config_handle_t leaf, const char *msg) {	//TODO remove leaf config

	return _gn_mqtt_send_leaf_status(leaf, msg);

}

void _gn_update_firmware() {
	xTaskCreate(_gn_ota_task, "gn_ota_task", 8196, NULL, 10, NULL);
}

void _gn_forward_evt(esp_event_base_t base, int32_t id, void *event_data) {

	if (_gn_default_conf->status == GN_CONFIG_STATUS_OK) { //TODO find a more elegant way to understand if nodes are present

		gn_event_t evt = { id, event_data, 0 };

		//TODO create a task to process callbacks
		//callback to all leaves
		for (int i = 0; i < _gn_default_conf->node_config->leaves.last; i++) {

			ESP_LOGD(TAG, "sending evt %d to %s", id,
					_gn_default_conf->node_config->leaves.at[i]->name);

			if (xQueueSend(_gn_default_conf->node_config->leaves.at[i]->xLeafTaskEventQueue,
					&evt, 0) != pdTRUE) {
				ESP_LOGE(TAG, "failed to send evt %d to %s", id,
						_gn_default_conf->node_config->leaves.at[i]->name);
			}
		}
	}
}

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

static bool IRAM_ATTR _gn_timer_callback_isr(void *args) {
	BaseType_t high_task_awoken = pdFALSE;
	esp_event_isr_post_to(gn_event_loop, GN_BASE_EVENT,
			GN_KEEPALIVE_START_EVENT, NULL, 0, &high_task_awoken);
	return high_task_awoken == pdTRUE;
}

void _gn_keepalive_start() {

	timer_start(TIMER_GROUP_0, TIMER_0);
	ESP_LOGD(TAG, "timer started");
}

void _gn_keepalive_stop() {
	timer_pause(TIMER_GROUP_0, TIMER_0);
	ESP_LOGD(TAG, "timer paused");
}

void _gn_evt_handler(void *handler_args, esp_event_base_t base, int32_t id,
		void *event_data) {

	if (pdTRUE == xSemaphoreTake(_gn_xEvtSemaphore, portMAX_DELAY)) {

		ESP_LOGD(TAG, "_gn_evt_handler event: %d", id);

		switch (id) {
		case GN_NET_OTA_START:
			_gn_update_firmware();
			break;

		case GN_NET_RST_START:
			nvs_flash_erase();
			esp_restart();
			break;

		case GN_NETWORK_CONNECTED_EVENT:
			_gn_forward_evt(base, id, event_data);
			break;
		case GN_NETWORK_DISCONNECTED_EVENT:
			_gn_forward_evt(base, id, event_data);
			break;
		case GN_SERVER_CONNECTED_EVENT:
			//start keepalive service
			_gn_keepalive_start();
			_gn_forward_evt(base, id, event_data);
			break;
		case GN_SERVER_DISCONNECTED_EVENT:
			//stop keepalive service
			_gn_keepalive_stop();
			_gn_forward_evt(base, id, event_data);
			break;
		case GN_KEEPALIVE_START_EVENT:
			ESP_LOGD(TAG, "keepalive fired!");
			break;

		default:
			break;
		}

		xSemaphoreGive(_gn_xEvtSemaphore);
	}
}

void _gn_evt_reset_start_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

}

esp_err_t _gn_register_event_handlers(gn_config_handle_t conf) {

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, _gn_evt_handler, conf, NULL));

	return ESP_OK;

}

esp_err_t _gn_init_timer(gn_config_handle_t conf) {

	timer_config_t config = { .divider = TIMER_DIVIDER, .counter_dir =
			TIMER_COUNT_UP, .counter_en = TIMER_PAUSE, .alarm_en =
			TIMER_ALARM_EN, .auto_reload = 1, }; // default clock source is APB
	timer_init(TIMER_GROUP_0, TIMER_0, &config);
	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 60 * TIMER_SCALE); //TODO timer configurable from kconfig
	timer_enable_intr(TIMER_GROUP_0, TIMER_0);
	return timer_isr_callback_add(TIMER_GROUP_0, TIMER_0,
			_gn_timer_callback_isr,
			NULL, 0);

}

gn_config_handle_t gn_init() { //TODO make the node working even without network

	esp_err_t ret = ESP_OK;

	if (initialized)
		return _gn_default_conf;

	_gn_xEvtSemaphore = xSemaphoreCreateMutex();

	_gn_default_conf = _gn_create_config();
	_gn_default_conf->status = GN_CONFIG_STATUS_INITIALIZING;

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
	ESP_GOTO_ON_ERROR(_gn_register_event_handlers(_gn_default_conf), err, TAG,
			"error _gn_register_event_handlers: %s", esp_err_to_name(ret));

	//heartbeat to check network comm and send periodical system watchdog to the network
	ESP_GOTO_ON_ERROR(_gn_init_timer(_gn_default_conf), err, TAG,
			"error on timer init: %s", esp_err_to_name(ret));

	//init display
	ESP_GOTO_ON_ERROR(gn_init_display(_gn_default_conf), err, TAG,
			"error on display init: %s", esp_err_to_name(ret));

	//init wifi
	ESP_GOTO_ON_ERROR(_gn_init_wifi(_gn_default_conf), err_net, TAG,
			"error on display init: %s", esp_err_to_name(ret));

	//init time sync. note: if bad, continue
	ESP_GOTO_ON_ERROR(_gn_init_time_sync(_gn_default_conf), err_timesync, TAG,
			"error on time sync init: %s", esp_err_to_name(ret));

	err_timesync:
	//init mqtt system
	ESP_GOTO_ON_ERROR(_gn_mqtt_init(_gn_default_conf), err_srv, TAG,
			"error on server init: %s", esp_err_to_name(ret));


	_gn_default_conf->status = GN_CONFIG_STATUS_OK;
	initialized = true;
	return _gn_default_conf;

	err: _gn_default_conf->status = GN_CONFIG_STATUS_ERROR;
	return _gn_default_conf;

	err_net: _gn_default_conf->status = GN_CONFIG_STATUS_NETWORK_ERROR;
	return _gn_default_conf;

	err_srv: _gn_default_conf->status = GN_CONFIG_STATUS_SERVER_ERROR;
	return _gn_default_conf;

}

#ifdef __cplusplus
}
#endif //__cplusplus
