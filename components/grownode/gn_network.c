#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_check.h"
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

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */


#include "lwip/apps/sntp.h"

#include "grownode.h"

static const char *TAG = "network";

/* Signal Wi-Fi events on this event-group */
const int GN_WIFI_CONNECTED_EVENT = BIT0;
const int GN_PROV_END_EVENT = BIT1;

EventGroupHandle_t _gn_event_group_wifi;

gn_config_handle_t _conf;

void _gn_wifi_event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {

	ESP_LOGD(TAG, "_gn_wifi_event_handler");

	if (event_base == WIFI_PROV_EVENT) {
		switch (event_id) {
		case WIFI_PROV_START:
			gn_message_display("Provisioning Started");
			break;
		case WIFI_PROV_CRED_RECV: {
			wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t*) event_data;
			ESP_LOGD(TAG,
					"Received Wi-Fi credentials" "\n\tSSID     : %s\n\tPassword : %s",
					(const char* ) wifi_sta_cfg->ssid,
					(const char* ) wifi_sta_cfg->password);
			break;
		}
		case WIFI_PROV_CRED_FAIL: {
			wifi_prov_sta_fail_reason_t *reason =
					(wifi_prov_sta_fail_reason_t*) event_data;

			ESP_LOGE(TAG,
					"Provisioning failed!\n\tReason : %s",
					((*reason == WIFI_PROV_STA_AUTH_ERROR) ?
							"Wi-Fi station authentication failed" :
							"Wi-Fi access-point not found"));
			break;
		}
		case WIFI_PROV_CRED_SUCCESS:
			break;
		case WIFI_PROV_END:
			ESP_LOGD(TAG, "WIFI_PROV_END");
			gn_message_display("Provisioning OK");
			break;
		default:
			break;
		}
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGD(TAG, "WIFI_EVENT_STA_START");
		esp_wifi_connect();
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		char log[33]; //TODO make configurable lenght

		uint8_t eth_mac[6];
		const char ssid_prefix[10] = CONFIG_GROWNODE_PROV_SOFTAP_PREFIX;
		esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
		char deviceName[17];
		snprintf(deviceName, 16, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
				eth_mac[4], eth_mac[5]);

		memcpy(_conf->macAddress, eth_mac, 6);
		strcpy(_conf->deviceName, deviceName);

		sprintf(log, "%s-%d.%d.%d.%d", deviceName, IP2STR(&event->ip_info.ip));
		gn_message_display(log);

		//ESP_LOGI(TAG, "IP : " IPSTR, IP2STR(&event->ip_info.ip));

		if (ESP_OK
				!= esp_event_post_to(_conf->event_loop,
						GN_BASE_EVENT, GN_NETWORK_CONNECTED_EVENT, NULL, 0,
						portMAX_DELAY)) {
			ESP_LOGE(TAG, "failed to send GN_NETWORK_DISCONNECTED_EVENT event");
		}

		/* Signal main application to continue execution */
		xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT);

	} else if (event_base == WIFI_EVENT
			&& event_id == WIFI_EVENT_STA_DISCONNECTED) {

		ESP_LOGI(TAG, "Disconnected. Connecting to the AP again.");

		if (ESP_OK
				!= esp_event_post_to(_conf->event_loop,
						GN_BASE_EVENT, GN_NETWORK_DISCONNECTED_EVENT, NULL, 0,
						portMAX_DELAY)) {
			ESP_LOGE(TAG, "failed to send GN_NETWORK_DISCONNECTED_EVENT event");
		}

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
	const char *ssid_prefix = CONFIG_GROWNODE_PROV_SOFTAP_PREFIX;
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
		ESP_LOGD(TAG, "Received data: %.*s", inlen, (char* ) inbuf);
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

esp_err_t _gn_init_wifi(gn_config_handle_t conf) {

	_conf = conf;

	esp_err_t ret = ESP_OK;

	ESP_GOTO_ON_ERROR(esp_netif_init(), fail, TAG, "");

	_gn_event_group_wifi = xEventGroupCreate();

	ESP_GOTO_ON_ERROR(
			esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &_gn_wifi_event_handler, NULL),
			fail, TAG, "");

	ESP_GOTO_ON_ERROR(
			esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_gn_wifi_event_handler, NULL),
			fail, TAG, "");

	ESP_GOTO_ON_ERROR(
			esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_gn_wifi_event_handler, NULL),
			fail, TAG, "");

	esp_netif_create_default_wifi_sta();

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
	esp_netif_create_default_wifi_ap();
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;

	ESP_GOTO_ON_ERROR(esp_wifi_init(&cfg), fail, TAG, "");

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

	ESP_GOTO_ON_ERROR(wifi_prov_mgr_init(config), fail, TAG, "");

	conf->prov_config = config;

	bool provisioned = false;

#ifdef CONFIG_GROWNODE_RESET_PROVISIONED
    wifi_prov_mgr_reset_provisioning();
#endif

	/* Let's find out if the device is provisioned */
	ESP_GOTO_ON_ERROR(wifi_prov_mgr_is_provisioned(&provisioned), fail, TAG,
			"");

	/* If device is not yet provisioned start provisioning service */
	if (!provisioned) {
		ESP_LOGI(TAG, "Starting provisioning");

		/* What is the Device Service Name that we want
		 * This translates to :
		 *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
		 *     - device name when scheme is wifi_prov_scheme_ble
		 */
		char service_name[18];
		_gn_wifi_get_device_service_name(service_name, sizeof(service_name));

		ESP_LOGD(TAG, "service_name: %s", service_name);

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

#ifdef CONFIG_GROWNODE_PROV_POP
		const char *pop = CONFIG_GROWNODE_PROV_POP;
#else
		const char *pop = NULL;
#endif

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

		ESP_GOTO_ON_ERROR(
				wifi_prov_mgr_start_provisioning(security, pop, service_name,
						service_key), fail, TAG, "");

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

	ESP_LOGD(TAG, "Wait for Wi-Fi connection");

	/* Wait for Wi-Fi connection */
	xEventGroupWaitBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT, false,
	true, portMAX_DELAY);

	fail: return ret;

}

static bool time_sync_init_done = false;

esp_err_t _gn_init_time_sync(gn_config_handle_t conf) {

	if (sntp_enabled()) {
		ESP_LOGD(TAG, "SNTP already initialized.");
		time_sync_init_done = true;
		return ESP_OK;
	}
	char *sntp_server_name = CONFIG_GROWNODE_SNTP_SERVER_NAME;
	ESP_LOGI(TAG, "Initializing SNTP. Using the SNTP server: %s",
			sntp_server_name);
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, sntp_server_name);
	sntp_init();
	time_sync_init_done = true;
	return ESP_OK;

}


void _gn_ota_task(void *pvParameter) {

	//vTaskDelay(30000 / portTICK_PERIOD_MS);

	//screenPayload x;
	//x.type = screenPayloadType::LOG;
	//strcpy(x.text, "Firmware update start");
	//xQueueSend(screenEventQueue, &x, 0);
	gn_message_display("Firmware update in progress..");

	esp_wifi_set_ps(WIFI_PS_NONE);

	esp_http_client_config_t config = { };
	config.url = CONFIG_GROWNODE_FIRMWARE_URL;
	//config.event_handler = _http_event_handler;
	config.cert_pem = (char*) server_cert_pem_start;

	esp_err_t ret = esp_https_ota(&config);
	if (ret == ESP_OK) {

		gn_message_display("Firmware updated. Rebooting..");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		esp_restart();

	} else {

		//screen->log("Firmware Not Updated");
		gn_message_display("Firmware upgrade failed.");
		//vTaskDelay(5000 / portTICK_PERIOD_MS);
		//esp_restart();
	}

	vTaskDelete(NULL);

}

#ifdef __cplusplus
}
#endif //__cplusplus

