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
#include "gn_pump.h"

#include "gn_watering_control.h"

#define TAG "gn_leaf_watering_control"

void gn_watering_control_task(gn_leaf_config_handle_t leaf_config);

// workflow (into task loop)

//if time to water

//start watering task

//check water temperature

//start water temperature management loop

//if not enough water - send error and end of cycle

//if out of allowable range - send error and end of cycle

//if out of maximum watering time - send error and end of cycle

//if temperature not suitable for watering, enable peltier and climate control pump and add time

//if suitable for watering

//enable watering pump

//add time to the watering time

//if watering time is enough - end of cycle

//check parameter update

//check temperature treshold

//check time between irrigation

//check irrigation duration

//check manual override to start or stop irrigation

gn_leaf_descriptor_handle_t gn_watering_control_config(
		gn_leaf_config_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_WATERING_CONTROL_TYPE,
	GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_watering_control_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	//no data storage needed here

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	return descriptor;

}

void gn_watering_control_task(gn_leaf_config_handle_t leaf_config) {

	ESP_LOGD(TAG, "%s - gn_watering_control_task",
			gn_leaf_get_config_name(leaf_config));

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);

	gn_leaf_event_subscribe(leaf_config, GN_LEAF_PARAM_CHANGED_EVENT);

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);

		if (_cnt) {

			lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title,
					gn_leaf_get_config_name(leaf_config));
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

		}

		gn_display_leaf_refresh_end();
	}

#endif

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			ESP_LOGD(TAG, "event %d", evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change for this node
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "request to update param %s, data = '%s'",
						evt.param_name, evt.data);

				break;

			case GN_LEAF_PARAM_CHANGED_EVENT:

				ESP_LOGD(TAG, "notified update param %s, data = '%s'",
						evt.param_name, evt.data);

				break;

			default:
				break;

			}

		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
