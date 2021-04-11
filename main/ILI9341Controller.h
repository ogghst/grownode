/*
 * ILI9341Controller.h
 *
 *  Created on: 9 mar 2021
 *      Author: muratori.n
 */

#ifndef MAIN_ILI9341CONTROLLER_H_
#define MAIN_ILI9341CONTROLLER_H_

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "event_source.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

}

namespace GrowNode {
namespace Controller {

class GrowNodeController;

class ILI9341Controller {

public:

	ILI9341Controller(GrowNodeController *_controller);
	~ILI9341Controller();

	void init();

private:

	GrowNodeController *controller;

	bool initialized;

	//EventGroupHandle_t renderEvent;

	//SemaphoreHandle_t msgMutex;
	//SemaphoreHandle_t renderMutex;

	static void guiTask(void *pvParameter);
	static void create_gui();
	static void lv_tick_task(void *arg);
	static void log_system_handler(void *handler_args, esp_event_base_t base,
			int32_t id, void *event_data);
	static void ota_event_handler(lv_obj_t * obj, lv_event_t event);

};

}
}

#endif /* MAIN_ILI9341CONTROLLER_H_ */
