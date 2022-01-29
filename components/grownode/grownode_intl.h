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

#ifndef COMPONENTS_GROWNODE_SROWNODE_INTL_H_
#define COMPONENTS_GROWNODE_SROWNODE_INTL_H_

#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"
#include "esp_event_base.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "grownode.h"
#include "gn_leaf_context.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

typedef struct gn_node_config_t *gn_node_config_handle_intl_t;

typedef struct gn_leaf_config_t *gn_leaf_config_handle_intl_t;

typedef struct gn_config_t *gn_config_handle_intl_t;

typedef struct {
	size_t size;
	size_t last;
	gn_leaf_config_handle_intl_t at[64];
} gn_leaves_list;

typedef struct {
	size_t size;
	size_t last;
	gn_node_config_handle_intl_t *at;
} gn_nodes_list;

struct gn_config_t {
	esp_vfs_spiffs_conf_t spiffs_conf;
	esp_mqtt_client_handle_t mqtt_client;
	esp_event_loop_handle_t event_loop;
	wifi_init_config_t wifi_config;
	wifi_prov_mgr_config_t prov_config;
	char deviceName[17];
	uint8_t macAddress[6];
	gn_node_status_t status;
	gn_node_config_handle_intl_t node_config;
	gn_config_init_param_t* config_init_params;

};

struct gn_node_config_t {
	char name[GN_NODE_NAME_SIZE];
	//esp_event_loop_handle_t event_loop;
	gn_config_handle_intl_t config;
	gn_leaves_list leaves;
};

struct gn_leaf_config_t {
	gn_leaf_descriptor_handle_t leaf_descriptor;
	char name[GN_LEAF_NAME_SIZE];
	size_t task_size;
	gn_node_config_handle_intl_t node_config;
	//gn_leaf_display_task_t display_task;
	//gn_leaf_config_handle_t next;
	//gn_leaf_task_callback task_cb;
	QueueHandle_t event_queue;
	//esp_event_loop_handle_t event_loop;
	gn_leaf_param_handle_t params;
	//gn_display_handler_t display_handler;
	gn_leaf_context_handle_t leaf_context;
	gn_display_container_t display_container;
};

typedef struct {
	gn_val_type_t t;
	gn_val_t v;
} gn_param_val_t;

typedef gn_param_val_t *gn_param_val_handle_t;

typedef gn_param_val_t *gn_param_val_handle_int_t;

struct gn_leaf_param {
	char *name;
	gn_leaf_param_access_type_t access;
	gn_leaf_param_storage_t storage;
	gn_param_val_handle_t param_val;
	gn_leaf_handle_t leaf_config;
	gn_validator_callback_t validator;
	struct gn_leaf_param *next;
};

typedef struct gn_leaf_param gn_leaf_param_t;

typedef gn_leaf_param_t *gn_leaf_param_handle_intl_t;


//typedef struct gn_leaf_param gn_leaf_param_t;

//typedef gn_leaf_param_t *gn_leaf_param_handle_t;

/*
 typedef struct {
 char *name;
 esp_event_loop_handle_t event_loop;
 gn_config_handle_t config;
 gn_leaf_config_handle_t leaf;
 } gn_node_config_t;


 typedef gn_node_config_t *gn_node_config_handle_t;


 typedef struct {
 char *name;
 gn_node_config_handle_t node_config;
 gn_event_callback_t callback;
 gn_leaf_config_handle_t next;
 } gn_leaf_config_t;

 typedef gn_leaf_config_t *gn_leaf_config_handle_t;
 */

gn_err_t _gn_send_event_to_leaf(gn_leaf_config_handle_intl_t leaf_config,
		gn_leaf_parameter_event_handle_t evt);

gn_err_t _gn_leaf_parameter_update(const gn_leaf_handle_t leaf_config,
		const char *param, const void *data, const int data_len);

#endif /* COMPONENTS_GROWNODE_SROWNODE_INTL_H_ */
