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

bool initialized = false;

ESP_EVENT_DEFINE_BASE(GN_BASE_EVENT);
ESP_EVENT_DEFINE_BASE(GN_LEAF_EVENT);

gn_config_handle_t _gn_create_config() {
	gn_config_handle_t _conf = (gn_config_handle_t) malloc(sizeof(gn_config_t));
	_conf->status = GN_CONFIG_STATUS_NOT_INITIALIZED;
	_conf->event_loop = NULL;
	_conf->mqtt_client = NULL;
	strncpy(_conf->deviceName, "anonymous", 10);
	//_conf->prov_config = NULL;
	//_conf->spiffs_conf = NULL;
	//_conf->wifi_config = NULL;
	return _conf;
}

esp_err_t _gn_init_event_loop(gn_config_handle_t conf) {

	esp_err_t ret = ESP_OK;

	//default event loop
	ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), fail, TAG,
			"error creating system event loop: %s", esp_err_to_name(ret));

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5, .task_name =
			"loop_task", // task will be created
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
	gn_leaves_list
	leaves = {.size = 5, .last = 0, .at =
		(gn_leaf_config_handle_t*) malloc(5 * sizeof(gn_leaf_config_t))};

	n_c->leaves = leaves;

	return n_c;
}

esp_err_t gn_destroy_node(gn_node_config_handle_t node) {

	free(node->leaves.at);
	free(node->name);
	free(node);

	return ESP_OK;
}

esp_err_t gn_publish_node(gn_node_config_handle_t node) {

	return _gn_mqtt_send_node_config(node);

}

gn_leaf_config_handle_t _gn_create_leaf_config() {
	gn_leaf_config_handle_t _conf = (gn_leaf_config_handle_t) malloc(
			sizeof(gn_leaf_config_t));
	_conf->callback = NULL;
	_conf->name = NULL;
	_conf->node_config = NULL;
	return _conf;
}

gn_leaf_config_handle_t gn_create_leaf(gn_node_config_handle_t node_cfg,
		const char *name, gn_event_callback_t callback) {

	if (node_cfg == NULL || node_cfg->config == NULL
			|| node_cfg->config->mqtt_client == NULL || name == NULL
			|| callback == NULL) {
		ESP_LOGE(TAG, "gn_create_leaf failed. parameters not correct");
		return NULL;
	}

	gn_leaf_config_handle_t l_c = _gn_create_leaf_config();

	l_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(l_c->name, name, GN_MEM_NAME_SIZE);
	l_c->node_config = node_cfg;
	l_c->callback = callback;


	//TODO add leaf to node. implement dynamic array
	if (l_c->node_config->leaves.last == l_c->node_config->leaves.size-1) {
		ESP_LOGE(TAG, "gn_create_leaf failed. not possible to add more than 5 leaves to a node");
		return NULL;
	}

	l_c->node_config->leaves.at[l_c->node_config->leaves.last] = l_c;
	l_c->node_config->leaves.last++;

	return l_c;
}

esp_err_t gn_destroy_leaf(gn_leaf_config_handle_t leaf) {

	free(leaf->name);
	free(leaf);

	return ESP_OK;

}

esp_err_t gn_init_leaf(gn_leaf_config_handle_t leaf) {

	int ret = ESP_OK;

	//send callback init request
	leaf->callback(GN_LEAF_INIT_REQUEST_EVENT, leaf);

	//notice network of the leaf added
	_gn_mqtt_subscribe_leaf(leaf);
	return ret;

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
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	return ret;

}

void gn_log_message(const char *message) {

	//void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
	ESP_LOGI(TAG, "gn_log_message '%s' (%i)", message, strlen(message));

	//char *ptr = malloc(sizeof(char) * strlen(message) + 1);

	ESP_ERROR_CHECK(
			esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_DISPLAY_LOG_EVENT, message, strlen(message) + 1, portMAX_DELAY));

	//free(ptr);
}

void _gn_update_firmware() {
	xTaskCreate(_gn_ota_task, "_gn_ota_task", 8196, NULL, 10, NULL);
}

void _gn_evt_ota_start_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	_gn_update_firmware();

}

void _gn_evt_reset_start_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	gn_log_message("resetting flash");
	nvs_flash_erase();
	gn_log_message("reboot in 3 sec");

	vTaskDelay(3000 / portTICK_PERIOD_MS);

	esp_restart();

}

esp_err_t _gn_register_event_handlers(gn_config_handle_t conf) {

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_NET_OTA_START, _gn_evt_ota_start_handler, NULL, NULL));

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_NET_RST_START, _gn_evt_reset_start_handler, NULL, NULL));

	return ESP_OK;

}

gn_config_handle_t gn_init() {

	esp_err_t ret = ESP_OK;

	if (initialized)
		return _gn_default_conf;

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

	//init display
	ESP_GOTO_ON_ERROR(gn_init_display(_gn_default_conf), err, TAG,
			"error on display init: %s", esp_err_to_name(ret));

	//init wifi
	ESP_GOTO_ON_ERROR(_gn_init_wifi(_gn_default_conf), err_net, TAG,
			"error on display init: %s", esp_err_to_name(ret));

	//init time sync. note: if bad, continue
	ESP_GOTO_ON_ERROR(_gn_init_time_sync(_gn_default_conf), err_timesync, TAG,
			"error on time sync init: %s", esp_err_to_name(ret));

	//TODO implement heartbeat to check network comm and send periodical system watchdog to the network

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
