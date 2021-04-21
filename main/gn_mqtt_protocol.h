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


#define GN_MQTT_MAX_TOPIC_LENGTH 80

#define GN_MQTT_COMMAND_MESS "cmd"

#define GN_MQTT_STATUS_MESS "sts"

typedef struct {
	int nodeid;
	char nodeName[30];
} gn_mqtt_startup_message_t;

typedef gn_mqtt_startup_message_t *gn_mqtt_startup_message_handle_t;

esp_err_t _gn_mqtt_subscribe_leaf(gn_leaf_config_handle_t leaf_config);

void _gn_init_mqtt(gn_config_handle_t conf);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_GN_MQTT_PROTOCOL_H_ */
