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

} /* extern "C" */

} /* namespace Controller */
} /* namespace GrowNode */
