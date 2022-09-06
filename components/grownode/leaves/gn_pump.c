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

#include "gn_pump.h"

#define TAG "gn_leaf_pump"

typedef struct {

	gn_leaf_param_handle_t gn_pump_channel_param;
	gn_leaf_param_handle_t gn_pump_toggle_param;
	gn_leaf_param_handle_t gn_pump_gpio_toggle_param;
	gn_leaf_param_handle_t gn_pump_power_param;
	gn_leaf_param_handle_t gn_pump_gpio_power_param;
	mcpwm_unit_t pwm_unit;
	mcpwm_timer_t pwm_timer;
	mcpwm_generator_t pwm_generator;
	mcpwm_pin_config_t *pin_config;
	mcpwm_config_t *pwm_config;

} gn_pump_data_t;

void gn_pump_task(gn_leaf_handle_t leaf_config);

gn_leaf_descriptor_handle_t gn_pump_config(gn_leaf_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_PUMP_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_pump_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	gn_pump_data_t *data = malloc(sizeof(gn_pump_data_t));

	//parameter definition. if found in flash storage, they will be created with found values instead of default
	data->gn_pump_toggle_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_TOGGLE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_pump_toggle_param);

	data->gn_pump_channel_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_CHANNEL, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_pump_channel_param);

	data->gn_pump_gpio_toggle_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_GPIO_TOGGLE, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 32 }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_pump_gpio_toggle_param);

	data->gn_pump_power_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_POWER, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_pump_power_param);

	data->gn_pump_gpio_power_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_GPIO_POWER, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 32 }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_pump_gpio_power_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_pump_task(gn_leaf_handle_t leaf_config) {

	bool need_update = true;

	gn_leaf_parameter_event_t evt;

	esp_err_t ret = ESP_OK;

	//retrieves status descriptor from config
	gn_pump_data_t *data =
			(gn_pump_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	double gpio_toggle;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_PARAM_GPIO_TOGGLE,
			&gpio_toggle);

	double gpio_power;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_PARAM_GPIO_POWER,
			&gpio_power);

	double power;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_PARAM_POWER, &power);

	bool toggle;
	gn_leaf_param_get_bool(leaf_config, GN_PUMP_PARAM_TOGGLE, &toggle);

	bool channel;
	gn_leaf_param_get_bool(leaf_config, GN_PUMP_PARAM_CHANNEL, &channel);

	//setup toggle
	gpio_set_direction(gpio_toggle, GPIO_MODE_OUTPUT);
	gpio_set_level(gpio_toggle, toggle ? 1 : 0);

	//setup pwm

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "%s - setting pwm pin %d channel %d", leaf_name,
			(int )gpio_toggle, (int )channel);

	mcpwm_unit_t pwm_unit = channel ? MCPWM_UNIT_1 : MCPWM_UNIT_0;

	/*
	 mcpwm_io_signals_t pwm_signal =
	 data->gn_pump_channel_param->param_val->v.b ? MCPWM1A : MCPWM0A;
	 */

	mcpwm_timer_t pwm_timer = channel ? MCPWM_TIMER_1 : MCPWM_TIMER_0;

	mcpwm_generator_t pwm_generator = channel ? MCPWM_GEN_B : MCPWM_GEN_A;

	mcpwm_pin_config_t pin_config = { .mcpwm0a_out_num = (
			channel ? 0 : gpio_power), .mcpwm1a_out_num = (
			channel ? gpio_power : 0)

	};
	mcpwm_set_pin(pwm_unit, &pin_config);
	mcpwm_config_t pwm_config;
	pwm_config.frequency = 10000; //TODO make configurable
	pwm_config.cmpr_a = 0.0;
	pwm_config.cmpr_b = 0.0;
	pwm_config.counter_mode = MCPWM_UP_COUNTER;
	pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(pwm_unit, pwm_timer, &pwm_config); //Configure PWM0A & PWM0B with above settings

	ESP_LOGD(TAG, "%s - pwm initialized", leaf_name);

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_status = NULL;
	lv_obj_t *label_power = NULL;
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			//style from the container
			lv_style_t *style = _cnt->styles->style;

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

			label_status = lv_label_create(_cnt);
			//lv_obj_add_style(label_status, style, 0);
			lv_label_set_text(label_status,
					toggle ?
							"status: on" : "status: off");
			//lv_obj_align_to(label_status, label_title, LV_ALIGN_BOTTOM_LEFT,
			//		LV_PCT(10), LV_PCT(10));
			lv_obj_set_grid_cell(label_status, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 2);

			label_power = lv_label_create(_cnt);
			lv_obj_add_style(label_power, style, 0);

			char _p[21];
			snprintf(_p, 20, "power: %4.0f",
					power);
			lv_label_set_text(label_power, _p);
			//lv_obj_align_to(label_power, label_status, LV_ALIGN_TOP_LEFT, 0,
			//		LV_PCT(10));
			lv_obj_set_grid_cell(label_power, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 2, 2);

		}

		gn_display_leaf_refresh_end();

	}

#endif

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			ESP_LOGD(TAG, "received message: %d", evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "request to update param %s, data = '%s'",
						evt.param_name, evt.data);

				//parameter is status
				if (gn_leaf_event_mask_param(&evt, data->gn_pump_toggle_param)
						== 0) {

					bool ret = false;
					if (gn_event_payload_to_bool(evt, &ret) != GN_RET_OK) {
						break;
					}

					//execute change
					gn_leaf_param_force_bool(leaf_config, GN_PUMP_PARAM_TOGGLE,
							ret);

					need_update = true;

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
								toggle ?
										"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
#endif

					//parameter is power
				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_pump_power_param) == 0) {

					double pow = 0;
					if (gn_event_payload_to_double(evt, &pow) != GN_RET_OK) {
						break;
					}

					if (pow < 0)
						pow = 0;
					if (pow > 100)
						pow = 100;

					//execute change
					gn_leaf_param_force_double(leaf_config, GN_PUMP_PARAM_POWER,
							pow);

					need_update = true;

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {
						char _p[21];
						snprintf(_p, 20, "power: %f", pow);
						lv_label_set_text(label_power, _p);

						gn_display_leaf_refresh_end();
					}
#endif

				}

				break;

				//what to do when network is connected
			case GN_NET_CONNECTED_EVENT:
				//gn_pump_state = GN_PUMP_STATE_RUNNING;
				break;

				//what to do when network is disconnected
			case GN_NET_DISCONNECTED_EVENT:
				break;

				//what to do when server is connected
			case GN_SRV_CONNECTED_EVENT:
				break;

				//what to do when server is disconnected
			case GN_SRV_DISCONNECTED_EVENT:
				break;

			default:
				break;

			}

		}

		if (need_update == true) {

			//finally, we update sensor using the parameter values
			gn_leaf_param_get_bool(leaf_config, GN_PUMP_PARAM_TOGGLE, &toggle);
			gn_leaf_param_get_double(leaf_config, GN_PUMP_PARAM_POWER, &power);

			if (!toggle) {
				ret = gpio_set_level(gpio_toggle, 0);

				if (ret != ESP_OK) {
					gn_log(TAG, GN_LOG_ERROR,
							"error in disabling signal, channel %d",
							(int) channel);
				}

				ret = mcpwm_set_duty(pwm_unit, pwm_timer, pwm_generator, 0);

				if (ret != ESP_OK) {
					gn_log(TAG, GN_LOG_ERROR,
							"error in changing power, channel %d",
							(int) channel);
				}

				ESP_LOGD(TAG, "%s - toggle off", leaf_name);

				need_update = false;

			} else {

				ret = gpio_set_level(gpio_toggle, 1);

				if (ret != ESP_OK) {
					gn_log(TAG, GN_LOG_ERROR,
							"error in setting signal, channel %d",
							(int) channel);
				}

				ret = mcpwm_set_duty(pwm_unit, pwm_timer, pwm_generator, power);

				if (ret != ESP_OK) {
					gn_log(TAG, GN_LOG_ERROR,
							"error in  changing power, channel %d",
							(int) channel);
				}

				ESP_LOGD(TAG, "%s - setting power pin %d to %d", leaf_name,
						(int )gpio_power, (int )power);
				need_update = false;
			}
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
