/*
 * srownode_intl.h
 *
 *  Created on: 15 giu 2021
 *      Author: muratori.n
 */

#ifndef COMPONENTS_GROWNODE_SROWNODE_INTL_H_
#define COMPONENTS_GROWNODE_SROWNODE_INTL_H_

#include "grownode.h"



extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

struct gn_config_t {
	esp_vfs_spiffs_conf_t spiffs_conf;
	esp_mqtt_client_handle_t mqtt_client;
	esp_event_loop_handle_t event_loop;
	wifi_init_config_t wifi_config;
	wifi_prov_mgr_config_t prov_config;
	char deviceName[30];
	uint8_t macAddress[6];
	gn_config_status_t status;
	gn_node_config_handle_t node_config;
};

struct gn_node_config_t {
	char name[GN_NODE_NAME_SIZE];
	//esp_event_loop_handle_t event_loop;
	gn_config_handle_t config;
	gn_leaves_list leaves;
};

struct gn_leaf_config_t {
	char name[GN_LEAF_NAME_SIZE];
	gn_node_config_handle_t node_config;
	gn_leaf_task_callback task_cb;
	gn_leaf_display_config_callback display_config_cb;
	//gn_leaf_display_task_t display_task;
	//gn_leaf_config_handle_t next;
	//QueueHandle_t xLeafTaskEventQueue;
	esp_event_loop_handle_t event_loop;
	gn_leaf_param_handle_t params;
};

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



#endif /* COMPONENTS_GROWNODE_SROWNODE_INTL_H_ */
