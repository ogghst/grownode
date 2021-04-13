/*
 * GrowNode.cpp
 *
 *  Created on: 22 mar 2021
 *      Author: muratori.n
 */

#include "GrowNodeController.h"

namespace GrowNode {
namespace Controller {

static constexpr char TAG[] = "GrowNodeController";

GrowNodeController::GrowNodeController() :
		wifi { this }, display { this } {

	initialized = false;
	init();
}

GrowNodeController::~GrowNodeController() {

}

/*
 GrowNodeController* GrowNodeController::getInstance() {

 if (instance == nullptr) {
 instance = new GrowNodeController();
 }

 return instance;
 }
 */

void GrowNodeController::init() {

	if (initialized)
		return;

	ESP_LOGI(TAG, "init - START");

	//gpio_install_isr_service(0);

	ESP_LOGI(TAG, "initFlash");
	initFlash();
	ESP_LOGI(TAG, "initEventLoop");
	initEventLoop();
	ESP_LOGI(TAG, "initSPIFFS");
	initSPIFFS();
	ESP_LOGI(TAG, "initGUI");
	initGUI();

	ESP_LOGI(TAG, "initWifi");
	wifi.init();

	ESP_LOGI(TAG, "initMQTT");
	initMQTT();

	ESP_LOGI(TAG, "init - END");

	logMessage("GrowNode Initialized");

	initialized = true;
}

void GrowNodeController::initEventLoop() {

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5,
			.task_name = "loop_task", // task will be created
			.task_priority = uxTaskPriorityGet(NULL), .task_stack_size = 2048,
			.task_core_id = tskNO_AFFINITY };
	ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &eventLoop));
	//ESP_ERROR_CHECK(esp_event_handler_register_with(eventLoop, TASK_EVENTS, TEXT_EVENT, textEventHandlerCallback, eventLoop));
}

void GrowNodeController::initFlash() {
	/* Initialize NVS partition */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
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

void GrowNodeController::initGUI() {
	display.init();
}

void GrowNodeController::initSPIFFS() {

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t vfs_conf;
	vfs_conf.base_path = "/spiffs";
	vfs_conf.partition_label = NULL;
	vfs_conf.max_files = 6;
	vfs_conf.format_if_mount_failed = true;

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
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

	/*
	 DIR *dir = opendir("/spiffs");
	 assert(dir != NULL);
	 while (true) {
	 struct dirent *pe = readdir(dir);
	 if (!pe)
	 break;
	 ESP_LOGI(TAG, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino,
	 pe->d_type);
	 }
	 closedir(dir);
	 */

}



void GrowNodeController::initMQTT() {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_GROWNODE_MQTT_URL
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}


void GrowNodeController::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
    	char buf[30];
        snprintf(buf, 29, "%.*s\r", event->data_len, event->data);
        logMessage(buf);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

extern "C" {

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

ESP_EVENT_DEFINE_BASE(MESSAGE_EVENT);

#define OTA_URL_SIZE 256






void GrowNodeController::logMessage(const char *message) {

	//display.log(message);
	void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
	//ESP_LOGI(TAG, "logMessage '%s', size %i", message, strlen(message) +1);
	ESP_ERROR_CHECK(
			esp_event_post(MESSAGE_EVENT, LOG_SYSTEM, ptr, strlen(message) +1 , portMAX_DELAY));
}

void GrowNodeController::updateFirmware() {
	xTaskCreate(OTATask, "ota_task", 8196, NULL, 10, NULL);
}

void GrowNodeController::OTATask(void *pvParameter) {

	//vTaskDelay(30000 / portTICK_PERIOD_MS);

	//screenPayload x;
	//x.type = screenPayloadType::LOG;
	//strcpy(x.text, "Firmware update start");
	//xQueueSend(screenEventQueue, &x, 0);
	logMessage("OTA in progress..");

	esp_wifi_set_ps(WIFI_PS_NONE);

	esp_http_client_config_t config = { };
	config.url = CONFIG_GROWNODE_FIRMWARE_URL;//"http://discoboy.duckdns.org/esp/test_esp32_core.bin";
	//config.event_handler = _http_event_handler;
	config.cert_pem = (char*) server_cert_pem_start;

	esp_err_t ret = esp_https_ota(&config);
	if (ret == ESP_OK) {

		logMessage("Firmware updated. Rebooting..");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		esp_restart();

	} else {

		//screen->log("Firmware Not Updated");
		logMessage("Firmware upgrade failed.");
		//vTaskDelay(5000 / portTICK_PERIOD_MS);
		//esp_restart();
	}

	vTaskDelete(NULL);

}

void GrowNodeController::log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}




} /* extern "C" */

} /* namespace Controller */
} /* namespace GrowNode */
