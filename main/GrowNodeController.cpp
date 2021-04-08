/*
 * GrowNode.cpp
 *
 *  Created on: 22 mar 2021
 *      Author: muratori.n
 */

#include "GrowNodeController.h"

namespace GrowNode {
namespace Controller {

static const char *TAG = "GrowNodeController";
GrowNodeController *GrowNodeController::instance = nullptr;

GrowNodeController::GrowNodeController() :
		wifi() {
	init();
}

GrowNodeController::~GrowNodeController() {

}

GrowNodeController* GrowNodeController::getInstance() {

	if (instance == nullptr) {
		instance = new GrowNodeController();
	}

	return instance;
}

void GrowNodeController::init() {

	if (initialized)
		return;

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	gpio_install_isr_service(0);

	initFlash();
	initSPIFFS();

	wifi.init();

	ESP_LOGI(TAG, "init");
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
	nvs_flash_erase();
#endif

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

}

} /* namespace Controller */
} /* namespace GrowNode */
