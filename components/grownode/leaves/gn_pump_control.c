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

#include "gn_ds18b20.h"
#include "gn_pump_control.h"
#include "gn_pump.h"

#define TAG "gn_leaf_pump_control"

size_t pump_status = 0;

void gn_pump_control_task(gn_leaf_config_handle_t leaf_config);

void gn_pump_control_task_event_handler(void *handler_args,
		esp_event_base_t base, int32_t event_id, void *event_data) {

	ESP_LOGD(TAG, "Pump Control received event: %d", event_id);

	//gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_args;

	gn_leaf_parameter_event_t *evt = (gn_leaf_parameter_event_t*) event_data;
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
								GN_PUMP_PARAM_TOGGLE, (const char*) &"1", 2)
								!= GN_RET_OK) {
							gn_log(TAG, GN_LOG_ERROR,
									"impossible to update parameter %s on leaf %s",
									GN_PUMP_PARAM_TOGGLE, "pump");
							return;
						}

						ESP_LOGD(TAG, "setting pump to 1");

						pump_status = 1;
					}

					//if temp is < 25 and pump is on, turn off
					if (pump_status == 1 && temp < 26) {
						//send message to pump
						if (gn_send_leaf_param_change_message("pump",
								GN_PUMP_PARAM_TOGGLE, &"0", 2) != GN_RET_OK) {
							gn_log(TAG, GN_LOG_ERROR,
									"impossible to update parameter %s on leaf %s",
									GN_PUMP_PARAM_TOGGLE, "pump");
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


gn_leaf_descriptor_handle_t gn_pump_control_config(
		gn_leaf_config_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_PUMP_CONTROL_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_pump_control_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	//no data storage needed here

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	return descriptor;

}

void gn_pump_control_task(gn_leaf_config_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);


	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title, leaf_name);
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

		}

		gn_display_leaf_refresh_end();
	}

#endif

	//gn_leaf_event_t evt;
	//bool status = true;

	//register for events
	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(gn_leaf_get_event_loop(leaf_config), GN_BASE_EVENT, GN_EVENT_ANY_ID, gn_pump_control_task_event_handler, leaf_config, NULL));

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
