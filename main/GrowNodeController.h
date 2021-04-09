/*
 * GrowNode.h
 *
 *  Created on: 22 mar 2021
 *      Author: muratori.n
 */

#ifndef COMPONENTS_GROWNODE_GROWNODE_H_
#define COMPONENTS_GROWNODE_GROWNODE_H_

extern "C" {

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
//#include "esp_wifi.h"
#include "esp_event.h"
//#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
//#include "esp_ota_ops.h"
//#include "esp_http_client.h"
//#include "esp_https_ota.h"
//#include "esp_smartconfig.h"

#include "nvs_flash.h"

#include "driver/gpio.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

}

#include "WifiController.h"

#define LV_TICK_PERIOD_MS 1

namespace GrowNode {
namespace Controller {

class GrowNodeController {

private:
	static GrowNodeController *instance;
	int initialized;
	esp_event_loop_handle_t eventLoop;

	GrowNodeController();
	virtual ~GrowNodeController();

	void init();

	void initFlash();
	void initSPIFFS();
	void initGUI();
	void initEventLoop();

	static void guiTask(void *pvParameter);
	static void create_gui();
	static void lv_tick_task(void *arg);

	Wifi::WifiController wifi;


public:
	static GrowNodeController* getInstance();


};

} /* namespace Controller */
} /* namespace GrowNode */

#endif /* COMPONENTS_GROWNODE_GROWNODE_H_ */
