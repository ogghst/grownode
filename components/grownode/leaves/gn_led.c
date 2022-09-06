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

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "lvgl_helpers.h"
#endif

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_led.h"

#define TAG "gn_leaf_led"

void gn_led_task(gn_leaf_handle_t leaf_config);

typedef struct {
	gn_leaf_param_handle_t gn_led_status_param;
	gn_leaf_param_handle_t gn_led_inverted_param;
	gn_leaf_param_handle_t gn_led_blinktime_param;
	gn_leaf_param_handle_t gn_led_gpio_param;
	bool blink_status;
	int gpio;
} gn_led_data_t;

void blink_callback(const gn_leaf_handle_t leaf_config) {

	gn_led_data_t *data =
			(gn_led_data_t*) gn_leaf_get_descriptor(leaf_config)->data;
	data->blink_status = !data->blink_status;
	gpio_set_level(data->gpio, data->blink_status);
}

gn_leaf_descriptor_handle_t gn_led_config(gn_leaf_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_LED_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_led_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_led_data_t *data = malloc(sizeof(gn_led_data_t));

	data->gn_led_status_param = gn_leaf_param_create(leaf_config,
			GN_LED_PARAM_TOGGLE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_VOLATILE, gn_validator_boolean);

	data->gn_led_inverted_param = gn_leaf_param_create(leaf_config,
			GN_LED_PARAM_INVERTED, GN_LEAF_PARAM_ACCESS_ALL, (gn_val_t ) { .b =
					false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_boolean);

	data->gn_led_blinktime_param = gn_leaf_param_create(leaf_config,
			GN_LED_PARAM_BLINK_TIME_MS, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_led_gpio_param = gn_leaf_param_create(leaf_config,
			GN_LED_PARAM_GPIO, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 32 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_PERSISTED,
			gn_validator_double_positive);

	gn_leaf_param_add_to_leaf(leaf_config, data->gn_led_status_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_led_inverted_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_led_blinktime_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_led_gpio_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_led_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "Initializing led leaf %s..", leaf_name);

	//retrieves status descriptor from config
	gn_led_data_t *data =
			(gn_led_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_LED_PARAM_GPIO, &gpio);

	bool status;
	gn_leaf_param_get_bool(leaf_config, GN_LED_PARAM_TOGGLE, &status);

	bool inverted;
	gn_leaf_param_get_bool(leaf_config, GN_LED_PARAM_INVERTED, &inverted);

	double blinktime;
	gn_leaf_param_get_double(leaf_config, GN_LED_PARAM_BLINK_TIME_MS,
			&blinktime);

	bool _act_status = status ? (inverted ? 0 : 1) : (inverted ? 1 : 0);

	data->gpio = gpio;
	data->blink_status = _act_status;

	//setup
	gpio_set_direction((int) gpio, GPIO_MODE_OUTPUT);
	gpio_set_level((int) gpio, _act_status);

	//creates the blink timer
	esp_timer_handle_t timer_blink_handler;
	esp_timer_create_args_t timer_blink_args = { .callback = &blink_callback,
			.arg = leaf_config, .name = "blink_timer" };
	esp_timer_create(&timer_blink_args, &timer_blink_handler);

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_status = NULL;
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			//style from the container
			lv_style_t *style = _cnt->styles->style;

			lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = {90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
			lv_coord_t row_dsc[] = {20, 20, 20, LV_GRID_FR(1),
				LV_GRID_TEMPLATE_LAST};
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title, leaf_name);
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

			label_status = lv_label_create(_cnt);
			//lv_obj_add_style(label_status, style, 0);
			lv_label_set_text(label_status,
					"status: off");
			//lv_obj_align_to(label_status, label_title, LV_ALIGN_BOTTOM_LEFT,
			//		LV_PCT(10), LV_PCT(10));
			lv_obj_set_grid_cell(label_status, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 2);

		}

		gn_display_leaf_refresh_end();

	}

#endif

	bool _changed = true;
	gn_leaf_parameter_event_t evt;

	//task cycle
	while (true) {

		//ESP_LOGD(TAG, "task cycle..");

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			ESP_LOGD(TAG, "%s - received message: %d", leaf_name, evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "request to update param %s, data = '%s'",
						evt.param_name, evt.data);

				//status change
				if (gn_leaf_event_mask_param(&evt, data->gn_led_status_param)
						== 0) {

					bool _val = false;
					if (gn_event_payload_to_bool(evt, &_val) != GN_RET_OK) {
						break;
					}

					gn_leaf_param_force_bool(leaf_config, GN_LED_PARAM_TOGGLE,
							_val);
					gn_leaf_param_get_bool(leaf_config, GN_LED_PARAM_TOGGLE,
							&status);
					_changed = true;
				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_led_inverted_param) == 0) {

					bool _val = false;
					if (gn_event_payload_to_bool(evt, &_val) != GN_RET_OK) {
						break;
					}

					gn_leaf_param_force_bool(leaf_config, GN_LED_PARAM_INVERTED,
							_val);
					gn_leaf_param_get_bool(leaf_config, GN_LED_PARAM_INVERTED,
							&inverted);
					_changed = true;
				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_led_blinktime_param) == 0) {

					double _val = 0;
					if (gn_event_payload_to_double(evt, &_val) != GN_RET_OK) {
						break;
					}

					gn_leaf_param_force_double(leaf_config,
							GN_LED_PARAM_BLINK_TIME_MS,
							_val);
					gn_leaf_param_get_double(leaf_config,
							GN_LED_PARAM_BLINK_TIME_MS, &blinktime);
					_changed = true;
				}
				/*else if (gn_leaf_event_mask_param(&evt,
				 data->gn_led_gpio_param) == 0) {
				 gn_leaf_param_set_double(leaf_config, GN_LED_PARAM_GPIO,
				 (double) atof(evt.data));
				 gn_leaf_param_get_double(leaf_config, GN_LED_PARAM_GPIO,
				 &gpio);
				 _changed = true;
				 }*/

				break;

			default:
				break;

			}

		}

		if (_changed) {

			_act_status = status ? (inverted ? 0 : 1) : (inverted ? 1 : 0);

			ESP_LOGD(TAG, "%s - gpio %d, status %d", leaf_name, (int )gpio,
					_act_status);

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
								status ?
								"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
#endif

			//finally, define if led has to blink
			if (blinktime > 0 && _act_status) {
				if (esp_timer_is_active(timer_blink_handler))
					esp_timer_stop(timer_blink_handler);
				esp_timer_start_periodic(timer_blink_handler, blinktime * 1000);
			} else if (blinktime == 0 && _act_status) {
				if (esp_timer_is_active(timer_blink_handler))
					esp_timer_stop(timer_blink_handler);
				gpio_set_level((int) gpio, _act_status);
				data->blink_status = _act_status;
			} else {
				gpio_set_level((int) gpio, _act_status);
				data->blink_status = _act_status;
			}
			_changed = false;
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
