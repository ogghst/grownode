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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

#include "gn_capacitive_water_level.h"

#define TAG "gn_leaf_cwl"

//#define GN_CWL_TOUCH_CHANNEL   (1)			//GPIO4
#define GN_CWL_TOUCH_THRESH   (0)
#define GN_CWL_TOUCHPAD_FILTER_TOUCH_PERIOD (10)

struct leaf_data {
	gn_leaf_config_handle_t leaf_config;
	gn_leaf_param_handle_t gn_cwl_touch_channel;
	gn_leaf_param_handle_t max_level_param;
	gn_leaf_param_handle_t min_level_param;
	gn_leaf_param_handle_t act_level_param;
	gn_leaf_param_handle_t trg_hig_param;
	gn_leaf_param_handle_t trg_low_param;
	gn_leaf_param_handle_t upd_time_sec_param;

};

void gn_cwl_sensor_collect(struct leaf_data *data) {

	ESP_LOGD(TAG, "gn_cwl_sensor_collect");

	uint16_t read_value;
	touch_pad_clear_status();
	touch_pad_read_raw_data(data->gn_cwl_touch_channel->param_val->v.d,
			&read_value);
	//converting to double
	double result = read_value;
	//memcpy(&result, &read_value, sizeof(uint16_t));

	//store parameter and notify network
	gn_leaf_param_set_double(data->leaf_config, data->act_level_param->name,
			result);

	//if level is above maximum, trigger max
	if (result
			>= data->max_level_param->param_val->v.d&& data->trg_hig_param->param_val->v.b == false) {
		gn_leaf_param_set_bool(data->leaf_config, data->trg_hig_param->name,
		true);
	}

	//reset the trigger status if returning below max
	if (result
			< data->max_level_param->param_val->v.d&& data->trg_hig_param->param_val->v.b == true) {
		gn_leaf_param_set_bool(data->leaf_config, data->trg_hig_param->name,
		false);
	}

	//if level is below maximum, trigger min
	if (result
			<= data->min_level_param->param_val->v.d&& data->trg_low_param->param_val->v.b == false) {
		gn_leaf_param_set_bool(data->leaf_config, data->trg_low_param->name,
		true);
	}

	//reset the trigger status if returning above min
	if (result
			> data->min_level_param->param_val->v.d&& data->trg_low_param->param_val->v.b == true) {
		gn_leaf_param_set_bool(data->leaf_config, data->trg_low_param->name,
		false);
	}

}

void gn_capacitive_water_level_task(gn_leaf_config_handle_t leaf_config) {

	gn_leaf_param_handle_t gn_cwl_touch_channel = gn_leaf_param_create(
			leaf_config, GN_CWL_PARAM_TOUCH_CHANNEL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_READWRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, gn_cwl_touch_channel);

	gn_leaf_param_handle_t max_level_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_MAX_LEVEL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 1000 }, GN_LEAF_PARAM_ACCESS_READWRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, max_level_param);

	gn_leaf_param_handle_t min_level_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_MIN_LEVEL, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_READWRITE, GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, min_level_param);

	gn_leaf_param_handle_t act_level_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_ACT_LEVEL, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_READ, GN_LEAF_PARAM_STORAGE_VOLATILE);
	gn_leaf_param_add(leaf_config, act_level_param);

	gn_leaf_param_handle_t trg_hig_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_TRG_HIGH, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_READ,
			GN_LEAF_PARAM_STORAGE_VOLATILE);
	gn_leaf_param_add(leaf_config, trg_hig_param);

	gn_leaf_param_handle_t trg_low_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_TRG_LOW, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_READ,
			GN_LEAF_PARAM_STORAGE_VOLATILE);
	gn_leaf_param_add(leaf_config, trg_low_param);

	gn_leaf_param_handle_t upd_time_sec_param = gn_leaf_param_create(
			leaf_config, GN_CWL_PARAM_UPDATE_TIME_SEC, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 10 }, GN_LEAF_PARAM_ACCESS_READWRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED);
	gn_leaf_param_add(leaf_config, upd_time_sec_param);

	//fill up transfer structure
	struct leaf_data data;
	data.leaf_config = leaf_config;
	data.gn_cwl_touch_channel = gn_cwl_touch_channel;
	data.act_level_param = act_level_param;
	data.max_level_param = max_level_param;
	data.min_level_param = min_level_param;
	data.trg_hig_param = trg_hig_param;
	data.trg_low_param = trg_low_param;
	data.upd_time_sec_param = upd_time_sec_param;

	gn_leaf_event_t evt;

	//setup capacitive pin
	ESP_LOGD(TAG, "Initializing capactivite water level sensor..");

	esp_err_t ret;

	ret = touch_pad_init();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to init capacitive water level sensor");
		return;
	}

	// Set reference voltage for charging/discharging
	// For most usage scenarios, we recommend using the following combination:
	// the high reference valtage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5,
			TOUCH_HVOLT_ATTEN_1V);

	touch_pad_config(gn_cwl_touch_channel->param_val->v.d, GN_CWL_TOUCH_THRESH);

	// Initialize and start a software filter to detect slight change of capacitance.
	touch_pad_filter_start(GN_CWL_TOUCHPAD_FILTER_TOUCH_PERIOD);

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	ESP_LOGD(TAG, "Starting GUI..");

	lv_obj_t *label_title = NULL;
	lv_obj_t *trg_title = NULL;
	lv_obj_t *trg_value = NULL;
	lv_obj_t *trg_act_title = NULL;
	lv_obj_t *trg_act_value = NULL;

	char _buf[21];

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);

		if (_cnt) {

			//style from the container
			lv_style_t *style = _cnt->styles->style;

			ESP_LOGD(TAG, "Set Layout");
			//lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			ESP_LOGD(TAG, "label_title");
			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title,
					gn_leaf_get_config_name(leaf_config));
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

			ESP_LOGD(TAG, "trg_title");
			trg_title = lv_label_create(_cnt);
			lv_label_set_text(trg_title, "Trigger");
			//lv_obj_add_style(trg_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "trg_value");
			trg_value = lv_label_create(_cnt);
			lv_label_set_text(trg_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "trg_act_title");
			trg_act_title = lv_label_create(_cnt);
			lv_label_set_text(trg_act_title, "Value");
			//lv_obj_add_style(trg_act_title, style, 0);
			//lv_obj_align_to(trg_act_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_act_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 2, 1);

			snprintf(_buf, 20, "%4.2f", data.act_level_param->param_val->v.d);

			ESP_LOGD(TAG, "trg_act_value");
			trg_act_value = lv_label_create(_cnt);
			lv_label_set_text(trg_act_value, _buf);
			//lv_obj_add_style(trg_act_value, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_act_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 2, 1);

			ESP_LOGD(TAG, "end");

		}

		gn_display_leaf_refresh_end();

	}

#endif

	ESP_LOGD(TAG, "Starting timer..");
	//create a timer to update temps
	esp_timer_handle_t water_sensor_timer;
	const esp_timer_create_args_t water_sensor_timer_args = { .callback =
			&gn_cwl_sensor_collect, .arg = &data, .name = "cwl_timer" };

	ret = esp_timer_create(&water_sensor_timer_args, &water_sensor_timer);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to init capacitive water level timer");
		return;
	}
	//start sensor callback
	ret = esp_timer_start_periodic(water_sensor_timer,
					data.upd_time_sec_param->param_val->v.d * 1000000);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to start capacitive water level timer");
		return;
	}

	ESP_LOGD(TAG, "Listening to events..");

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			ESP_LOGD(TAG, "received message: %d", evt.id);

		}

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
		//update GUI
		if (pdTRUE == gn_display_leaf_refresh_start()) {

			char _buf[21];
			snprintf(_buf, 20, "%4.2f", data.act_level_param->param_val->v.d);
			lv_label_set_text(trg_act_value, _buf);

			if (data.trg_hig_param->param_val->v.b == true
					&& data.trg_low_param->param_val->v.b == false)
				strncpy(_buf, "HIGH", 20);

			if (data.trg_hig_param->param_val->v.b == false
					&& data.trg_low_param->param_val->v.b == true)
				strncpy(_buf, "LOW", 20);

			if (data.trg_low_param->param_val->v.b == true
					&& data.trg_low_param->param_val->v.b == true)
				strncpy(_buf, "ERR", 20);

			if (data.trg_low_param->param_val->v.b == false
					&& data.trg_low_param->param_val->v.b == false)
				strncpy(_buf, "OK", 20);

			lv_label_set_text(trg_value, _buf);

			gn_display_leaf_refresh_end();
		}
#endif

		vTaskDelay(1000 / portTICK_PERIOD_MS);

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
