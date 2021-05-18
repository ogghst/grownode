/*
 * gn_mqtt_protocol.h
 *
 *  Created on: 20 apr 2021
 *      Author: muratori.n
 */

#ifndef MAIN_GN_MQTT_PROTOCOL_H_
#define MAIN_GN_MQTT_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"
#include "esp_log.h"



esp_err_t gn_mqtt_subscribe_leaf(gn_leaf_config_handle_t leaf_config);

esp_err_t gn_mqtt_subscribe_leaf_param(gn_leaf_param_handle_t param);

esp_err_t gn_mqtt_init(gn_config_handle_t conf);

esp_err_t gn_mqtt_send_node_config(gn_node_config_handle_t conf);

esp_err_t gn_mqtt_send_leaf_message(gn_leaf_config_handle_t leaf, const char* msg);

esp_err_t gn_mqtt_send_leaf_param(gn_leaf_param_handle_t config);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_GN_MQTT_PROTOCOL_H_ */
