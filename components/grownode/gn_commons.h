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

#ifndef MAIN_GN_COMMONS_H_
#define MAIN_GN_COMMONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GN_NODE_NAME_SIZE 32
#define GN_LEAF_NAME_SIZE 32
#define GN_LEAF_PARAM_NAME_SIZE 32
#define GN_LEAF_DATA_SIZE 512
#define GN_NODE_DATA_SIZE 512

#include "esp_system.h"
#include "esp_event.h"
#include "gn_event_source.h"

typedef enum {
	GN_CONFIG_STATUS_NOT_INITIALIZED,
	GN_CONFIG_STATUS_INITIALIZING,
	GN_CONFIG_STATUS_ERROR,
	GN_CONFIG_STATUS_NETWORK_ERROR,
	GN_CONFIG_STATUS_SERVER_ERROR,
	GN_CONFIG_STATUS_COMPLETED,
	GN_CONFIG_STATUS_STARTED
} gn_config_status_t;

typedef enum {
	GN_SERVER_CONNECTED,
	GN_SERVER_DISCONNECTED,
} gn_server_status_t;

/**
 * @brief error codes that the grownode functions can return.
 *
 * The GN_RET_OK and GN_RET_ERR are mapped like ESP_OK and ESP_FAIL for compatibility across platforms
 */
typedef enum {
	GN_RET_OK								= 0, 	/*!< Everything went OK */
	GN_RET_ERR								= -1, 	/*!< General error */
	GN_RET_ERR_INVALID_ARG					= 0x201,
	GN_RET_ERR_LEAF_NOT_STARTED				= 0x202,/*!< Not possible to start leaf */
	GN_RET_ERR_NODE_NOT_STARTED				= 0x203,
	GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION	= 0x204,/*!< eg. parameter had no write access */
	GN_RET_ERR_EVENT_LOOP_ERROR				= 0x205,/*!< impossible to send message to event loop */
	GN_RET_ERR_LEAF_NOT_FOUND				= 0x206,
	GN_RET_ERR_EVENT_NOT_SENT				= 0x207,
	GN_RET_ERR_MQTT_SUBSCRIBE				= 0x208
} gn_err_t;

typedef void *gn_leaf_config_handle_t;
typedef void *gn_node_config_handle_t;
typedef void *gn_config_handle_t;

typedef void *gn_display_container_t;

typedef struct {
	gn_event_id_t id;
	char leaf_name[GN_LEAF_NAME_SIZE];
	char param_name[GN_LEAF_PARAM_NAME_SIZE];
	char data[GN_LEAF_DATA_SIZE]; /*!< Data associated with this event */
	int data_size; /*!< Length of the data for this event */
} gn_leaf_event_t;

typedef gn_leaf_event_t *gn_leaf_event_handle_t;


typedef struct {
	gn_event_id_t id;
	char node_name[GN_NODE_NAME_SIZE];
	void *data[GN_NODE_DATA_SIZE]; /*!< Data associated with this event */
	int data_size; /*!< Length of the data for this event */
} gn_node_event_t;

typedef gn_node_event_t *gn_node_event_handle_t;

typedef void (*gn_leaf_task_callback)(gn_leaf_config_handle_t leaf_config);

//parameters

typedef enum {
	GN_VAL_TYPE_STRING, GN_VAL_TYPE_BOOLEAN, GN_VAL_TYPE_DOUBLE,
} gn_val_type_t;

/*
 * type of parameter access
 */
typedef enum {
	GN_LEAF_PARAM_ACCESS_WRITE, /*!< param can be modified only by network (eg. configuration settings from environment)*/
	GN_LEAF_PARAM_ACCESS_READ, /*!< param can be modified only by the node (eg. sensor data)*/
	GN_LEAF_PARAM_ACCESS_READWRITE /*!< param can be modified both by the node and network (eg. local configuration settings)*/
} gn_leaf_param_access_t;

/*
 * storage policy
 */
typedef enum {
	GN_LEAF_PARAM_STORAGE_PERSISTED, /*!< param is stored in NVS flash every time it changes*/
	GN_LEAF_PARAM_STORAGE_VOLATILE /*< param is never stored in NVS flash*/
} gn_leaf_param_storage_t;

typedef union {
	char *s;
	bool b;
	double d;
} gn_val_t;

typedef struct {
	gn_val_type_t t;
	gn_val_t v;
} gn_param_val_t;

typedef gn_param_val_t *gn_param_val_handle_t;

struct gn_leaf_param {
	char *name;
	gn_leaf_param_access_t access;
	gn_leaf_param_storage_t storage;
	gn_param_val_handle_t param_val;
	gn_leaf_config_handle_t leaf_config;
	struct gn_leaf_param *next;
};

typedef struct gn_leaf_param gn_leaf_param_t;//  = {NULL, GN_LEAF_PARAM_ACCESS_WRITE, GN_LEAF_PARAM_STORAGE_ALWAYS, NULL, NULL, NULL};

typedef gn_leaf_param_t *gn_leaf_param_handle_t;

//typedef void* gn_leaf_context_handle_t;

size_t gn_common_leaf_event_mask_param(gn_leaf_event_handle_t evt,
		gn_leaf_param_handle_t param);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_COMMONS_H_ */
