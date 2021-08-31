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

#include "esp_log.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "lvgl_helpers.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_commons.h"

#include "gn_pump_control.h"
#include "gn_ds18b20.h"
#include "gn_pump.h"

#define TAG "gn_pump_control"

size_t pump_status = 0;

void gn_pump_control_task_event_handler(void *handler_args,
		esp_event_base_t base, int32_t event_id, void *event_data) {

	ESP_LOGD(TAG, "Pump Control received event: %d", event_id);

	//gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_args;

	gn_leaf_event_t *evt = (gn_leaf_event_t*) event_data;
	switch (event_id) {
	case GN_LEAF_PARAM_CHANGED_EVENT:

		//event is from temp sensor
		if (strcmp(evt->leaf_name, "ds18b20") == 0) {

			//search for temp parameter
			for (int i = 0; i < GN_DS18B20_MAX_SENSORS; i++) {

				if (evt->data != NULL
						&& strcmp(evt->param_name,
								GN_DS18B20_PARAM_SENSOR_NAMES[i]) == 0) {

					//double *temp = (double*) evt->data;
					double temp = strtod(evt->data, NULL);

					ESP_LOGD(TAG,
							"parameter changed on leaf: %s - param %s - value %f",
							evt->leaf_name, GN_DS18B20_PARAM_SENSOR_NAMES[i],
							temp);

					//if temp is > 25 and pump is off, turn on
					if (pump_status == 0 && temp > 26) {
						//send message to pump
						if (gn_send_leaf_param_change_message("pump",
								GN_PUMP_PARAM_STATUS, (const char*)&"1", 2) != GN_RET_OK) {
							ESP_LOGE(TAG,
									"impossible to update parameter %s on leaf %s",
									GN_PUMP_PARAM_STATUS, "pump");
							return;
						}

						ESP_LOGD(TAG, "setting pump to 1");

						pump_status = 1;
					}

					//if temp is < 25 and pump is on, turn off
					if (pump_status == 1 && temp < 26) {
						//send message to pump
						if (gn_send_leaf_param_change_message("pump",
								GN_PUMP_PARAM_STATUS, &"0", 2) != GN_RET_OK) {
							ESP_LOGE(TAG,
									"impossible to update parameter %s on leaf %s",
									GN_PUMP_PARAM_STATUS, "pump");
							return;
						}

						ESP_LOGD(TAG, "setting pump to 0");

						pump_status = 0;
					}
				}
			}
		}
		break;
	}

}

void gn_pump_control_task(gn_leaf_config_handle_t leaf_config) {

	//gn_leaf_event_t evt;
	//bool status = true;

	//register for events
	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(gn_leaf_get_config_event_loop(leaf_config), GN_BASE_EVENT, GN_EVENT_ANY_ID, gn_pump_control_task_event_handler, leaf_config, NULL));

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
