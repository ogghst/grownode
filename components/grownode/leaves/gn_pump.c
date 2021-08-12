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

#include "gn_pump.h"

static const char *TAG = "gn_pump";

void gn_pump_task(gn_leaf_config_handle_t leaf_config) {

	const size_t GN_PUMP_STATE_STOP = 0;
	const size_t GN_PUMP_STATE_RUNNING = 1;
	const size_t GPIO_PWM0A_OUT = 32;

	size_t gn_pump_state = GN_PUMP_STATE_RUNNING;
	gn_leaf_event_t evt;

	//parameter definition. if found in flash storage, they will be created with found values instead of default
	gn_leaf_param_handle_t status_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_STATUS, GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b = false }, GN_LEAF_PARAM_WRITE);
	gn_leaf_param_add(leaf_config, status_param);

	gn_leaf_param_handle_t power_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_PARAM_POWER, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_WRITE);
	gn_leaf_param_add(leaf_config, power_param);

	//setup pwm
	mcpwm_pin_config_t pin_config = { .mcpwm0a_out_num = GPIO_PWM0A_OUT };
	mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
	mcpwm_config_t pwm_config;
	pwm_config.frequency = 3000; //TODO make configurable
	pwm_config.cmpr_a = power_param->param_val->v.d;
	pwm_config.cmpr_b = 0.0;
	pwm_config.counter_mode = MCPWM_UP_COUNTER;
	pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	static lv_obj_t *label_status;
	static lv_obj_t *label_power;
	static lv_obj_t *label_pump;

	//parent container where adding elements
	lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);

	if (_cnt) {

		//style from the container
		lv_style_t *style = lv_style_list_get_local_style(&_cnt->style_list);

		label_pump = lv_label_create(_cnt, NULL);
		lv_label_set_text(label_pump, "PUMP");
		lv_obj_add_style(label_pump, LV_LABEL_PART_MAIN, style);

		label_status = lv_label_create(_cnt, NULL);
		lv_obj_add_style(label_status, LV_LABEL_PART_MAIN, style);
		lv_label_set_text(label_status,
				status_param->param_val->v.b ? "status: on" : "status: off");

		label_power = lv_label_create(_cnt, NULL);
		lv_obj_add_style(label_power, LV_LABEL_PART_MAIN, style);

		char _p[21];
		snprintf(_p, 20, "power: %4.0f", power_param->param_val->v.d);
		lv_label_set_text(label_power, _p);
	}

#endif

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT:

				//parameter is status
				if (gn_common_leaf_event_mask_param(&evt, status_param) == 0) {

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
								status_param->param_val->v.b ?
										"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
#endif
					//parameter is power
				} else if (gn_common_leaf_event_mask_param(&evt, power_param)
						== 0) {

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {
						char _p[21];
						snprintf(_p, 20, "power: %f",
								power_param->param_val->v.d);
						lv_label_set_text(label_power, _p);

						gn_display_leaf_refresh_end();
					}
#endif

				}

				break;

				//what to do when network is connected
			case GN_NETWORK_CONNECTED_EVENT:
				//gn_pump_state = GN_PUMP_STATE_RUNNING;
				break;

				//what to do when network is disconnected
			case GN_NETWORK_DISCONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_STOP;
				break;

				//what to do when server is connected
			case GN_SERVER_CONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_RUNNING;
				break;

				//what to do when server is disconnected
			case GN_SERVER_DISCONNECTED_EVENT:
				gn_pump_state = GN_PUMP_STATE_STOP;
				break;

			default:
				break;

			}

		}

		//finally, we update sensor using the parameter values
		if (gn_pump_state != GN_PUMP_STATE_RUNNING) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
		} else if (!status_param->param_val->v.b) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
			//change = false;
		} else {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A,
					power_param->param_val->v.d);
			//change = false;
		}

		vTaskDelay(1);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
