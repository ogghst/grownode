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
#include "esp_event.h"
//#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#ifdef CONFIG_GROWNODE_WIFI_ENABLED
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
//#include "esp_smartconfig.h"

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#else
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

#include "esp_sntp.h"

#include "grownode_intl.h"

#define TAG "gn_network"

/* Signal Wi-Fi events on this event-group */
const int GN_WIFI_CONNECTED_EVENT = BIT0;
const int GN_WIFI_FAIL_EVENT = BIT2;
const int GN_PROV_END_EVENT = BIT1;
const int GN_WIFI_DISCONNECTED_EVENT = BIT3;
const int GN_WIFI_STOPPED_EVENT = BIT4;

EventGroupHandle_t _gn_event_group_wifi;
int s_retry_num = 0;

gn_config_handle_intl_t _conf;

void _gn_wifi_event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	ESP_LOGD(TAG, "_gn_wifi_event_handler");

	if (event_base == WIFI_PROV_EVENT) {
		switch (event_id) {

		case WIFI_PROV_START:
			ESP_LOGD(TAG, "WIFI_PROV_START");
			gn_log(TAG, GN_LOG_INFO, "Provisioning Started");
			break;
		case WIFI_PROV_CRED_RECV: {
			ESP_LOGD(TAG, "WIFI_PROV_CRED_RECV");
			wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t*) event_data;
			ESP_LOGD(TAG,
					"Received Wi-Fi credentials" "\n\tSSID     : %s\n\tPassword : %s",
					(const char* ) wifi_sta_cfg->ssid,
					(const char* ) wifi_sta_cfg->password);
			break;
		}

		case WIFI_PROV_CRED_FAIL: {
			ESP_LOGD(TAG, "WIFI_PROV_CRED_FAIL");
			wifi_prov_sta_fail_reason_t *reason =
					(wifi_prov_sta_fail_reason_t*) event_data;

			gn_log(TAG, GN_LOG_ERROR, "Provisioning failed!\n\tReason : %s",
					((*reason == WIFI_PROV_STA_AUTH_ERROR) ?
							"Wi-Fi station authentication failed" :
							"Wi-Fi access-point not found"));
			wifi_prov_mgr_reset_provisioning();
			gn_reboot();
			break;
		}

		case WIFI_PROV_CRED_SUCCESS:
			ESP_LOGD(TAG, "WIFI_PROV_CRED_SUCCESS");
			break;

		case WIFI_PROV_END:
			ESP_LOGD(TAG, "WIFI_PROV_END");
			gn_log(TAG, GN_LOG_INFO, "Provisioning OK");
			break;
		default:
			break;
		}

	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {

		ESP_LOGD(TAG, "WIFI_EVENT_STA_START");
		esp_wifi_connect();
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

		ESP_LOGD(TAG, "IP_EVENT_STA_GOT_IP");
		s_retry_num = 0;
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;

		char log[42];
		uint8_t eth_mac[6];
		const char ssid_prefix[10];
		strncpy(ssid_prefix, CONFIG_GROWNODE_PROV_SOFTAP_PREFIX, 10);
		esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
		char deviceName[17];
		snprintf(deviceName, 16, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
				eth_mac[4], eth_mac[5]);

		memcpy(_conf->macAddress, eth_mac, 6);
		strcpy(_conf->deviceName, deviceName);

		snprintf(log, 52, "board:%s -IP: %d.%d.%d.%d", deviceName,
				IP2STR(&event->ip_info.ip));
		gn_log(TAG, GN_LOG_DEBUG, log);

		//ESP_LOGI(TAG, "IP : " IPSTR, IP2STR(&event->ip_info.ip));

		esp_event_post_to(_conf->event_loop, GN_BASE_EVENT,
				GN_NET_CONNECTED_EVENT, NULL, 0,
				portMAX_DELAY);

		/* Signal main application to continue execution */
		xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT);

	} else if (event_base == WIFI_EVENT
			&& event_id == WIFI_EVENT_STA_DISCONNECTED) {
		ESP_LOGD(TAG, "WIFI_EVENT_STA_DISCONNECTED");
		wifi_event_sta_disconnected_t *disconnected =
				(wifi_event_sta_disconnected_t*) event_data;
		ESP_LOGD(TAG, "wifi_event_sta_disconnected_t reason: %d",
				disconnected->reason);
		//ESP_LOGI(TAG, "Disconnected. Connecting to the AP again.");

		esp_event_post_to(_conf->event_loop, GN_BASE_EVENT,
				GN_NET_DISCONNECTED_EVENT, NULL, 0,
				portMAX_DELAY);

		xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_DISCONNECTED_EVENT);

		//WIFI_REASON_ASSOC_LEAVE means that is a voluntary disconnect, do not retry to reconnect
		if (disconnected->reason != WIFI_REASON_ASSOC_LEAVE) {

			if (_conf->config_init_params->wifi_retries_before_reset_provisioning
					== -1
					|| s_retry_num
							< _conf->config_init_params->wifi_retries_before_reset_provisioning) {
				ESP_LOGI(TAG, "Retry to connect - attempt %i of %i",
						s_retry_num,
						_conf->config_init_params->wifi_retries_before_reset_provisioning);
				s_retry_num++;
				esp_wifi_connect();
			} else {
				xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_FAIL_EVENT);
			}
			//ESP_LOGI(TAG,"connect to the AP fail");
		}

	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {

		xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_STOPPED_EVENT);

	} else {
		ESP_LOGD(TAG, "other event received. event_base = %s, event_id = %d",
				event_base, event_id);
	}

#endif

}

gn_err_t _gn_wifi_init_sta(void) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	/* Start Wi-Fi in station mode */
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	EventBits_t bits = xEventGroupWaitBits(_gn_event_group_wifi,
			GN_WIFI_CONNECTED_EVENT | GN_WIFI_FAIL_EVENT,
			pdTRUE,
			pdFALSE,
			portMAX_DELAY);

	if (bits & GN_WIFI_CONNECTED_EVENT) {
		ESP_LOGI(TAG, "_gn_wifi_init_sta - connected");
	} else if (bits & GN_WIFI_FAIL_EVENT) {
		ESP_LOGE(TAG,
				"_gn_wifi_init_sta - Failed to connect to AP. Resetting Provisioning Status");
		wifi_prov_mgr_reset_provisioning();
		gn_reboot();
		return GN_RET_ERR;
	} else {
		ESP_LOGE(TAG, "_gn_wifi_init_sta - UNEXPECTED EVENT");
		return GN_RET_ERR;
	}

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

void _gn_wifi_get_device_service_name(char *service_name, size_t max) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	uint8_t eth_mac[6];
	const char *ssid_prefix = CONFIG_GROWNODE_PROV_SOFTAP_PREFIX;
	esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
	snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
			eth_mac[4], eth_mac[5]);

#endif

}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
esp_err_t _gn_wifi_custom_prov_data_handler(uint32_t session_id,
		const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen,
		void *priv_data) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (inbuf) {
		ESP_LOGD(TAG, "Received data: %.*s", inlen, (char* ) inbuf);
	}
	char response[] = "SUCCESS";
	*outbuf = (uint8_t*) strdup(response);
	if (*outbuf == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "System out of memory");
		return ESP_ERR_NO_MEM;
	}
	*outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

#endif

	return ESP_OK;
}

esp_err_t gn_wifi_init(gn_config_handle_intl_t conf) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

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

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

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

		wifi_prov_security_t security = WIFI_PROV_SECURITY_0;

		if (_conf->config_init_params->provisioning_security) {
			security = WIFI_PROV_SECURITY_1;
		}

		/* Do we want a proof-of-possession (ignored if Security 0 is selected):
		 *      - this should be a string with length > 0
		 *      - NULL if not used
		 */
		const char *pop = _conf->config_init_params->provisioning_password;

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
	//xEventGroupWaitBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT, false,
	//true, portMAX_DELAY);
	fail: return ret;

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

#ifdef CONFIG_GROWNODE_WIFI_ENABLED
static bool time_sync_init_done = false;
#endif

void time_sync_notification_cb(struct timeval *tv) {
	ESP_LOGD(TAG, "Notification of a time synchronization event");
}

esp_err_t gn_wifi_time_sync_init(gn_config_handle_t conf) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (sntp_enabled()) {
		ESP_LOGD(TAG, "SNTP already initialized.");
		time_sync_init_done = true;
		return ESP_OK;
	}
	ESP_LOGI(TAG, "Initializing SNTP. Using the SNTP server: %s",
			((gn_config_handle_intl_t )conf)->config_init_params->sntp_url);

#ifdef LWIP_DHCP_GET_NTP_SRV
	sntp_servermode_dhcp(1);
#endif

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0,
			((gn_config_handle_intl_t) conf)->config_init_params->sntp_url);
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	sntp_init();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGD(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    setenv("TZ", ((gn_config_handle_intl_t) conf)->config_init_params->timezone, 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGD(TAG, "The current date/time is: %s", strftime_buf);

	time_sync_init_done = true;
	return ESP_OK;

#else
	return ESP_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

void gn_ota_task(void *pvParameter) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	//vTaskDelay(30000 / portTICK_PERIOD_MS);

	//screenPayload x;
	//x.type = screenPayloadType::LOG;
	//strcpy(x.text, "Firmware update start");
	//xQueueSend(screenEventQueue, &x, 0);
	gn_log(TAG, GN_LOG_INFO, "Firmware update in progress..");

	esp_wifi_set_ps(WIFI_PS_NONE);

	esp_http_client_config_t config = { };
	config.url = _conf->config_init_params->firmware_url;
	//config.event_handler = _http_event_handler;
	config.cert_pem = (char*) server_cert_pem_start;

	esp_err_t ret = esp_https_ota(&config);
	if (ret == ESP_OK) {

		gn_log(TAG, GN_LOG_INFO, "Firmware updated. Rebooting..");
		//vTaskDelay(5000 / portTICK_PERIOD_MS);
		gn_reboot();

	} else {

		//screen->log("Firmware Not Updated");
		gn_log(TAG, GN_LOG_INFO, "Firmware upgrade failed.");
		//vTaskDelay(5000 / portTICK_PERIOD_MS);
		//esp_restart();
	}

	vTaskDelete(NULL);

#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

/**
 * @brief 	stops the wifi connection, this is usually done in power saving mode
 *
 * @param 	config	the configuration to use
 *
 * @return 	GN_RET_ERR_INVALID_ARG 	in case of null _conf
 * @return	GN_RET_ERR 				in case of general errors
 */
gn_err_t gn_wifi_stop(gn_config_handle_t config) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	if (!config)
		return GN_RET_ERR_INVALID_ARG;

	//gn_config_handle_intl_t _config = (gn_config_handle_intl_t) config;

	esp_err_t esp_ret;
	esp_ret = esp_wifi_disconnect();
	if (esp_ret != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_wifi_disconnect: %s",
				esp_err_to_name(esp_ret));
		return GN_RET_ERR;
	}

	ESP_LOGD(TAG, "gn_wifi_stop - Start Wait for Wi-Fi disconnection");

	xEventGroupWaitBits(_gn_event_group_wifi, GN_WIFI_DISCONNECTED_EVENT,
	pdTRUE,
	pdFALSE, portMAX_DELAY);

	ESP_LOGD(TAG, "gn_wifi_stop - End Wait for Wi-Fi disconnection");

	esp_ret = esp_wifi_stop();
	if (esp_ret != ESP_OK) {
		ESP_LOGE(TAG, "Error on esp_wifi_stop: %s", esp_err_to_name(esp_ret));
		return GN_RET_ERR;
	}

	xEventGroupWaitBits(_gn_event_group_wifi, GN_WIFI_STOPPED_EVENT,
	pdTRUE,
	pdFALSE, portMAX_DELAY);

	ESP_LOGD(TAG, "gn_wifi_stop - returning");

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

/**
 * @brief 	reconnect the MQTT subsystem keeping the client configuration
 *
 * @param 	config	the configuration to use
 *
 * @return 	GN_RET_ERR_INVALID_ARG 	in case of null _conf
 * @return	GN_RET_ERR 				in case of general errors
 */
gn_err_t gn_wifi_start(gn_config_handle_t conf) {

#ifdef CONFIG_GROWNODE_WIFI_ENABLED

	_gn_wifi_init_sta();

	return GN_RET_OK;

#else
	return GN_RET_OK;
#endif /* CONFIG_GROWNODE_WIFI_ENABLED */

}

#ifdef __cplusplus
}
#endif //__cplusplus

