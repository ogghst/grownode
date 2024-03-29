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

#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_pwm.h"

#include "math.h"

#define TAG "gn_leaf_pwm"

static const ledc_mode_t GN_LEAF_PWM_PARAM_LEDC_MODE = LEDC_HIGH_SPEED_MODE;

#define GN_LEAF_PWM_FADE /*!< define if duty has to be faded */
#define GN_LEAF_PWM_FADE_SPEED 500 /*!< define fade speed (msec) */

#define GN_LEAF_PWM_UNKNOWN_CHANNEL	-1
#define GN_LEAF_PWM_UNKNOWN_POWER	-1
//#define GN_LEAF_PWM_UNKNOWN_FREQ	-1
//#define GN_LEAF_PWM_UNKNOWN_RES		-1
#define GN_LEAF_PWM_UNKNOWN_GPIO	-1

typedef struct {

	gn_leaf_param_handle_t toggle_param;
	gn_leaf_param_handle_t power_param;
	gn_leaf_param_handle_t channel_param;
//	gn_leaf_param_handle_t freq_param;
//	gn_leaf_param_handle_t res_param;
	gn_leaf_param_handle_t gpio_param;

} gn_pump_hs_data_t;

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
#ifdef GN_LEAF_PWM_FADE
static bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg) {
	portBASE_TYPE taskAwoken = pdFALSE;

	if (param->event == LEDC_FADE_END_EVT) {
		SemaphoreHandle_t counting_sem = (SemaphoreHandle_t) user_arg;
		xSemaphoreGiveFromISR(counting_sem, &taskAwoken);
	}

	return (taskAwoken == pdTRUE);
}
#endif

void gn_leaf_pwm_task(gn_leaf_handle_t leaf_config);

gn_leaf_descriptor_handle_t gn_leaf_pwm_config(
		gn_leaf_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));

	strncpy(descriptor->type, GN_LEAF_PWM_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_leaf_pwm_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	gn_pump_hs_data_t *data = malloc(sizeof(gn_pump_hs_data_t));

	//parameter definition. if found in flash storage, they will be created with found values instead of default
	data->toggle_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_PWM_PARAM_TOGGLE, GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b =
					false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->toggle_param);

	data->power_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_PWM_PARAM_POWER, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
					GN_LEAF_PWM_UNKNOWN_POWER }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->power_param);

	data->channel_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_PWM_PARAM_CHANNEL, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
					GN_LEAF_PWM_UNKNOWN_CHANNEL }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->channel_param);

	/*
	 data->freq_param = gn_leaf_param_create(leaf_config, GN_LEAF_PWM_PARAM_FREQ,
	 GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
	 GN_LEAF_PWM_UNKNOWN_FREQ }, GN_LEAF_PARAM_ACCESS_READWRITE,
	 GN_LEAF_PARAM_STORAGE_PERSISTED,
	 NULL);
	 gn_leaf_param_add_to_leaf(leaf_config, data->freq_param);

	 data->res_param = gn_leaf_param_create(leaf_config, GN_LEAF_PWM_PARAM_RES,
	 GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
	 GN_LEAF_PWM_UNKNOWN_RES }, GN_LEAF_PARAM_ACCESS_READWRITE,
	 GN_LEAF_PARAM_STORAGE_PERSISTED,
	 NULL);
	 gn_leaf_param_add_to_leaf(leaf_config, data->res_param);
	 */

	data->gpio_param = gn_leaf_param_create(leaf_config, GN_LEAF_PWM_PARAM_GPIO,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
					GN_LEAF_PWM_UNKNOWN_GPIO }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gpio_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_leaf_pwm_task(gn_leaf_handle_t leaf_config) {

	bool need_update = true;

	gn_leaf_parameter_event_t evt;

	esp_err_t ret = ESP_OK;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_pump_hs_data_t *data = descriptor->data;

	bool toggle;
	gn_leaf_param_get_bool(leaf_config, GN_LEAF_PWM_PARAM_TOGGLE, &toggle);

	double power;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_PWM_PARAM_POWER, &power);

	double channel;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_PWM_PARAM_CHANNEL, &channel);

	/*
	 double freq;
	 gn_leaf_param_get_double(leaf_config, GN_LEAF_PWM_PARAM_FREQ, &freq);

	 double res;
	 gn_leaf_param_get_double(leaf_config, GN_LEAF_PWM_PARAM_RES, &res);
	 */

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_PWM_PARAM_GPIO, &gpio);

	//setup ledc pwm

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "%s - setting pwm pin %d channel %d", leaf_name, (int )gpio,
			(int )channel);

#ifdef GN_LEAF_PWM_FADE
	ESP_LOGD(TAG, "%s - xSemaphoreCreateBinary", leaf_name);
	SemaphoreHandle_t fade_sem = xSemaphoreCreateBinary();
#endif

	ledc_timer_config_t timer;
	timer.speed_mode = GN_LEAF_PWM_PARAM_LEDC_MODE;
	timer.timer_num = LEDC_TIMER_0;
	timer.duty_resolution = LEDC_TIMER_13_BIT;
	timer.freq_hz = 5000;
	timer.clk_cfg = LEDC_AUTO_CLK;

	ESP_LOGD(TAG, "%s - ledc_timer_config", leaf_name);

	ret = ledc_timer_config(&timer);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "error in configuring timer config");
		descriptor->status = GN_LEAF_STATUS_ERROR;
	} else {

		ledc_channel_config_t ledc_channel;
		ledc_channel.speed_mode = GN_LEAF_PWM_PARAM_LEDC_MODE;
		ledc_channel.channel = (ledc_channel_t) channel;
		ledc_channel.timer_sel = (ledc_timer_t) LEDC_TIMER_0;
		ledc_channel.intr_type = LEDC_INTR_DISABLE;
		ledc_channel.gpio_num = (int) gpio;
		ledc_channel.duty = 0;
		ledc_channel.hpoint = 0;

		ESP_LOGD(TAG, "%s - ledc_channel_config", leaf_name);

		ret = ledc_channel_config(&ledc_channel);

#ifdef GN_LEAF_PWM_FADE
		if (ret == ESP_OK) {

			ESP_LOGD(TAG, "%s - ledc_fade_func_install", leaf_name);
			ledc_fade_func_install(0);

			ledc_cbs_t callbacks = { .fade_cb = cb_ledc_fade_end_event };

			ESP_LOGD(TAG, "%s - ledc_cb_register", leaf_name);
			ledc_cb_register(ledc_channel.speed_mode, ledc_channel.channel,
					&callbacks, (void*) fade_sem);
		}
#endif

		if (ret != ESP_OK) {
			ESP_LOGE(TAG,
					"error in configuring ledc_channel, channel %d, code %d",
					(int )channel, ret);
			descriptor->status = GN_LEAF_STATUS_ERROR;
		} else {
			ESP_LOGD(TAG, "%s - pwm initialized", leaf_name);
		}
	}

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
					toggle ? "status: on" : "status: off");
			//lv_obj_align_to(label_status, label_title, LV_ALIGN_BOTTOM_LEFT,
			//		LV_PCT(10), LV_PCT(10));
			lv_obj_set_grid_cell(label_status, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 2);

			label_power = lv_label_create(_cnt);
			lv_obj_add_style(label_power, style, 0);

			char _p[21];
			snprintf(_p, 20, "power: %4.0f", power);
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

				if (gn_leaf_event_mask_param(&evt, data->toggle_param) == 0) {

					ESP_LOGD(TAG, "updating toggle");

					bool _toggle = false;
					if (gn_event_payload_to_bool(evt, &_toggle) != GN_RET_OK) {
						break;
					}

					//execute change
					gn_leaf_param_force_bool(leaf_config,
							GN_LEAF_PWM_PARAM_TOGGLE, _toggle);
					toggle = _toggle;

					need_update = true;

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
								toggle ?
										"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
#endif

				} else if (gn_leaf_event_mask_param(&evt, data->power_param)
						== 0) {

					ESP_LOGD(TAG, "updating power");

					double pow = strtod(evt.data, NULL);
					if (pow < 0)
						pow = 0;
					if (pow > 100)
						pow = 100;

					//execute change
					gn_leaf_param_force_double(leaf_config,
							GN_LEAF_PWM_PARAM_POWER, pow);

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

			ESP_LOGD(TAG, "updating values");

			//finally, we update sensor using the parameter values
			gn_leaf_param_get_bool(leaf_config, GN_LEAF_PWM_PARAM_TOGGLE,
					&toggle);
			gn_leaf_param_get_double(leaf_config, GN_LEAF_PWM_PARAM_POWER,
					&power);

			if (toggle == false) {

#ifdef GN_LEAF_PWM_FADE
				ret = ledc_set_fade_with_time(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, 0, GN_LEAF_PWM_FADE_SPEED);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in  changing duty, channel %d",
							(int )channel);
					goto fail;
				}

				ret = ledc_fade_start(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, LEDC_FADE_NO_WAIT);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in updating duty, channel %d",
							(int )channel);
					goto fail;
				}

#else
				ret = ledc_set_duty(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, 0);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in changing duty, channel %d",
							(int )channel);
					goto fail;
				}

				ret = ledc_update_duty(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in updating duty, channel %d",
							(int )channel);
					goto fail;
				}
#endif

#ifdef GN_LEAF_PWM_FADE
				xSemaphoreTake(fade_sem, portMAX_DELAY);
#endif

				ESP_LOGD(TAG,
						"%s - setting power pin %d - channel %d to %d - duty 0",
						leaf_name, (int )gpio, (int ) channel, (int )power);

				need_update = false;

			} else {

				double duty = (pow(2, 13) - 1) * (power / 100); // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095

#ifdef GN_LEAF_PWM_FADE
				ret = ledc_set_fade_with_time(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, duty, GN_LEAF_PWM_FADE_SPEED);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in  changing power, channel %d",
							(int )channel);
					goto fail;
				}

				ret = ledc_fade_start(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, LEDC_FADE_NO_WAIT);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in updating duty, channel %d",
							(int )channel);
					goto fail;
				}

#else
				ret = ledc_set_duty(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, duty);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in  changing power, channel %d",
							(int )channel);
					goto fail;
				}

				ret = ledc_update_duty(GN_LEAF_PWM_PARAM_LEDC_MODE,
						(ledc_channel_t) channel);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in updating duty, channel %d",
							(int )channel);
					goto fail;
				}
#endif

				ESP_LOGD(TAG,
						"%s - setting power pin %d - channel %d to %d - duty %f",
						leaf_name, (int )gpio, (int ) channel, (int )power,
						duty);

#ifdef GN_LEAF_PWM_FADE
				xSemaphoreTake(fade_sem, portMAX_DELAY);
#endif
				need_update = false;
			}

			fail:

//wait for next cycle
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	}

//fallback cycle
	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
