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

#ifndef MAIN_GN_MQTT_PROTOCOL_H_
#define MAIN_GN_MQTT_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gn_commons.h"
//#include "esp_log.h"

#define _GN_MQTT_MAX_TOPIC_LENGTH 80
#define _GN_MQTT_MAX_PAYLOAD_LENGTH CONFIG_GROWNODE_MQTT_BUFFER_SIZE

#define _GN_MQTT_COMMAND_MESS "cmd"
#define _GN_MQTT_STATUS_MESS "sts"
#define _GN_MQTT_LOG_MESS "log"

#define _GN_MQTT_PAYLOAD_RST "RST"
#define _GN_MQTT_PAYLOAD_OTA "OTA"
#define _GN_MQTT_PAYLOAD_RBT "RBT"

#define GN_MQTT_NODE_NAME_SIZE 13

#define _GN_MQTT_DEFAULT_QOS 0

gn_err_t gn_mqtt_subscribe_leaf(gn_leaf_handle_t leaf_config);

esp_err_t gn_mqtt_subscribe_leaf_param(gn_leaf_param_handle_t param);

gn_err_t gn_mqtt_start(gn_config_handle_t config);

gn_err_t gn_mqtt_stop(gn_config_handle_t config);

gn_err_t gn_mqtt_disconnect(gn_config_handle_t config);

gn_err_t gn_mqtt_reconnect(gn_config_handle_t config);

gn_err_t gn_mqtt_send_node_config(gn_node_handle_t conf);

gn_err_t gn_mqtt_send_leaf_message(gn_leaf_handle_t leaf,
		const char *msg);

gn_err_t gn_mqtt_send_leaf_param(gn_leaf_param_handle_t config);

gn_err_t gn_mqtt_send_startup_message(gn_config_handle_t _config);

gn_err_t gn_mqtt_send_reboot_message(gn_config_handle_t _config);

gn_err_t gn_mqtt_send_reset_message(gn_config_handle_t _config);

gn_err_t gn_mqtt_send_ota_message(gn_config_handle_t _config);

gn_err_t gn_mqtt_send_log_message(gn_config_handle_t _config, char *log_tag,
		gn_log_level_t level, char *message);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_GN_MQTT_PROTOCOL_H_ */
