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


gn_err_t gn_mqtt_subscribe_leaf(gn_leaf_config_handle_t leaf_config);

esp_err_t gn_mqtt_subscribe_leaf_param(gn_leaf_param_handle_t param);

esp_err_t gn_mqtt_init(gn_config_handle_t conf);

esp_err_t gn_mqtt_send_node_config(gn_node_config_handle_t conf);

esp_err_t gn_mqtt_send_leaf_message(gn_leaf_config_handle_t leaf, const char* msg);

gn_err_t gn_mqtt_send_leaf_param(gn_leaf_param_handle_t config);

gn_server_status_t gn_mqtt_get_status();

#ifdef __cplusplus
}
#endif

#endif /* MAIN_GN_MQTT_PROTOCOL_H_ */
