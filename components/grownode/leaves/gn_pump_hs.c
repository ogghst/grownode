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

#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_pump_hs.h"

#include "math.h"

#define TAG "gn_leaf_pump_hs"

static const ledc_mode_t GN_PUMP_HS_PARAM_LEDC_MODE = LEDC_LOW_SPEED_MODE;

//#define GN_PUMP_HS_FADE /*!< define if duty has to be faded */
#define GN_PUMP_HS_FADE_SPEED 500 /*!< define fade speed (msec) */

typedef struct {

	gn_leaf_param_handle_t channel_param;
	gn_leaf_param_handle_t toggle_param;
	gn_leaf_param_handle_t gpio_toggle_param;
	gn_leaf_param_handle_t power_param;
	gn_leaf_param_handle_t gpio_power_param;

} gn_pump_hs_data_t;

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
static bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg) {
	portBASE_TYPE taskAwoken = pdFALSE;

	if (param->event == LEDC_FADE_END_EVT) {
		SemaphoreHandle_t counting_sem = (SemaphoreHandle_t) user_arg;
		xSemaphoreGiveFromISR(counting_sem, &taskAwoken);
	}

	return (taskAwoken == pdTRUE);
}

void gn_pump_hs_task(gn_leaf_config_handle_t leaf_config);

gn_leaf_descriptor_handle_t gn_pump_hs_config(
		gn_leaf_config_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_PUMP_HS_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_pump_hs_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	gn_pump_hs_data_t *data = malloc(sizeof(gn_pump_hs_data_t));

	//parameter definition. if found in flash storage, they will be created with found values instead of default
	data->toggle_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_HS_PARAM_TOGGLE, GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b =
					false }, GN_LEAF_PARAM_ACCESS_READWRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, data->toggle_param);

	data->channel_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_HS_PARAM_CHANNEL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_READWRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, data->channel_param);

	data->gpio_toggle_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_HS_PARAM_GPIO_TOGGLE, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							32 }, GN_LEAF_PARAM_ACCESS_WRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, data->gpio_toggle_param);

	data->power_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_HS_PARAM_POWER, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_READWRITE, GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, data->power_param);

	data->gpio_power_param = gn_leaf_param_create(leaf_config,
			GN_PUMP_HS_PARAM_GPIO_POWER, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							32 }, GN_LEAF_PARAM_ACCESS_WRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, data->gpio_power_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_pump_hs_task(gn_leaf_config_handle_t leaf_config) {

	bool need_update = true;

	gn_leaf_parameter_event_t evt;

	esp_err_t ret = ESP_OK;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_pump_hs_data_t *data = descriptor->data;

	double gpio_toggle;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_HS_PARAM_GPIO_TOGGLE,
			&gpio_toggle);

	double gpio_power;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_HS_PARAM_GPIO_POWER,
			&gpio_power);

	double power;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_HS_PARAM_POWER, &power);

	bool toggle;
	gn_leaf_param_get_bool(leaf_config, GN_PUMP_HS_PARAM_TOGGLE, &toggle);

	double channel;
	gn_leaf_param_get_double(leaf_config, GN_PUMP_HS_PARAM_CHANNEL, &channel);

	//setup toggle
	gpio_set_direction(gpio_toggle, GPIO_MODE_OUTPUT);
	gpio_set_level(gpio_toggle, toggle ? 1 : 0);

	//setup ledc pwm

	ESP_LOGD(TAG, "%s - setting pwm pin %d channel %d",
			gn_leaf_get_config_name(leaf_config), (int )gpio_power,
			(int )channel);

#ifdef GN_PUMP_HS_FADE
	SemaphoreHandle_t fade_sem = xSemaphoreCreateBinary();
#endif

	ledc_timer_config_t timer;
	timer.speed_mode = GN_PUMP_HS_PARAM_LEDC_MODE;
	timer.timer_num = (ledc_timer_t) channel;
	timer.duty_resolution = LEDC_TIMER_13_BIT;
	timer.freq_hz = 5000;
	timer.clk_cfg = LEDC_AUTO_CLK;

	ret = ledc_timer_config(&timer);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "error in configuring timer config, channel %d",
				(int )channel);
		descriptor->status = GN_LEAF_STATUS_ERROR;
	} else {

		ledc_channel_config_t ledc_channel;
		ledc_channel.speed_mode = GN_PUMP_HS_PARAM_LEDC_MODE;
		ledc_channel.channel = (ledc_channel_t) channel;
		ledc_channel.timer_sel = (ledc_timer_t) channel;
		ledc_channel.intr_type = LEDC_INTR_DISABLE;
		ledc_channel.gpio_num = (int) gpio_power;
		ledc_channel.duty = 0;
		ledc_channel.hpoint = 0;

		ret = ledc_channel_config(&ledc_channel);

#ifdef GN_PUMP_HS_FADE
		if (ret == ESP_OK) {
			ret = ledc_fade_func_install(0);
			ledc_cbs_t callbacks = { .fade_cb = cb_ledc_fade_end_event };

			ledc_cb_register(ledc_channel.speed_mode, ledc_channel.channel,
					&callbacks, (void*) fade_sem);
		}
#endif

		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "error in configuring timer config, channel %d",
					(int )channel);
			descriptor->status = GN_LEAF_STATUS_ERROR;
		} else {
			ESP_LOGD(TAG, "%s - pwm initialized",
					gn_leaf_get_config_name(leaf_config));
		}
	}

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_status = NULL;
	lv_obj_t *label_power = NULL;
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);

		if (_cnt) {

			//style from the container
			lv_style_t *style = _cnt->styles->style;

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

				//parameter is status
				if (gn_common_leaf_event_mask_param(&evt, data->toggle_param)
						== 0) {

					ESP_LOGD(TAG, "updating toggle");

					const bool _toggle =
							strncmp((char*) evt.data, "0", evt.data_size) == 0 ?
									false : true;

					//execute change
					gn_leaf_param_set_bool(leaf_config, GN_PUMP_HS_PARAM_TOGGLE,
							_toggle);
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

				} else if (gn_common_leaf_event_mask_param(&evt,
						data->gpio_toggle_param) == 0) {

					ESP_LOGD(TAG, "updating gpio toggle");

					//check limits
					int gpio = atoi(evt.data);
					if (gpio >= 0 && gpio < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_set_double(leaf_config,
								GN_PUMP_HS_PARAM_GPIO_TOGGLE, gpio);
					}

				} else if (gn_common_leaf_event_mask_param(&evt,
						data->power_param) == 0) {

					ESP_LOGD(TAG, "updating power");

					double pow = strtod(evt.data, NULL);
					if (pow < 0)
						pow = 0;
					if (pow > 100)
						pow = 100;

					//execute change
					gn_leaf_param_set_double(leaf_config,
							GN_PUMP_HS_PARAM_POWER, pow);

					need_update = true;

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {
						char _p[21];
						snprintf(_p, 20, "power: %f", pow);
						lv_label_set_text(label_power, _p);

						gn_display_leaf_refresh_end();
					}
#endif

				} else if (gn_common_leaf_event_mask_param(&evt,
						data->gpio_power_param) == 0) {

					ESP_LOGD(TAG, "updating gpio power");

					//check limits
					int gpio = atoi(evt.data);
					if (gpio >= 0 && gpio < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_set_double(leaf_config,
								GN_PUMP_HS_PARAM_GPIO_POWER, gpio);
					}

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
			gn_leaf_param_get_bool(leaf_config, GN_PUMP_HS_PARAM_TOGGLE,
					&toggle);
			gn_leaf_param_get_double(leaf_config, GN_PUMP_HS_PARAM_POWER,
					&power);

			if (!toggle) {

#ifdef GN_PUMP_HS_FADE
				ret = ledc_set_fade_with_time(GN_PUMP_HS_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, 0, GN_PUMP_HS_FADE_SPEED);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in  changing power, channel %d",
							(int )channel);
					goto fail;
				}

				ret = ledc_fade_start(GN_PUMP_HS_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, LEDC_FADE_NO_WAIT);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in updating duty, channel %d",
							(int )channel);
					goto fail;
				}

#else
		ret = ledc_set_duty(GN_PUMP_HS_PARAM_LEDC_MODE,
				(ledc_channel_t) channel, 0);

		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "error in changing power, channel %d",
					(int )channel);
			goto fail;
		}

		ret = ledc_update_duty(GN_PUMP_HS_PARAM_LEDC_MODE,
				(ledc_channel_t) channel);

		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "error in updating duty, channel %d",
					(int )channel);
			goto fail;
		}
#endif

#ifdef GN_PUMP_HS_FADE
				xSemaphoreTake(fade_sem, portMAX_DELAY);
#endif

				ret = gpio_set_level(toggle, 0);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in disabling signal, channel %d",
							(int )channel);
					goto fail;
				}

				ESP_LOGD(TAG, "%s - toggle off, channel %d",
						gn_leaf_get_config_name(leaf_config), (int )channel);

				need_update = false;

			} else {

				ret = gpio_set_level(gpio_toggle, 1);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in setting signal, channel %d",
							(int )channel);
					goto fail;
				}

				double duty = (pow(2, 13) - 1) * (power / 100); // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095

#ifdef GN_PUMP_HS_FADE
				ret = ledc_set_fade_with_time(GN_PUMP_HS_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, duty, GN_PUMP_HS_FADE_SPEED);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in  changing power, channel %d",
							(int )channel);
					goto fail;
				}

				ret = ledc_fade_start(GN_PUMP_HS_PARAM_LEDC_MODE,
						(ledc_channel_t) channel, LEDC_FADE_NO_WAIT);

				if (ret != ESP_OK) {
					ESP_LOGE(TAG, "error in updating duty, channel %d",
							(int )channel);
					goto fail;
				}

#else
	ret = ledc_set_duty(GN_PUMP_HS_PARAM_LEDC_MODE,
			(ledc_channel_t) channel, duty);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "error in  changing power, channel %d",
				(int )channel);
		goto fail;
	}

	ret = ledc_update_duty(GN_PUMP_HS_PARAM_LEDC_MODE,
			(ledc_channel_t) channel);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "error in updating duty, channel %d",
				(int )channel);
		goto fail;
	}
#endif

				ESP_LOGD(TAG,
						"%s - setting power pin %d - channel %d to %d - duty %f",
						gn_leaf_get_config_name(leaf_config), (int )gpio_power,
						(int ) channel, (int )power, duty);

#ifdef GN_PUMP_HS_FADE
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
