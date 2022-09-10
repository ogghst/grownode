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

/*! \file gn_commons.h
    \brief General structures and functions

*/

#ifndef MAIN_GN_COMMONS_H_
#define MAIN_GN_COMMONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GN_LEAF_TASK_PRIORITY 1

#define GN_NODE_NAME_SIZE 16
#define GN_LEAF_NAME_SIZE 16
#define GN_LEAF_PARAM_NAME_SIZE 32
#define GN_LEAF_PARAM_VAL_SIZE 64
#define GN_LEAF_PARAM_UNIT_SIZE 32
#define GN_LEAF_PARAM_FORMAT_SIZE 32
#define GN_LEAF_DATA_SIZE 512
#define GN_NODE_DATA_SIZE 512
#define GN_LEAF_DESC_TYPE_SIZE 32

#define GN_NODE_LEAF_MAX_SIZE 64

#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "gn_event_source.h"

/**
    @brief maximum time between two keepalive messages
*/
static const int16_t GN_CONFIG_MAX_SERVER_KEEPALIVE_SEC = 3600;

/**
 * @brief maximum number of msec to wait on event queue
 */
static const unsigned long GN_MAX_EVENT_WAIT_MS = portMAX_DELAY / portTICK_PERIOD_MS;


#define GN_LEAF_MESSAGE_FALSE "false"
#define GN_LEAF_MESSAGE_TRUE "true"

typedef enum {
	GN_NODE_STATUS_NOT_INITIALIZED = 0,
	GN_NODE_STATUS_INITIALIZING = 1,
	GN_NODE_STATUS_ERROR = 2,
	GN_NODE_STATUS_NETWORK_ERROR = 3,
	GN_NODE_STATUS_SERVER_ERROR = 4,
	GN_NODE_STATUS_READY_TO_START = 5,
	GN_NODE_STATUS_STARTED = 6,
	GN_NODE_STATUS_ERROR_MISSING_FIRMWARE_URL = 7,
	GN_NDOE_STATUS_ERROR_MISSING_PROVISIONING_PASSWORD = 8,
	GN_NODE_STATUS_ERROR_MISSING_SERVER_BASE_TOPIC = 9,
	GN_NODE_STATUS_ERROR_MISSING_SERVER_URL = 10,
	GN_NODE_STATUS_ERROR_BAD_SERVER_KEEPALIVE_SEC = 11,
	GN_NODE_STATUS_ERROR_MISSING_SNTP_URL = 12,
	GN_NODE_STATUS_ERROR_MISSING_SERVER_DISCOVERY_PREFIX = 13,
	GN_NODE_STATUS_SLEEPING = 14
} gn_node_status_t;

typedef enum {
	GN_SLEEP_MODE_NONE = 0,
	GN_SLEEP_MODE_LIGHT = 1,
	GN_SLEEP_MODE_DEEP = 2
} gn_sleep_mode_t;

const char *gn_config_status_descriptions [14];

typedef enum {
	GN_SERVER_CONNECTED, GN_SERVER_DISCONNECTED,
} gn_server_status_t;

/**
 * @brief error codes that the grownode functions can return.
 *
 * The GN_RET_OK and GN_RET_ERR are mapped like ESP_OK and ESP_FAIL for compatibility across platforms
 */
typedef enum {
	GN_RET_OK = 0, /*!< Everything went OK */
	GN_RET_ERR = -1, /*!< General error */
	GN_RET_ERR_INVALID_ARG = 0x201,
	GN_RET_ERR_LEAF_NOT_STARTED = 0x202,/*!< Not possible to start leaf */
	GN_RET_ERR_NODE_NOT_STARTED = 0x203,
	GN_RET_ERR_LEAF_PARAM_ACCESS_VIOLATION = 0x204,/*!< eg. parameter had no write access */
	GN_RET_ERR_EVENT_LOOP_ERROR = 0x205,/*!< impossible to send message to event loop */
	GN_RET_ERR_LEAF_NOT_FOUND = 0x206,
	GN_RET_ERR_EVENT_NOT_SENT = 0x207,
	GN_RET_ERR_MQTT_SUBSCRIBE = 0x208,
	GN_RET_ERR_MQTT_ERROR = 0x209,
	GN_RET_NVS_PARAMETER_NOT_FOUND = 0x301,/*!< parameter requested was not found in NVS */
	GN_RET_NVS_PARAMETER_FOUND = 0x302,/*!< parameter requested was found in NVS */
	GN_RET_VALUE_OUT_OF_LIMIT = 0x401,/*!< trying to set a value outside limits  */
	GN_RET_TIMEOUT = 0x501/*!< timeout expired  */
} gn_err_t;

/**
 * @brief 	result of a validation function run
 */
typedef enum {
	GN_LEAF_PARAM_VALIDATOR_PASSED = 0x000,					/*!< value is compliant */
	GN_LEAF_PARAM_VALIDATOR_ERROR_ABOVE_MAX = 0x001,		/*!< value is over the maximum limit */
	GN_LEAF_PARAM_VALIDATOR_ERROR_BELOW_MIN = 0x002,		/*!< value is below the minimum limit */
	GN_LEAF_PARAM_VALIDATOR_ERROR_NOT_ALLOWED = 0x100,		/*!< value is not allowed for other reasons */
	GN_LEAF_PARAM_VALIDATOR_ERROR_GENERIC = 0x101,			/*!< algorithm has returned an error */
	GN_LEAF_PARAM_VALIDATOR_PASSED_CHANGED = 0x200			/*!< value was not allowed but has been modified by the validator to be compliant*/
} gn_leaf_param_validator_result_t;

typedef enum {
	GN_LOG_DEBUG = ESP_LOG_DEBUG,
	GN_LOG_INFO = ESP_LOG_INFO,
	GN_LOG_WARNING = ESP_LOG_WARN,
	GN_LOG_ERROR = ESP_LOG_ERROR,
} gn_log_level_t;

typedef struct {
	bool provisioning_security;
	char provisioning_password[9];
	int16_t wifi_retries_before_reset_provisioning; /*!< -1 to never lose provisioning (warning: in case of SSID change, no way to reset!*/
	bool server_board_id_topic;
	char server_base_topic[80];
	char server_url[255];
	uint16_t server_keepalive_timer_sec;
	bool server_discovery;
	char server_discovery_prefix[80];
	char firmware_url[255];
	char sntp_url[255];
	uint64_t wakeup_time_millisec; /*! if sleep mode is GN_SLEEP_MODE_LIGHT or GN_SLEEP_MODE_DEEP, sets for how long the board must stay on (counted from boot) !*/
	uint64_t sleep_time_millisec; /*! if sleep mode is GN_SLEEP_MODE_LIGHT or GN_SLEEP_MODE_DEEP, sets for how long the board must sleep !*/
	uint64_t sleep_delay_millisec; /*! if sleep mode is GN_SLEEP_MODE_LIGHT or GN_SLEEP_MODE_DEEP, sets for how long the board must stay on waiting for leaves to complete its job before sleeping!*/
	gn_sleep_mode_t sleep_mode; /*! define if and how the board must sleep !*/
	char timezone[32]; /*! defines the timezone in POSIX time (TZ env variable) !*/

} gn_config_init_param_t;

typedef struct gn_config_init_param_t *gn_config_init_param_handle_t;

typedef void *gn_leaf_handle_t;
typedef void *gn_node_handle_t;
typedef void *gn_config_handle_t;

typedef void *gn_display_container_t;

typedef struct {
	gn_event_id_t id;
	char leaf_name[GN_LEAF_NAME_SIZE];
	char param_name[GN_LEAF_PARAM_NAME_SIZE];
	char data[GN_LEAF_DATA_SIZE]; /*!< Data associated with this event */
	int data_len; /*!< Length of the data for this event */
} gn_leaf_parameter_event_t;

typedef gn_leaf_parameter_event_t *gn_leaf_parameter_event_handle_t;

/*
typedef struct {
	gn_event_id_t id;
	gn_leaf_config_handle_t leaf_config;
} gn_leaf_event_subscription_t;

typedef gn_leaf_event_subscription_t *gn_leaf_event_subscription_handle_t;
*/

typedef struct {
	gn_event_id_t id;
	char node_name[GN_NODE_NAME_SIZE];
	void *data[GN_NODE_DATA_SIZE]; /*!< Data associated with this event */
	int data_size; /*!< Length of the data for this event */
} gn_node_event_t;

typedef gn_node_event_t *gn_node_event_handle_t;

typedef void (*gn_leaf_task_callback)(gn_leaf_handle_t leaf_config);

/**
 * @brief status of the leaf
 */
typedef enum {
	GN_LEAF_STATUS_NOT_INITIALIZED = 0,
	GN_LEAF_STATUS_INITIALIZED = 1,
	GN_LEAF_STATUS_ERROR = 2
} gn_leaf_status_t;

/**
 * @brief this represents the description and status informations of the leaf
 */
typedef struct {
	char type[GN_LEAF_DESC_TYPE_SIZE];
	gn_leaf_task_callback callback;
	gn_leaf_status_t status;
	void *data;
} gn_leaf_descriptor_t;

typedef gn_leaf_descriptor_t *gn_leaf_descriptor_handle_t;

/**
 * this is the leaf configuration callback. every leaf must implement one. it configures internal parameters and
 * returns a leaf config
 */
typedef gn_leaf_descriptor_handle_t (*gn_leaf_config_callback)(
		gn_leaf_handle_t leaf_config);

//parameters

/**
 * @brief type of parameters available
 */
typedef enum {
	GN_VAL_TYPE_STRING, 	/*!< character array, user defined dimension */
	GN_VAL_TYPE_BOOLEAN, 	/*!< true/false */
	GN_VAL_TYPE_DOUBLE,		/*!< floating point with sign */
} gn_val_type_t;

/**
 * @brief	holds the parameter value
 */
typedef union {
	char *s;
	bool b;
	double d;
} gn_val_t;

/*
 * type of parameter accessibility
 */
typedef enum {
	GN_LEAF_PARAM_ACCESS_ALL = 0x01, 			/*!< param can be modified both by the node and network (eg. local configuration settings)*/
	//GN_LEAF_PARAM_ACCESS_NETWORK = 0x02,		/*!< param can be modified only by network (eg. configuration settings from environment)*/
	GN_LEAF_PARAM_ACCESS_NODE = 0x03, 			/*!< param can be modified only by the node (eg. sensor data)*/
	GN_LEAF_PARAM_ACCESS_NODE_INTERNAL = 0x04, 	/*!< param can be modified only by the node (eg. sensor data) and it is not shown externally*/
	GN_LEAF_PARAM_ACCESS_IMMUTABLE = 0x05  /*!< param cannot be modified*/
} gn_leaf_param_access_type_t;

/*
 * storage policy
 */
typedef enum {
	GN_LEAF_PARAM_STORAGE_PERSISTED, 	/*!< param is stored in NVS flash every time it changes*/
	GN_LEAF_PARAM_STORAGE_VOLATILE 		/*< param is never stored in NVS flash*/
} gn_leaf_param_storage_t;

typedef void *gn_leaf_param_handle_t;

//typedef void* gn_leaf_context_handle_t;


//utilities

size_t gn_leaf_event_mask_param(gn_leaf_parameter_event_handle_t evt,
		gn_leaf_param_handle_t param);

uint64_t gn_hash(const char *key);

void gn_hash_str(const char *key, char *buf, size_t len);

gn_err_t gn_event_payload_to_bool(gn_leaf_parameter_event_t evt, bool *_ret);

gn_err_t gn_event_payload_to_double(gn_leaf_parameter_event_t evt, double *_ret);

//TODO remove
//gn_err_t gn_int_from_event(gn_leaf_parameter_event_t evt, int *_ret);

gn_err_t gn_event_payload_to_string(gn_leaf_parameter_event_t evt, char *_ret, int ret_len);

gn_err_t gn_bool_to_event_payload(bool val, gn_leaf_parameter_event_handle_t evt);

gn_err_t gn_double_to_event_payload(double val, gn_leaf_parameter_event_handle_t evt);

gn_err_t gn_string_to_event_payload(char *val, int val_len, gn_leaf_parameter_event_handle_t evt);

//validators
gn_leaf_param_validator_result_t gn_validator_double_positive(
		gn_leaf_param_handle_t param, void **param_value);

gn_leaf_param_validator_result_t gn_validator_double(
		gn_leaf_param_handle_t param, void **param_value);

gn_leaf_param_validator_result_t gn_validator_boolean(
		gn_leaf_param_handle_t param, void **param_value);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_COMMONS_H_ */
