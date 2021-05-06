#ifndef GROWNODE_H_
#define GROWNODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "gn_event_source.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define GN_MEM_NAME_SIZE 32

typedef enum {
	GN_CONFIG_STATUS_NOT_INITIALIZED,
	GN_CONFIG_STATUS_INITIALIZING,
	GN_CONFIG_STATUS_ERROR,
	GN_CONFIG_STATUS_NETWORK_ERROR,
	GN_CONFIG_STATUS_SERVER_ERROR,
	GN_CONFIG_STATUS_OK
} gn_config_status_t;

typedef struct {
	esp_vfs_spiffs_conf_t spiffs_conf;
	esp_mqtt_client_handle_t mqtt_client;
	esp_event_loop_handle_t event_loop;
	wifi_init_config_t wifi_config;
	wifi_prov_mgr_config_t prov_config;
	char deviceName[30];
	uint8_t macAddress[6];
	gn_config_status_t status;
} gn_config_t;

typedef gn_config_t *gn_config_handle_t;

typedef struct {
	gn_event_id_t event;
	char *data; /*!< Data associated with this event */
	int data_len; /*!< Length of the data for this event */
} gn_event_t;

typedef gn_event_t *gn_event_handle_t;

typedef void (*gn_event_callback_t)(gn_event_id_t event, void *event_data);

typedef struct __gn_node_config_t gn_node_config_t;
typedef gn_node_config_t *gn_node_config_handle_t;

typedef struct __gn_leaf_config_t gn_leaf_config_t;
typedef gn_leaf_config_t *gn_leaf_config_handle_t;

typedef struct {
	size_t size;
	size_t last;
	gn_leaf_config_handle_t* at;
} gn_leaves_list;

struct __gn_node_config_t {
	char *name;
	//esp_event_loop_handle_t event_loop;
	gn_config_handle_t config;
	gn_leaves_list leaves;
};

struct __gn_leaf_config_t {
	char *name;
	gn_node_config_handle_t node_config;
	gn_event_callback_t callback;
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

//functions
gn_node_config_handle_t gn_create_node(gn_config_handle_t config,
		const char *name);

esp_err_t gn_destroy_node(gn_node_config_handle_t node);

esp_err_t gn_publish_node(gn_node_config_handle_t node);

gn_config_handle_t gn_init();

gn_leaf_config_handle_t gn_create_leaf(gn_node_config_handle_t node_config,
		const char *name, gn_event_callback_t callback);

esp_err_t gn_destroy_leaf(gn_leaf_config_handle_t leaf);

esp_err_t gn_init_leaf(gn_leaf_config_handle_t leaf);

void gn_log_message(const char *message);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
