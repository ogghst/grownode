#ifndef GROWNODE_H_
#define GROWNODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"

#include "gn_commons.h"
#include "gn_event_source.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

//functions
gn_node_config_handle_t gn_node_create(gn_config_handle_t config,
		const char *name);

esp_err_t gn_node_destroy(gn_node_config_handle_t node);

esp_err_t gn_node_start(gn_node_config_handle_t node);

size_t gn_node_get_size(gn_node_config_handle_t config);

gn_config_handle_t gn_init();

esp_err_t gn_firmware_update();

esp_err_t gn_reset();

gn_leaf_config_handle_t gn_leaf_create(gn_node_config_handle_t node_config,
		const char *name, gn_leaf_task_callback task,
		size_t task_size); //, gn_leaf_display_task_t display_task);

esp_err_t gn_leaf_destroy(gn_leaf_config_handle_t leaf);

//esp_err_t _gn_start_leaf(gn_leaf_config_handle_t leaf);

QueueHandle_t gn_leaf_get_event_queue (gn_leaf_config_handle_t leaf_config);

gn_leaf_param_handle_t gn_leaf_param_create(const char *name,
		const gn_val_type_t type, const gn_val_t val);

esp_err_t gn_leaf_param_add(const gn_leaf_config_handle_t leaf,
		const gn_leaf_param_handle_t new_param);

gn_leaf_param_handle_t gn_leaf_param_get(const gn_leaf_config_handle_t leaf,
		const char *param_name);

esp_err_t gn_leaf_param_set_string(const gn_leaf_config_handle_t leaf,
		const char *name, const char *val);

esp_err_t gn_leaf_param_set_bool(const gn_leaf_config_handle_t leaf,
		const char *name, const bool val);

esp_err_t gn_leaf_param_set_double(const gn_leaf_config_handle_t leaf,
		const char *name, const double val);

esp_err_t gn_leaf_parameter_update(gn_leaf_config_handle_t leaf_config,
		char *param, char *data, int data_len);

esp_err_t gn_leaf_param_destroy(gn_leaf_param_handle_t new_param);

void* gn_leaf_context_add_to_leaf(const gn_leaf_config_handle_t leaf, char *key, void *value);

void* gn_leaf_context_get_key_to_leaf(const gn_leaf_config_handle_t leaf, char *key);

esp_err_t gn_message_display(char *message);

esp_err_t gn_message_send_text(gn_leaf_config_handle_t config, const char *msg);

esp_err_t gn_event_send_internal(gn_config_handle_t conf, gn_leaf_event_handle_t event);

gn_config_status_t gn_get_config_status(gn_config_handle_t config);

esp_event_loop_handle_t gn_get_config_event_loop(gn_config_handle_t config);

char* gn_get_node_config_name(gn_node_config_handle_t node_config);

char* gn_get_leaf_config_name(gn_leaf_config_handle_t leaf_config);

esp_event_loop_handle_t gn_get_leaf_config_event_loop(
		gn_leaf_config_handle_t leaf_config);

gn_leaf_param_handle_t gn_get_leaf_config_params(
		gn_leaf_config_handle_t leaf_config);

extern gn_display_container_t gn_display_setup_leaf_display(gn_leaf_config_handle_t leaf_config);

extern BaseType_t gn_display_leaf_refresh_start();

extern BaseType_t gn_display_leaf_refresh_end();

extern void gn_display_leaf_start(gn_leaf_config_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
