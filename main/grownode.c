/*
 * grownode.c
 *
 *  Created on: 17 apr 2021
 *      Author: muratori.n
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
//#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
//#include "esp_smartconfig.h"

#include "nvs_flash.h"

#include "driver/gpio.h"

#include "mqtt_client.h"

#include "lwip/apps/sntp.h"

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

#include "gn_commons.h"
#include "gn_event_source.h"
#include "gn_mqtt_protocol.h"
#include "gn_display.h"

static const char *TAG = "grownode";

esp_event_loop_handle_t gn_event_loop;

gn_config_handle_t _gn_default_conf;

gn_config_handle_t _gn_create_config() {
	gn_config_handle_t _conf = (gn_config_handle_t) malloc(sizeof(gn_config_t));
	_conf->event_loop = NULL;
	_conf->mqtt_client = NULL;
	strncpy(_conf->deviceName,"anonymous",10);
	//_conf->prov_config = NULL;
	//_conf->spiffs_conf = NULL;
	//_conf->wifi_config = NULL;
	return _conf;
}


void _gn_init_event_loop(gn_config_handle_t conf) {
	//default event loop
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5,
			.task_name = "loop_task", // task will be created
			.task_priority = 0, .task_stack_size = 2048,
			.task_core_id = 1 };
	ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &gn_event_loop));

	conf->event_loop = gn_event_loop;


}

gn_node_config_handle_t _gn_create_node_config() {
	gn_node_config_handle_t _conf = (gn_node_config_handle_t) malloc(
			sizeof(gn_node_config_t));
	_conf->config = NULL;
	_conf->event_loop = NULL;
	_conf->name = NULL;
	return _conf;
}

gn_node_config_handle_t gn_create_node(gn_config_handle_t config,
		const char *name) {

	if (config == NULL || config->mqtt_client == NULL || name == NULL) {
		ESP_LOGE(TAG, "gn_create_node failed. parameters not correct");
		return NULL;
	}

	gn_node_config_handle_t n_c = _gn_create_node_config();

	//n_c->name = bf;
	n_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(n_c->name, name, GN_MEM_NAME_SIZE);
	n_c->event_loop = config->event_loop;
	n_c->config = config;

	return n_c;
}

esp_err_t gn_destroy_node(gn_node_config_handle_t node) {

	free(node->name);
	free(node);

	return ESP_OK;
}

gn_leaf_config_handle_t _gn_create_leaf_config() {
	gn_leaf_config_handle_t _conf = (gn_leaf_config_handle_t) malloc(
			sizeof(gn_leaf_config_t));
	_conf->callback = NULL;
	_conf->name = NULL;
	_conf->node_config = NULL;
	return _conf;
}

gn_leaf_config_handle_t gn_create_leaf(gn_node_config_handle_t node_cfg,
		const char *name, gn_event_callback_t callback) {

	if (node_cfg == NULL || node_cfg->config == NULL
			|| node_cfg->config->mqtt_client == NULL || name == NULL
			|| callback == NULL) {
		ESP_LOGE(TAG, "gn_create_leaf failed. parameters not correct");
		return NULL;
	}

	gn_leaf_config_handle_t l_c = _gn_create_leaf_config();

	l_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(l_c->name, name, GN_MEM_NAME_SIZE);
	l_c->node_config = node_cfg;
	l_c->callback = callback;

	//mqtt subscribe to topic
	_gn_mqtt_subscribe_leaf(l_c);

	return l_c;
}

esp_err_t gn_destroy_leaf(gn_leaf_config_handle_t leaf) {

	free(leaf->name);
	free(leaf);

	return ESP_OK;

}

void _gn_init_flash(gn_config_handle_t conf) {
	/* Initialize NVS partition */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
			|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		/* NVS partition was truncated
		 * and needs to be erased */
		ESP_ERROR_CHECK(nvs_flash_erase());
		/* Retry nvs_flash_init */
		ESP_ERROR_CHECK(nvs_flash_init());
	}

#ifdef CONFIG_GROWNODE_RESET_PROVISIONED
	ESP_ERROR_CHECK(nvs_flash_erase());
	/* Retry nvs_flash_init */
	ESP_ERROR_CHECK(nvs_flash_init());
#endif

}

void _gn_init_spiffs(gn_config_handle_t conf) {

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t vfs_conf;
	vfs_conf.base_path = "/spiffs";
	vfs_conf.partition_label = NULL;
	vfs_conf.max_files = 6;
	vfs_conf.format_if_mount_failed = true;

	conf->spiffs_conf = vfs_conf;

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&vfs_conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",
					esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
				esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}
}



bool initialized = false;



ESP_EVENT_DEFINE_BASE( GN_BASE_EVENT);

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

void gn_log_message(const char *message) {

	//void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
	ESP_LOGI(TAG, "gn_log_message '%s' (%i)", message, strlen(message));

	//char *ptr = malloc(sizeof(char) * strlen(message) + 1);


	ESP_ERROR_CHECK(
			esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_DISPLAY_LOG_SYSTEM, message,
					strlen(message) + 1, portMAX_DELAY));

	//free(ptr);

	ESP_LOGI(TAG, "end gn_log_message");
}

void _gn_ota_task(void *pvParameter) {

	//vTaskDelay(30000 / portTICK_PERIOD_MS);

	//screenPayload x;
	//x.type = screenPayloadType::LOG;
	//strcpy(x.text, "Firmware update start");
	//xQueueSend(screenEventQueue, &x, 0);
	gn_log_message("OTA in progress..");

	esp_wifi_set_ps(WIFI_PS_NONE);

	esp_http_client_config_t config = { };
	config.url = CONFIG_GROWNODE_FIRMWARE_URL;
	//config.event_handler = _http_event_handler;
	config.cert_pem = (char*) server_cert_pem_start;

	esp_err_t ret = esp_https_ota(&config);
	if (ret == ESP_OK) {

		gn_log_message("Firmware updated. Rebooting..");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		esp_restart();

	} else {

		//screen->log("Firmware Not Updated");
		gn_log_message("Firmware upgrade failed.");
		//vTaskDelay(5000 / portTICK_PERIOD_MS);
		//esp_restart();
	}

	vTaskDelete(NULL);

}

void _gn_update_firmware() {
	xTaskCreate(_gn_ota_task, "_gn_ota_task", 8196, NULL, 10, NULL);
}


/* Signal Wi-Fi events on this event-group */
const int GN_WIFI_CONNECTED_EVENT = BIT0;
const int GN_PROV_END_EVENT = BIT1;

EventGroupHandle_t _gn_event_group_wifi;

int _gn_wifi_connect_retries = 0;

void _gn_wifi_event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {

	ESP_LOGI(TAG, "_gn_wifi_event_handler");

	if (event_base == WIFI_PROV_EVENT) {
		switch (event_id) {
		case WIFI_PROV_START:
			gn_log_message("Provisioning Started");
			break;
		case WIFI_PROV_CRED_RECV: {
			wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t*) event_data;
			ESP_LOGI(TAG,
					"Received Wi-Fi credentials" "\n\tSSID     : %s\n\tPassword : %s",
					(const char*) wifi_sta_cfg->ssid,
					(const char*) wifi_sta_cfg->password);
			break;
		}
		case WIFI_PROV_CRED_FAIL: {
			wifi_prov_sta_fail_reason_t *reason =
					(wifi_prov_sta_fail_reason_t*) event_data;
			_gn_wifi_connect_retries++;
			if (_gn_wifi_connect_retries > 5) {
				nvs_flash_erase();
				esp_restart();
			}

			gn_log_message("Provisioning Failed");

			ESP_LOGE(TAG,
					"Provisioning failed!\n\tReason : %s" "\n\tRetrying %d of 5 times and then reset to factory",
					((*reason == WIFI_PROV_STA_AUTH_ERROR) ?
							"Wi-Fi station authentication failed" :
							"Wi-Fi access-point not found"),
					_gn_wifi_connect_retries);
			break;
		}
		case WIFI_PROV_CRED_SUCCESS:
			_gn_wifi_connect_retries = 0;
			gn_log_message("Provisioning OK");
			break;
		case WIFI_PROV_END:
			ESP_LOGI(TAG, "WIFI_PROV_END");
			/* De-initialize manager once provisioning is finished */
			//wifi_prov_mgr_deinit();
			//xEventGroupSetBits(_gn_event_group_wifi, GN_PROV_END_EVENT);
			break;
		default:
			break;
		}
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
		esp_wifi_connect();
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		_gn_wifi_connect_retries = 0;
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		char *log = (char*) malloc(sizeof(char) * 20);
		sprintf(log, "IP: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
		strncpy(_gn_default_conf->deviceName,log, 20);
		gn_log_message(log);
		//ESP_LOGI(TAG, "IP : " IPSTR, IP2STR(&event->ip_info.ip));
		free(log);
		/* Signal main application to continue execution */
		xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT);
	} else if (event_base == WIFI_EVENT
			&& event_id == WIFI_EVENT_STA_DISCONNECTED) {

		_gn_wifi_connect_retries++;
		if (_gn_wifi_connect_retries > 5) {
			nvs_flash_erase();
			esp_restart();
		}

		ESP_LOGI(TAG,
				"Disconnected. Connecting to the AP again. Trying %d out of 5..",
				_gn_wifi_connect_retries);
		gn_log_message("Disconnected");
		esp_wifi_connect();
	}
}

void _gn_wifi_init_sta(void) {
	/* Start Wi-Fi in station mode */
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void _gn_wifi_get_device_service_name(char *service_name, size_t max) {
	uint8_t eth_mac[6];
	const char *ssid_prefix = "PROV_";
	esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
	snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
			eth_mac[4], eth_mac[5]);
}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
esp_err_t _gn_wifi_custom_prov_data_handler(uint32_t session_id,
		const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen,
		void *priv_data) {
	if (inbuf) {
		ESP_LOGI(TAG, "Received data: %.*s", inlen, (char*) inbuf);
	}
	char response[] = "SUCCESS";
	*outbuf = (uint8_t*) strdup(response);
	if (*outbuf == NULL) {
		ESP_LOGE(TAG, "System out of memory");
		return ESP_ERR_NO_MEM;
	}
	*outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

	return ESP_OK;
}

void _gn_init_wifi(gn_config_handle_t conf) {

	gn_log_message("Wifi Init");

	ESP_ERROR_CHECK(esp_netif_init());
	_gn_event_group_wifi = xEventGroupCreate();

	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
					&_gn_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
					&_gn_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(
			esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
					&_gn_wifi_event_handler, NULL));

	esp_netif_create_default_wifi_sta();

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
	esp_netif_create_default_wifi_ap();
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	conf->wifi_config = cfg;

	/* Configuration for the provisioning manager */
	wifi_prov_mgr_config_t config = {
	/* What is the Provisioning Scheme that we want ?
	 * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
        .scheme = wifi_prov_scheme_ble,
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
			.scheme = wifi_prov_scheme_softap,
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

			/* Any default scheme specific event handler that you would
			 * like to choose. Since our example application requires
			 * neither BT nor BLE, we can choose to release the associated
			 * memory once provisioning is complete, or not needed
			 * (in case when device is already provisioned). Choosing
			 * appropriate scheme specific event handler allows the manager
			 * to take care of this automatically. This can be set to
			 * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
			.scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */
			};

	ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

	conf->prov_config = config;

	bool provisioned = false;

#ifdef CONFIG_GROWNODE_RESET_PROVISIONED
    wifi_prov_mgr_reset_provisioning();
#endif

	/* Let's find out if the device is provisioned */
	ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

	/* If device is not yet provisioned start provisioning service */
	if (!provisioned) {
		ESP_LOGI(TAG, "Starting provisioning");

		/* What is the Device Service Name that we want
		 * This translates to :
		 *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
		 *     - device name when scheme is wifi_prov_scheme_ble
		 */
		char service_name[12];
		_gn_wifi_get_device_service_name(service_name, sizeof(service_name));

		ESP_LOGI(TAG, "service_name: %s", service_name);

		/* What is the security level that we want (0 or 1):
		 *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
		 *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
		 *          using X25519 key exchange and proof of possession (pop) and AES-CTR
		 *          for encryption/decryption of messages.
		 */
		wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

		/* Do we want a proof-of-possession (ignored if Security 0 is selected):
		 *      - this should be a string with length > 0
		 *      - NULL if not used
		 */
		const char *pop = "abcd1234";

		/* What is the service key (could be NULL)
		 * This translates to :
		 *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
		 *     - simply ignored when scheme is wifi_prov_scheme_ble
		 */
		const char *service_key = NULL;

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

		/* An optional endpoint that applications can create if they expect to
		 * get some additional custom data during provisioning workflow.
		 * The endpoint name can be anything of your choice.
		 * This call must be made before starting the provisioning.
		 */
		wifi_prov_mgr_endpoint_create("custom-data");

		/* Start provisioning service */
		ESP_ERROR_CHECK(
				wifi_prov_mgr_start_provisioning(security, pop, service_name,
						service_key));

		/* The handler for the optional endpoint created above.
		 * This call must be made after starting the provisioning, and only if the endpoint
		 * has already been created above.
		 */
		wifi_prov_mgr_endpoint_register("custom-data",
				_gn_wifi_custom_prov_data_handler, NULL);

		/* Uncomment the following to wait for the provisioning to finish and then release
		 * the resources of the manager. Since in this case de-initialization is triggered
		 * by the default event loop handler, we don't need to call the following */
		wifi_prov_mgr_wait();
		wifi_prov_mgr_deinit();

	} else {
		ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

		/* We don't need the manager as device is already provisioned,
		 * so let's release it's resources */
		wifi_prov_mgr_deinit();

		/* Start Wi-Fi station */
		_gn_wifi_init_sta();
	}

	ESP_LOGI(TAG, "Wait for Wi-Fi connection");
	gn_log_message("Connecting...");
	/* Wait for Wi-Fi connection */
	xEventGroupWaitBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT, false,
			true, portMAX_DELAY);

}

static bool time_sync_init_done = false;

esp_err_t _gn_init_time_sync(gn_config_handle_t conf) {

	if (sntp_enabled()) {
		ESP_LOGI(TAG, "SNTP already initialized.");
		time_sync_init_done = true;
		return ESP_OK;
	}
	char *sntp_server_name = CONFIG_GROWNODE_SNTP_SERVER_NAME;
	ESP_LOGI(TAG, "Initializing SNTP. Using the SNTP server: %s",
			sntp_server_name);
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, sntp_server_name);
	sntp_init();
	gn_log_message("Time sync");
	time_sync_init_done = true;
	return ESP_OK;

}

void _gn_evt_ota_start_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	_gn_update_firmware();

}

void _gn_evt_reset_start_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	gn_log_message("resetting flash");
	nvs_flash_erase();
	gn_log_message("reboot in 3 sec");

	vTaskDelay(3000 / portTICK_PERIOD_MS);

	esp_restart();


}


esp_err_t _gn_register_event_handlers(gn_config_handle_t conf) {

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_NET_OTA_START, _gn_evt_ota_start_handler, NULL, NULL));

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_NET_RESET_START, _gn_evt_reset_start_handler, NULL, NULL));

	return ESP_OK;

}


gn_config_handle_t gn_init() {

	if (initialized)
		return _gn_default_conf;

	_gn_default_conf = _gn_create_config();

	//init flash
	_gn_init_flash(_gn_default_conf);
	//init spiffs
	_gn_init_spiffs(_gn_default_conf);
	//init event loop
	_gn_init_event_loop(_gn_default_conf);

	//register to events
	_gn_register_event_handlers(_gn_default_conf);

	//init display
	gn_init_display(_gn_default_conf);
	//vTaskDelay(1000 / portTICK_PERIOD_MS);

	//init wifi
	_gn_init_wifi(_gn_default_conf);
	//init time sync
	_gn_init_time_sync(_gn_default_conf);
	//init mqtt system
	_gn_mqtt_init(_gn_default_conf);


	initialized = true;

	return _gn_default_conf;

}

#ifdef __cplusplus
}
#endif //__cplusplus
