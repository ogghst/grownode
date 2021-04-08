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

}

#include "WifiController.h"

namespace GrowNode {
namespace Controller {

class GrowNodeController {

private:
	static GrowNodeController *instance;
	int initialized;

	GrowNodeController();
	virtual ~GrowNodeController();

	void init();

	void initFlash();
	void initSPIFFS();

	Wifi::WifiController wifi;


public:
	static GrowNodeController* getInstance();


};

} /* namespace Controller */
} /* namespace GrowNode */

#endif /* COMPONENTS_GROWNODE_GROWNODE_H_ */
