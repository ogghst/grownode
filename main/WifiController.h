/*
 * WifiController.h
 *
 *  Created on: 27 mar 2021
 *      Author: muratori.n
 */

#ifndef MAIN_WIFICONTROLLER_H_
#define MAIN_WIFICONTROLLER_H_

extern "C" {

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "stdio.h"
#include "string.h"

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

}

namespace GrowNode {
namespace Controller {

class GrowNodeController;

class WifiController {

private:

	GrowNodeController *controller;

public:
	WifiController(GrowNodeController *_controller);
	virtual ~WifiController();

	void init();

};

} /* namespace Controller */
} /* namespace GrowNode */

#include "GrowNodeController.h"

#endif /* MAIN_WIFICONTROLLER_H_ */
