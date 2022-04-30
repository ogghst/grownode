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

typedef struct {
	gn_leaf_param_handle_t active_param;
	gn_leaf_param_handle_t gn_cwl_touch_channel_param;
	gn_leaf_param_handle_t max_level_param;
	gn_leaf_param_handle_t min_level_param;
	gn_leaf_param_handle_t act_level_param;
	gn_leaf_param_handle_t trg_hig_param;
	gn_leaf_param_handle_t trg_low_param;
	gn_leaf_param_handle_t upd_time_sec_param;

	esp_timer_handle_t sensor_timer;

} gn_cwl_data_t;

/*
 gn_leaf_param_validator_result_t gm_capacitive_water_level_validator(gn_leaf_param_handle_t param, void** param_value) {

 double MIN_WATERING_INTERVAL = 1140;
 double MAX_WATERING_INTERVAL = 1150;

 double val;
 if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
 return GN_LEAF_PARAM_VALIDATOR_ERROR;

 double _p1 = **(double**) param_value;
 ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int)_p1);

 if (MIN_WATERING_INTERVAL > **(double**)param_value) {
 *param_value = &MIN_WATERING_INTERVAL;
 return GN_LEAF_PARAM_VALIDATOR_BELOW_MIN;
 }
 else if (MAX_WATERING_INTERVAL < **(double**)param_value) {
 *param_value = &MAX_WATERING_INTERVAL;
 return GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX;
 }

 _p1 = **(double**) param_value;
 ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int)_p1);


 return GN_LEAF_PARAM_VALIDATOR_PASSED;

 }
 */

void gn_capacitive_water_level_task(gn_leaf_handle_t leaf_config);

void gn_cwl_sensor_collect(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_cwl_sensor_collect", leaf_name);

	//retrieves status descriptor from config
	//gn_cwl_data_t *data =
	//		(gn_cwl_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	double channel;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_TOUCH_CHANNEL, &channel);

	uint16_t read_value;
	touch_pad_clear_status();
	touch_pad_read_raw_data(channel, &read_value);
	//converting to double
	double result = read_value;
	//memcpy(&result, &read_value, sizeof(uint16_t));

	ESP_LOGD(TAG, "[%s] result: %f", leaf_name, result);

	//store parameter and notify network
	gn_leaf_param_force_double(leaf_config, GN_CWL_PARAM_ACT_LEVEL, result);

	double max_level;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_MAX_LEVEL, &max_level);

	double min_level;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_MIN_LEVEL, &min_level);

	bool trg_high;
	gn_leaf_param_get_bool(leaf_config, GN_CWL_PARAM_TRG_HIGH, &trg_high);

	bool trg_low;
	gn_leaf_param_get_bool(leaf_config, GN_CWL_PARAM_TRG_LOW, &trg_low);

	//if level is above maximum, trigger max
	if (result >= max_level && trg_high == false) {
		gn_leaf_param_force_bool(leaf_config, GN_CWL_PARAM_TRG_HIGH,
		true);
	} else

	//reset the trigger status if returning below max
	if (result < max_level && trg_high == true) {
		gn_leaf_param_force_bool(leaf_config, GN_CWL_PARAM_TRG_HIGH,
		false);
	} else

	//if level is below maximum, trigger min
	if (result <= min_level && trg_low == false) {
		gn_leaf_param_force_bool(leaf_config, GN_CWL_PARAM_TRG_LOW,
		true);
	} else

	//reset the trigger status if returning above min
	if (result > min_level && trg_low == true) {
		gn_leaf_param_force_bool(leaf_config, GN_CWL_PARAM_TRG_LOW,
		false);
	}

}

gn_leaf_descriptor_handle_t gn_capacitive_water_level_config(
		gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_capacitive_water_level_config", leaf_name);

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_CWL_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_capacitive_water_level_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_cwl_data_t *data = malloc(sizeof(gn_cwl_data_t));

	data->active_param = gn_leaf_param_create(leaf_config, GN_CWL_PARAM_ACTIVE,
			GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b = false },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->gn_cwl_touch_channel_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_TOUCH_CHANNEL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->max_level_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_MAX_LEVEL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 1000 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->min_level_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_MIN_LEVEL, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->act_level_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_ACT_LEVEL, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->trg_hig_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_TRG_HIGH, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->trg_low_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_TRG_LOW, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->upd_time_sec_param = gn_leaf_param_create(leaf_config,
			GN_CWL_PARAM_UPDATE_TIME_SEC, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							10 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	gn_leaf_param_add_to_leaf(leaf_config, data->active_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_cwl_touch_channel_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->max_level_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->min_level_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->act_level_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->trg_hig_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->trg_low_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->upd_time_sec_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_capacitive_water_level_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_cwl_data_t *data =
			(gn_cwl_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_CWL_PARAM_ACTIVE, &active);

	double touch_channel;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_TOUCH_CHANNEL,
			&touch_channel);

	double min_level;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_MIN_LEVEL, &min_level);

	bool trg_high;
	gn_leaf_param_get_bool(leaf_config, GN_CWL_PARAM_TRG_HIGH, &trg_high);

	bool trg_low;
	gn_leaf_param_get_bool(leaf_config, GN_CWL_PARAM_TRG_LOW, &trg_low);

	double max_level;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_MAX_LEVEL, &max_level);

	double update_time_sec;
	gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_UPDATE_TIME_SEC,
			&update_time_sec);

	//setup capacitive pin

	ESP_LOGD(TAG, "[%s] gn_capacitive_water_level_task", leaf_name);

	esp_err_t ret;

	ret = touch_pad_init();
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"[%s] failed to init capacitive water level sensor", leaf_name);
		return;
	}

	// Set reference voltage for charging/discharging
	// For most usage scenarios, we recommend using the following combination:
	// the high reference valtage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5,
			TOUCH_HVOLT_ATTEN_1V);

	touch_pad_config(touch_channel,
	GN_CWL_TOUCH_THRESH);

	// Initialize and start a software filter to detect slight change of capacitance.
	touch_pad_filter_start(GN_CWL_TOUCHPAD_FILTER_TOUCH_PERIOD);

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	ESP_LOGD(TAG, "[%s] starting GUI...", leaf_name);

	lv_obj_t *label_title = NULL;
	lv_obj_t *active_title = NULL;
	lv_obj_t *active_value = NULL;
	lv_obj_t *trg_title = NULL;
	lv_obj_t *trg_value = NULL;
	lv_obj_t *trg_act_title = NULL;
	lv_obj_t *trg_act_value = NULL;

	char _buf[21];

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			//style from the container
			//lv_style_t *style = _cnt->styles->style;

			ESP_LOGD(TAG, "[%s] set layout", leaf_name);
			//lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			ESP_LOGD(TAG, "[%s] label_title", leaf_name);
			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title, leaf_name);
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

			ESP_LOGD(TAG, "[%s] active_title", leaf_name);
			active_title = lv_label_create(_cnt);
			lv_label_set_text(active_title, "Trigger");
			//lv_obj_add_style(active_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(active_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "[%s] active_value", leaf_name);
			active_value = lv_label_create(_cnt);
			lv_label_set_text(active_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(active_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "[%s] trg_title", leaf_name);
			trg_title = lv_label_create(_cnt);
			lv_label_set_text(trg_title, "Trigger");
			//lv_obj_add_style(trg_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 2, 1);

			ESP_LOGD(TAG, "[%s] trg_value", leaf_name);
			trg_value = lv_label_create(_cnt);
			lv_label_set_text(trg_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 2, 1);

			ESP_LOGD(TAG, "[%s] trg_act_title", leaf_name);
			trg_act_title = lv_label_create(_cnt);
			lv_label_set_text(trg_act_title, "Value");
			//lv_obj_add_style(trg_act_title, style, 0);
			//lv_obj_align_to(trg_act_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_act_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 3, 1);

			snprintf(_buf, 20, "%4.2f", 0.0);

			ESP_LOGD(TAG, "[%s] trg_act_value", leaf_name);
			trg_act_value = lv_label_create(_cnt);
			lv_label_set_text(trg_act_value, _buf);
			//lv_obj_add_style(trg_act_value, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(trg_act_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 3, 1);

			ESP_LOGD(TAG, "[%s] end", leaf_name);

		}

		gn_display_leaf_refresh_end();

	}

#endif

	ESP_LOGD(TAG, "[%s] starting timer...", leaf_name);
	//create a timer to update temps
	const esp_timer_create_args_t water_sensor_timer_args = { .callback =
			&gn_cwl_sensor_collect, .arg = leaf_config, .name = "cwl_timer" };

	ret = esp_timer_create(&water_sensor_timer_args, &data->sensor_timer);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"[%s] failed to init capacitive water level timer", leaf_name);
	}

	if (ret == ESP_OK && active == true) {

		//start sensor callback
		ret = esp_timer_start_periodic(data->sensor_timer,
				update_time_sec * 1000000);
		if (ret != ESP_OK) {
			gn_log(TAG, GN_LOG_ERROR,
					"[%s] failed to start capacitive water level timer", leaf_name);
			gn_leaf_get_descriptor(leaf_config)->status = GN_LEAF_STATUS_ERROR;
			gn_leaf_param_force_bool(leaf_config, GN_CWL_PARAM_ACTIVE,
			false);
			descriptor->status = GN_LEAF_STATUS_ERROR;
		}

		ESP_LOGD(TAG, "[%s] listening to events...", leaf_name);

	}

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "[%s] request to update param %s, data = '%s'",
						leaf_name, evt.param_name, evt.data);

				//parameter is update time
				if (gn_leaf_event_mask_param(&evt, data->upd_time_sec_param)
						== 0) {

					//check limits
					double updtime = strtod(evt.data, NULL);
					if (updtime < 10)
						updtime = 10;
					if (updtime > 600)
						updtime = 600;

					update_time_sec = updtime;

					esp_timer_stop(data->sensor_timer);
					ret = esp_timer_start_periodic(data->sensor_timer,
							update_time_sec * 1000000);

					//execute change
					gn_leaf_param_force_double(leaf_config,
							GN_CWL_PARAM_UPDATE_TIME_SEC, updtime);

				} else if (gn_leaf_event_mask_param(&evt, data->active_param)
						== 0) {

					bool prev_active = active;
					int _active = atoi(evt.data);

					//execute change
					gn_leaf_param_force_bool(leaf_config, GN_CWL_PARAM_ACTIVE,
							_active == 0 ? false : true);

					active = _active;

					//stop timer if false
					if (_active == 0 && prev_active == true) {
						esp_timer_stop(data->sensor_timer);
					} else if (_active != 0 && prev_active == false) {
						esp_timer_start_periodic(data->sensor_timer,
								update_time_sec * 1000000);
					}

				}

				break;

			default:
				break;

			}

		}

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
		//update GUI
		if (pdTRUE == gn_display_leaf_refresh_start()) {

			char _buf[21];

			double act_level;
			gn_leaf_param_get_double(leaf_config, GN_CWL_PARAM_ACT_LEVEL,
					&act_level);

			snprintf(_buf, 20, "%4.2f", act_level);
			lv_label_set_text(trg_act_value, _buf);

			if (trg_high == true && trg_low == false)
				strncpy(_buf, "HIGH", 20);

			if (trg_high == false && trg_low == true)
				strncpy(_buf, "LOW", 20);

			if (trg_high == true && trg_low == true)
				strncpy(_buf, "ERR_H", 20);

			if (trg_high == false && trg_low == false)
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
