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

#ifndef GROWNODE_H_
#define GROWNODE_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include <stddef.h>

//#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"

#include "gn_commons.h"
#include "gn_event_source.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#else
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#define GN_NODE_LEAVES_MAX_SIZE 64

//functions
gn_node_config_handle_t gn_node_create(gn_config_handle_t config,
		const char *name);

gn_err_t gn_node_destroy(gn_node_config_handle_t node);

gn_err_t gn_node_start(gn_node_config_handle_t node);

size_t gn_node_get_size(gn_node_config_handle_t config);

gn_config_handle_t gn_init(gn_config_init_param_t *config_init);

gn_err_t gn_firmware_update();

gn_err_t gn_reset();

gn_err_t gn_reboot();

gn_leaf_config_handle_t gn_leaf_create(gn_node_config_handle_t node_config,
		const char *name, gn_leaf_config_callback leaf_config,
		size_t task_size);

gn_leaf_descriptor_handle_t gn_leaf_get_descriptor(
		gn_leaf_config_handle_t leaf_config);

//esp_err_t _gn_start_leaf(gn_leaf_config_handle_t leaf);

QueueHandle_t gn_leaf_get_event_queue(gn_leaf_config_handle_t leaf_config);

gn_err_t gn_leaf_event_subscribe(gn_leaf_config_handle_t leaf_config,
		gn_event_id_t event_id);

gn_err_t gn_leaf_event_unsubscribe(gn_leaf_config_handle_t leaf_config,
		gn_event_id_t event_id);

typedef gn_leaf_param_validator_result_t (*gn_validator_t)(
		gn_leaf_param_handle_t param, void **value);

gn_leaf_param_handle_t gn_leaf_param_create(gn_leaf_config_handle_t leaf_config,
		const char *name, const gn_val_type_t type, const gn_val_t val,
		gn_leaf_param_visibility_t access, gn_leaf_param_storage_t storage,
		gn_validator_t validator);

gn_err_t gn_leaf_param_add_to_leaf(const gn_leaf_config_handle_t leaf,
		const gn_leaf_param_handle_t new_param);

gn_leaf_param_handle_t gn_leaf_param_get_param_handle(
		const gn_leaf_config_handle_t leaf, const char *param_name);

gn_err_t gn_leaf_param_get_value(const gn_leaf_param_handle_t param, void *val);

gn_err_t gn_leaf_param_get_string(const gn_leaf_config_handle_t leaf_config,
		const char *name, char *val, size_t max_lenght);

gn_err_t gn_leaf_param_set_string(const gn_leaf_config_handle_t leaf,
		const char *name, char *val);

gn_err_t gn_leaf_param_init_string(const gn_leaf_config_handle_t leaf,
		const char *name, const char *val);

gn_err_t gn_leaf_param_get_bool(const gn_leaf_config_handle_t leaf_config,
		const char *name, bool *val);

gn_err_t gn_leaf_param_set_bool(const gn_leaf_config_handle_t leaf,
		const char *name, bool val);

gn_err_t gn_leaf_param_init_bool(const gn_leaf_config_handle_t leaf,
		const char *name, bool val);

gn_err_t gn_leaf_param_get_double(const gn_leaf_config_handle_t leaf_config,
		const char *name, double *val);

gn_err_t gn_leaf_param_set_double(const gn_leaf_config_handle_t leaf,
		const char *name, double val);

gn_err_t gn_leaf_param_init_double(const gn_leaf_config_handle_t leaf,
		const char *name, double val);

gn_err_t gn_leaf_param_set_value(const gn_leaf_config_handle_t leaf,
		const void *val);

//gn_err_t gn_leaf_param_destroy(gn_leaf_param_handle_t new_param);

//void* gn_leaf_context_add_to_leaf(const gn_leaf_config_handle_t leaf, char *key,
//		void *value);

//void* gn_leaf_context_get_key_to_leaf(const gn_leaf_config_handle_t leaf,
//		char *key);

//gn_err_t gn_display_log(char *message);

gn_err_t gn_log(char *log_tag, gn_log_level_t level, const char *message, ...);

//esp_err_t gn_message_send_text(gn_leaf_config_handle_t config, const char *msg);

//esp_err_t gn_event_send_internal(gn_config_handle_t conf,
//		gn_leaf_event_handle_t event);


typedef struct gn_config_init_param_t *gn_config_init_param_handle_t;

gn_config_status_t gn_get_status(gn_config_handle_t config);

const char* gn_get_status_description(gn_config_handle_t config);

esp_event_loop_handle_t gn_get_event_loop(gn_config_handle_t config);

esp_event_loop_handle_t gn_leaf_get_event_loop(
		gn_config_handle_t config);

gn_err_t gn_node_get_name(gn_node_config_handle_t node_config, char* name);

gn_err_t gn_leaf_get_name(gn_leaf_config_handle_t leaf_config, char* name);

gn_leaf_config_handle_t gn_leaf_get_config_handle(gn_node_config_handle_t node,
		const char *name);

gn_node_config_handle_t gn_leaf_get_node(
		gn_leaf_config_handle_t leaf_config);

gn_err_t gn_send_leaf_param_change_message(const char *leaf_name,
		const char *param_name, const void *message, size_t message_len);

gn_err_t gn_send_node_leaf_param_status(
		const gn_node_config_handle_t _node_config);

gn_err_t gn_leaf_param_update_bool(const gn_leaf_config_handle_t leaf_config,
		const char *name, bool val);

gn_err_t gn_leaf_param_update_double(const gn_leaf_config_handle_t leaf_config,
		const char *name, double val);

gn_err_t gn_leaf_param_update_string(const gn_leaf_config_handle_t leaf_config,
		const char *name, char *val);

//esp_event_loop_handle_t gn_get_leaf_config_event_loop(
//		gn_leaf_config_handle_t leaf_config);

//gn_leaf_param_handle_t gn_get_leaf_config_params(
//		gn_leaf_config_handle_t leaf_config);

extern gn_display_container_t gn_display_setup_leaf(
		gn_leaf_config_handle_t leaf_config);

extern BaseType_t gn_display_leaf_refresh_start();

extern BaseType_t gn_display_leaf_refresh_end();

//extern void gn_display_leaf_start(gn_leaf_config_handle_t leaf_config);

extern gn_server_status_t gn_mqtt_get_status();

gn_err_t gn_storage_set(const char *key, const void *value,
		size_t required_size);

gn_err_t gn_storage_get(const char *key, void **value);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
