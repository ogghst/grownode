/*
 * grownode.c
 *
 *  Created on: 17 apr 2021
 *      Author: muratori.n
 */

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

//TODO switch from LOGI to LOGD
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

#include "driver/timer.h"
#include "driver/gpio.h"

#include "gn_display.h"
#include "gn_commons.h"

#include "gn_event_source.h"

#include "grownode_intl.h"

#include "gn_network.h"
#include "gn_mqtt_protocol.h"

static const char *TAG = "grownode";

esp_event_loop_handle_t gn_event_loop;

gn_config_handle_intl_t _gn_default_conf;

//SemaphoreHandle_t _gn_xEvtSemaphore;

bool initialized = false;

ESP_EVENT_DEFINE_BASE(GN_BASE_EVENT);
ESP_EVENT_DEFINE_BASE(GN_LEAF_EVENT);

esp_err_t _gn_leaf_start(gn_leaf_config_handle_intl_t leaf_config) {

	int ret = ESP_OK;
	ESP_LOGI(TAG, "_gn_start_leaf %s", leaf_config->name);

	if (xTaskCreate((void*) leaf_config->task_cb, leaf_config->name, leaf_config->task_size,
			leaf_config, 1,
			NULL) != pdPASS) {
		ESP_LOGE(TAG, "failed to create lef task for %s", leaf_config->name);
		goto fail;
	}

	//gn_display_leaf_start(leaf_config);

	//notice network of the leaf added
	gn_mqtt_subscribe_leaf(leaf_config);

	return ret;

	fail: return ESP_FAIL;

}

esp_err_t _gn_init_flash(gn_config_handle_t conf) {

	esp_err_t ret = ESP_OK;
	/* Initialize NVS partition */
	ret = nvs_flash_init();

#ifndef CONFIG_GROWNODE_RESET_PROVISIONED
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
#endif
		/* NVS partition was truncated
		 * and needs to be erased */
		ESP_GOTO_ON_ERROR(nvs_flash_erase(), err, TAG,
				"error erasing flash: %s", esp_err_to_name(ret));
		/* Retry nvs_flash_init */
		ESP_GOTO_ON_ERROR(nvs_flash_init(), err, TAG, "error init flash: %s",
				esp_err_to_name(ret));
#ifndef CONFIG_GROWNODE_RESET_PROVISIONED
	}
#endif

	err: return ret;

}

esp_err_t _gn_init_spiffs(gn_config_handle_intl_t conf) {

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

	//if (pdTRUE == xSemaphoreTake(_gn_xEvtSemaphore, portMAX_DELAY)) {

	ESP_LOGD(TAG, "_gn_evt_handler event: %d", id);

	switch (id) {
	case GN_NET_OTA_START:
		gn_firmware_update();
		break;

	case GN_NET_RST_START:
		gn_reset();
		break;

	case GN_NETWORK_CONNECTED_EVENT:

		break;
	case GN_NETWORK_DISCONNECTED_EVENT:

		break;
	case GN_SERVER_CONNECTED_EVENT:
		//start keepalive service
		_gn_keepalive_start();
		break;
	case GN_SERVER_DISCONNECTED_EVENT:
		//stop keepalive service
		_gn_keepalive_stop();
		break;

	case GN_KEEPALIVE_START_EVENT:
		//publish node
		gn_mqtt_send_node_config(_gn_default_conf->node_config);
		break;

	default:
		break;
	}

	//xSemaphoreGive(_gn_xEvtSemaphore);
	//}
}

void _gn_evt_reset_start_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

}

esp_err_t _gn_evt_handlers_register(gn_config_handle_intl_t conf) {

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, _gn_evt_handler, conf, NULL));

	return ESP_OK;

}

esp_err_t _gn_init_keepalive_timer(gn_config_handle_intl_t conf) {

	timer_config_t config = { .divider = TIMER_DIVIDER, .counter_dir =
			TIMER_COUNT_UP, .counter_en = TIMER_PAUSE, .alarm_en =
			TIMER_ALARM_EN, .auto_reload = 1, }; // default clock source is APB
	timer_init(TIMER_GROUP_0, TIMER_0, &config);
	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_0,
	CONFIG_GROWNODE_KEEPALIVE_TIMER_SEC * TIMER_SCALE); //TODO timer configurable from kconfig
	timer_enable_intr(TIMER_GROUP_0, TIMER_0);
	return timer_isr_callback_add(TIMER_GROUP_0, TIMER_0,
			_gn_timer_callback_isr,
			NULL, 0);

}

gn_config_handle_intl_t _gn_config_create() {

	gn_config_handle_intl_t _conf = (gn_config_handle_intl_t) malloc(
			sizeof(struct gn_config_t));
	_conf->status = GN_CONFIG_STATUS_NOT_INITIALIZED;
	return _conf;

}

esp_err_t _gn_init_event_loop(gn_config_handle_intl_t config) {

	esp_err_t ret = ESP_OK;

	//default event loop
	ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), fail, TAG,
			"error creating system event loop: %s", esp_err_to_name(ret));

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5, .task_name =
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

gn_config_status_t gn_get_config_status(gn_config_handle_t config) {

	if (!config)
		return GN_CONFIG_STATUS_ERROR;
	return ((gn_config_handle_intl_t) config)->status;

}

esp_event_loop_handle_t gn_get_config_event_loop(gn_config_handle_t config) {

	if (!config)
		return NULL;
	return ((gn_config_handle_intl_t) config)->event_loop;

}

gn_node_config_handle_t gn_node_create(gn_config_handle_t config,
		const char *name) {

	if (config == NULL
			|| ((gn_config_handle_intl_t) config)->mqtt_client == NULL
			|| name == NULL) {
		ESP_LOGE(TAG, "gn_create_node failed. parameters not correct");
		return NULL;
	}

	gn_node_config_handle_intl_t n_c = _gn_node_config_create();

	strncpy(n_c->name, name, GN_NODE_NAME_SIZE);
	//n_c->event_loop = config->event_loop;
	n_c->config = config;

	//create leaves
	gn_leaves_list leaves = { .size = 5, .last = 0 };

	n_c->leaves = leaves;
	((gn_config_handle_intl_t) config)->node_config = n_c;

	return n_c;
}

size_t gn_node_get_size(gn_node_config_handle_t node_config) {

	if (node_config == NULL)
		return -1;

	return ((gn_node_config_handle_intl_t) node_config)->leaves.last;
}

esp_err_t gn_node_destroy(gn_node_config_handle_t node) {

	//free(((gn_node_config_handle_intl_t) node)->leaves->at); //TODO implement free of leaves
	free((gn_node_config_handle_intl_t) node);

	return ESP_OK;
}

esp_err_t gn_node_start(gn_node_config_handle_t node) {

	esp_err_t ret = ESP_OK;
	ESP_LOGD(TAG, "gn_start_node: %s",
			((gn_node_config_handle_intl_t )node)->name);

	//publish node
	if (gn_mqtt_send_node_config(node) != ESP_OK)
		goto fail;

	//run leaves
	for (int i = 0; i < ((gn_node_config_handle_intl_t) node)->leaves.last;
			i++) {
		ESP_LOGD(TAG, "starting leaf: %d", i);
		_gn_leaf_start(((gn_node_config_handle_intl_t) node)->leaves.at[i]);
	}

	return ret;
	fail: return ESP_FAIL;

}

gn_leaf_config_handle_intl_t _gn_leaf_config_create() {

	gn_leaf_config_handle_intl_t _conf = (gn_leaf_config_handle_intl_t) malloc(
			sizeof(struct gn_leaf_config_t));
	//_conf->callback = NULL;
	strcpy(_conf->name, "");
	_conf->node_config = NULL;
	_conf->task_cb = NULL;
	_conf->params = NULL;
	return _conf;

}

char* gn_get_node_config_name(gn_node_config_handle_t node_config) {

	if (!node_config)
		return NULL;
	return ((gn_node_config_handle_intl_t) node_config)->name;

}

gn_leaf_config_handle_t gn_leaf_create(gn_node_config_handle_t node_config,
		const char *name, gn_leaf_task_callback task,
		gn_leaf_display_config_callback display_config,
		size_t task_size) { //, gn_leaf_display_task_t display_task) {

	gn_node_config_handle_intl_t node_cfg =
			(gn_node_config_handle_intl_t) node_config;
	gn_config_handle_intl_t cfg = (gn_config_handle_intl_t) node_cfg->config;

	if (node_cfg == NULL || node_cfg->config == NULL || name == NULL
			|| task == NULL || cfg->mqtt_client == NULL) {
		ESP_LOGE(TAG, "gn_create_leaf failed. parameters not correct");
		return NULL;
	}

	gn_leaf_config_handle_intl_t l_c = _gn_leaf_config_create();

	strncpy(l_c->name, name, GN_LEAF_NAME_SIZE);
	l_c->node_config = node_cfg;
	l_c->task_cb = task;
	l_c->task_size = task_size;
	l_c->display_config_cb = display_config;
	//l_c->display_task = display_task;
	/*
	 l_c->xLeafTaskEventQueue = xQueueCreate(10, sizeof(gn_event_t));
	 if (l_c->xLeafTaskEventQueue == NULL) {
	 return NULL;
	 }
	 */
	l_c->event_loop = gn_event_loop;

	gn_node_config_handle_intl_t n_c = l_c->node_config;

	//TODO add leaf to node. implement dynamic array
	if (n_c->leaves.last >= n_c->leaves.size - 1) {
		ESP_LOGE(TAG,
				"gn_create_leaf failed. not possible to add more than 5 leaves to a node");
		return NULL;
	}

	n_c->leaves.at[n_c->leaves.last] = l_c;
	n_c->leaves.last++;

	return l_c;

}

esp_err_t gn_leaf_destroy(gn_leaf_config_handle_t leaf) {

	//vQueueDelete(leaf->xLeafTaskEventQueue);;
	free(leaf);
	return ESP_OK;

}

char* gn_get_leaf_config_name(gn_leaf_config_handle_t leaf_config) {

	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->name;

}

esp_event_loop_handle_t gn_get_leaf_config_event_loop(
		gn_leaf_config_handle_t leaf_config) {

	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->event_loop;

}

gn_leaf_param_handle_t gn_leaf_param_create(const char *name,
		const gn_val_type_t type, const gn_val_t val) {

	if (!name) {
		ESP_LOGE(TAG, "gn_leaf_param_create incorrect parameters");
		return NULL;
	}

	gn_leaf_param_handle_t _ret = (gn_leaf_param_handle_t) malloc(
			sizeof(gn_leaf_param_t));
	_ret->next = NULL;

	char *_name = strdup(name);
	//(char*) malloc(sizeof(char)*strlen(name));
	//strncpy(_name, name, strlen(name));
	_ret->name = _name;

	gn_param_val_handle_t _param_val = (gn_param_val_handle_t) malloc(
			sizeof(gn_param_val_t));
	gn_val_t _val;

	switch (type) {
	case GN_VAL_TYPE_STRING:
		if (!val.s) {
			ESP_LOGE(TAG, "gn_leaf_param_create incorrect string parameter");
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
		ESP_LOGE(TAG, "param type not handled");
		return NULL;
		break;
	}

	memcpy(&_param_val->t, &type, sizeof(type));
	memcpy(&_param_val->v, &_val, sizeof(_val));

	_ret->param_val = _param_val;

	return _ret;

}
/*
 esp_err_t gn_leaf_param_set(const gn_leaf_config_handle_t leaf,
 const char *name, const gn_val_t *val) {

 if (!leaf || !name || !val)
 return ESP_ERR_INVALID_ARG;

 gn_param_handle_t _param = gn_leaf_param_get(leaf, name);

 switch (_param->param_val->t) {

 case GN_VAL_TYPE_STRING:
 ESP_LOGD(TAG, "gn_leaf_param_set %s %s", name, val->s);
 ESP_LOGD(TAG, "	old value %s", val->s);
 _param->param_val->v.s = (char*) realloc(_param->param_val->v.s,
 sizeof(char) * (strlen(val->s) + 1));
 memset(_param->param_val->v.s, 0, sizeof(char) * (strlen(val->s) + 1));
 strncpy(_param->param_val->v.s, val->s, strlen(val->s));
 ESP_LOGD(TAG, "gn_leaf_param_set - result %s", _param->param_val->v.s);
 break;
 case GN_VAL_TYPE_BOOLEAN:
 ESP_LOGD(TAG, "gn_leaf_param_set %s %d", name, val->b);
 ESP_LOGD(TAG, "	old value %d", val->b);
 _param->param_val->v.b = val->b;
 ESP_LOGD(TAG, "gn_leaf_param_set - result %d", _param->param_val->v.b);
 break;
 default:
 ESP_LOGE(TAG, "param type not supported");
 break;

 }

 return gn_mqtt_send_node_config(leaf->node_config);

 }
 */

esp_err_t gn_leaf_param_set_string(const gn_leaf_config_handle_t leaf,
		const char *name, const char *val) {

	if (!leaf || !name || !val)
		return ESP_ERR_INVALID_ARG;

	gn_leaf_param_handle_t _param = gn_leaf_param_get(leaf, name);
	if (!_param)
		return ESP_ERR_INVALID_ARG;

	ESP_LOGD(TAG, "gn_leaf_param_set_string - param:%s value:%s", name, val);
	ESP_LOGD(TAG, "	old value %s", _param->param_val->v.s);
	_param->param_val->v.s = (char*) realloc(_param->param_val->v.s,
			sizeof(char) * (strlen(val) + 1));
	memset(_param->param_val->v.s, 0, sizeof(char) * (strlen(val) + 1));
	strncpy(_param->param_val->v.s, val, strlen(val));
	ESP_LOGD(TAG, "gn_leaf_param_set - result %s", _param->param_val->v.s);

	return gn_mqtt_send_leaf_param(_param);

}

esp_err_t gn_leaf_param_set_bool(const gn_leaf_config_handle_t leaf,
		const char *name, const bool val) {

	if (!leaf || !name)
		return ESP_ERR_INVALID_ARG;

	gn_leaf_param_handle_t _param = gn_leaf_param_get(leaf, name);
	if (!_param)
		return ESP_ERR_INVALID_ARG;

	ESP_LOGD(TAG, "gn_leaf_param_set_bool %s %d", name, val);
	ESP_LOGD(TAG, "	old value %d", _param->param_val->v.b);
	_param->param_val->v.b = val;
	ESP_LOGD(TAG, "gn_leaf_param_set - result %d", _param->param_val->v.b);

	return gn_mqtt_send_leaf_param(_param);

}

esp_err_t gn_leaf_param_set_double(const gn_leaf_config_handle_t leaf,
		const char *name, const double val) {

	if (!leaf || !name)
		return ESP_ERR_INVALID_ARG;

	gn_leaf_param_handle_t _param = gn_leaf_param_get(leaf, name);
	if (!_param)
		return ESP_ERR_INVALID_ARG;

	ESP_LOGD(TAG, "gn_leaf_param_set_double %s %g", name, val);
	ESP_LOGD(TAG, "	old value %g", _param->param_val->v.d);

	_param->param_val->v.d = val;

	ESP_LOGD(TAG, "gn_leaf_param_set - result %g", _param->param_val->v.d);

	return gn_mqtt_send_leaf_param(_param);

}

esp_err_t gn_leaf_parameter_update(gn_leaf_config_handle_t leaf_config,
		char *param, char *data, int data_len) {

	if (!leaf_config || !param || !data || data_len == 0)
		return 1;

	ESP_LOGD(TAG, "gn_leaf_parameter_update. param=%s data=%.*s data_len %d", param, data_len, data, data_len);

	gn_leaf_param_handle_t leaf_params = gn_get_leaf_config_params(leaf_config);

	while (leaf_params != NULL) {

		//check param name
		if (strcmp(param, leaf_params->name) == 0) {
			//param is the one to update

			//check type
			switch (leaf_params->param_val->t) {

			case GN_VAL_TYPE_STRING: {

				char* pvs = leaf_params->param_val->v.s;

				pvs = realloc(pvs, data_len+1);
				strncpy(pvs, data, data_len);
				//end with terminating character to be handled as string if not yet
				pvs[data_len]='\0';

			}
				break;

			case GN_VAL_TYPE_DOUBLE: {

				double val = strtod(data, NULL);
				gn_leaf_param_set_double(leaf_config, param, val);

			}
				break;

			case GN_VAL_TYPE_BOOLEAN: {

				if (strncmp(data, "1", data_len) == 0) {
					//setting to true
					gn_leaf_param_set_bool(leaf_config, param,
					true);
				} else {
					//setting to false
					gn_leaf_param_set_bool(leaf_config, param,
					false);
				}

			}
				break;

			default:
				break;
			};

		}

		leaf_params = leaf_params->next;
	}

	return ESP_OK;

}

esp_err_t gn_leaf_param_destroy(gn_leaf_param_handle_t new_param) {

	if (!new_param)
		return ESP_ERR_INVALID_ARG;

	free(new_param->param_val->v.s);
	free(new_param->param_val);
	free(new_param->name);
	free(new_param);

	return ESP_OK;

}

esp_err_t gn_leaf_param_add(const gn_leaf_config_handle_t leaf,
		const gn_leaf_param_handle_t new_param) {

	if (!leaf || !new_param) {
		ESP_LOGE(TAG, "gn_leaf_param_add incorrect parameters");
		return ESP_ERR_INVALID_ARG;
	}

	gn_leaf_param_handle_t _param =
			((gn_leaf_config_handle_intl_t) leaf)->params;

	while (_param) {
		if (strcmp(_param->name, new_param->name) == 0) {
			ESP_LOGE(TAG, "Parameter with name %s already exists in Leaf %s",
					new_param->name,
					((gn_leaf_config_handle_intl_t )leaf)->name);
			return ESP_ERR_INVALID_ARG;
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

	if (gn_mqtt_subscribe_leaf_param(new_param) != ESP_OK) {
		ESP_LOGE(TAG, "gn_leaf_param_add failed to add param %s to leaf %s",
				new_param->name, ((gn_leaf_config_handle_intl_t )leaf)->name);
		return ESP_FAIL;
	}

	ESP_LOGD(TAG, "Param %s added in %s", new_param->name,
			((gn_leaf_config_handle_intl_t )leaf)->name);
	return ESP_OK;

}

gn_leaf_param_handle_t gn_get_leaf_config_params(
		gn_leaf_config_handle_t leaf_config) {

	if (!leaf_config)
		return NULL;
	return ((gn_leaf_config_handle_intl_t) leaf_config)->params;

}

gn_leaf_param_handle_t gn_leaf_param_get(const gn_leaf_config_handle_t leaf,
		const char *param_name) {

	if (!leaf || !param_name) {
		ESP_LOGE(TAG, "gn_leaf_param_get incorrect parameters");
		return NULL;
	}
	gn_leaf_param_handle_t param = ((gn_leaf_config_handle_intl_t) leaf)->params;
	while (param) {
		if (strcmp(param->name, param_name) == 0) {
			break;
		}
		param = param->next;
	}
	return param;

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

esp_err_t gn_message_display(char *message) {

	esp_err_t ret = ESP_OK;

	//void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
	ESP_LOGI(TAG, "%s", message);

	//char *ptr = malloc(sizeof(char) * strlen(message) + 1);

	ret = esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_DISPLAY_LOG_EVENT,
			message, strlen(message) + 1, portMAX_DELAY);

	//free(ptr);
	return ret;

}

esp_err_t gn_message_send_text(gn_leaf_config_handle_t leaf, const char *msg) {	//TODO remove leaf config

	return gn_mqtt_send_leaf_message(leaf, msg);

}

esp_err_t gn_event_send_internal(gn_config_handle_t conf,
		gn_leaf_event_handle_t event) {

	gn_config_handle_intl_t _conf = (gn_config_handle_intl_t) conf;

	esp_err_t ret = ESP_OK;

	ret = esp_event_post_to(_conf->event_loop, GN_BASE_EVENT, event->id, event,
			sizeof(event),
			portMAX_DELAY);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to send internal event");
	}
	return ret;

}

esp_err_t gn_firmware_update() {

#if CONFIG_GROWNODE_WIFI_ENABLED
	xTaskCreate(_gn_ota_task, "gn_ota_task", 8196, NULL, 10, NULL);
#endif
	return ESP_OK;

}

esp_err_t gn_reset() {

	nvs_flash_erase();
	esp_restart();
	return ESP_OK;

}

gn_config_handle_t gn_init() { //TODO make the node working even without network

	esp_err_t ret = ESP_OK;

	if (initialized)
		return _gn_default_conf;

	//_gn_xEvtSemaphore = xSemaphoreCreateMutex();

	_gn_default_conf = _gn_config_create();
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
	ESP_GOTO_ON_ERROR(_gn_evt_handlers_register(_gn_default_conf), err, TAG,
			"error _gn_register_event_handlers: %s", esp_err_to_name(ret));

	//heartbeat to check network comm and send periodical system watchdog to the network
	ESP_GOTO_ON_ERROR(_gn_init_keepalive_timer(_gn_default_conf), err, TAG,
			"error on timer init: %s", esp_err_to_name(ret));

	//init display
	ESP_GOTO_ON_ERROR(gn_init_display(_gn_default_conf), err, TAG,
			"error on display init: %s", esp_err_to_name(ret));

#if CONFIG_GROWNODE_WIFI_ENABLED
	//init wifi
	ESP_GOTO_ON_ERROR(_gn_init_wifi(_gn_default_conf), err_net, TAG,
			"error on display init: %s", esp_err_to_name(ret));

	//init time sync. note: if bad, continue
	ESP_GOTO_ON_ERROR(_gn_init_time_sync(_gn_default_conf), err_timesync, TAG,
			"error on time sync init: %s", esp_err_to_name(ret));

	err_timesync:
	//init mqtt system
	ESP_GOTO_ON_ERROR(gn_mqtt_init(_gn_default_conf), err_srv, TAG,
			"error on server init: %s", esp_err_to_name(ret));

#endif

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
