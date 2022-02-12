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

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
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

#include "ds18x20.h"
#include "gn_ds18b20.h"

#define TAG "gn_leaf_ds18b20"

const size_t GN_DS18B20_STATE_STOP = 0;
const size_t GN_DS18B20_STATE_RUNNING = 1;

void gn_ds18b20_task(gn_leaf_handle_t leaf_config);

void _scan_sensors(int gpio, size_t *sensor_count, ds18x20_addr_t *addrs) {

	esp_err_t res;
	//*sensor_count = 0;

	ESP_LOGD(TAG, "scan devices on gpio %d", gpio);

	res = ds18x20_scan_devices(gpio, addrs, GN_DS18B20_MAX_SENSORS,
			sensor_count);
	if (res != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR, "Sensors scan error %d (%s)", res, esp_err_to_name(res));
		return;
	}

	ESP_LOGD(TAG, "%d sensors detected. addr[0]=%llu, addr[1]=%llu, addr[2]=%llu, addr[3]=%llu",
			*sensor_count, addrs[0], addrs[1], addrs[2], addrs[3]);

	// If there were more sensors found than we have space to handle,
	// just report the first MAX_SENSORS..
	if (*sensor_count > GN_DS18B20_MAX_SENSORS) {
		*sensor_count = GN_DS18B20_MAX_SENSORS;
	}

}

typedef struct {

	size_t sensor_count;
	ds18x20_addr_t addrs[GN_DS18B20_MAX_SENSORS];
	float temp[GN_DS18B20_MAX_SENSORS];

	esp_timer_handle_t sensor_timer;

	gn_leaf_param_handle_t temp_param[GN_DS18B20_MAX_SENSORS];
	gn_leaf_param_handle_t update_time_param;
	gn_leaf_param_handle_t gpio_param;
	gn_leaf_param_handle_t active_param;

} gn_ds18b20_data_t;

gn_leaf_param_validator_result_t _gn_upd_time_sec_validator(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR_GENERIC;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_gn_upd_time_sec_validator - param: %d", (int )_p1);

	if (MIN_UPDATE_TIME_SEC > **(double**) param_value) {
		memcpy(param_value, &MIN_UPDATE_TIME_SEC, sizeof(MIN_UPDATE_TIME_SEC));
		return GN_LEAF_PARAM_VALIDATOR_ERROR_BELOW_MIN;
	} else if (MAX_UPDATE_TIME_SEC < **(double**) param_value) {
		memcpy(param_value, &MAX_UPDATE_TIME_SEC, sizeof(MAX_UPDATE_TIME_SEC));
		return GN_LEAF_PARAM_VALIDATOR_ERROR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

void gn_ds18b20_temp_sensor_collect(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_ds18b20_temp_sensor_collect", leaf_name);

	//struct leaf_data *data = (struct leaf_data*) arg;

	gn_ds18b20_data_t *data = (gn_ds18b20_data_t*) gn_leaf_get_descriptor(
			leaf_config)->data;

	esp_err_t res;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_DS18B20_PARAM_ACTIVE, &active);

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_DS18B20_PARAM_GPIO, &gpio);

	if (active == true) {

		ESP_LOGD(TAG, "[%s] reading from GPIO %d..", leaf_name, (int)gpio);

		//read data from sensors using GPIO parameter
		res = ds18x20_measure_and_read_multi(gpio, data->addrs,
				data->sensor_count, data->temp);

		if (res != ESP_OK) {
			gn_log(TAG, GN_LOG_ERROR, "[%s] sensors read error %d (%s)",
					leaf_name,
					res,
					esp_err_to_name(res));
			gn_leaf_get_descriptor(leaf_config)->status = GN_LEAF_STATUS_ERROR;
			gn_leaf_param_write_bool(leaf_config, GN_DS18B20_PARAM_ACTIVE,
			false);
			esp_timer_stop(data->sensor_timer);
			goto fail;
		}

		for (int j = 0; j < data->sensor_count; j++) {
			float temp_c = data->temp[j];
			float temp_f = (temp_c * 1.8) + 32;
			ESP_LOGD(TAG, "[%s] sensor %08x%08x (%s) reports %.3f �C (%.3f �F)",
					leaf_name,
					(uint32_t )(data->addrs[j] >> 32),
					(uint32_t )data->addrs[j],
					(data->addrs[j] & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20",
					temp_c, temp_f);

			//store parameter and notify network
			gn_leaf_param_write_double(leaf_config,
					GN_DS18B20_PARAM_SENSOR_NAMES[j], temp_c);
		}

		fail: return;

	}
}

gn_leaf_descriptor_handle_t gn_ds18b20_config(
		gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_ds18b20_config", leaf_name);

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_DS18B20_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_ds18b20_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	gn_ds18b20_data_t *data = malloc(sizeof(gn_ds18b20_data_t));

	data->sensor_count = 0;

	//parameter definition. if found in flash storage, they will be created with found values instead of default

	data->active_param = gn_leaf_param_create(leaf_config,
			GN_DS18B20_PARAM_ACTIVE, GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b =
					true }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->active_param);

	//get update time in sec, default 30
	data->update_time_param = gn_leaf_param_create(leaf_config,
			GN_DS18B20_PARAM_UPDATE_TIME_SEC, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 30 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, _gn_upd_time_sec_validator);
	gn_leaf_param_add_to_leaf(leaf_config, data->update_time_param);

	//get gpio from params. default 27
	data->gpio_param = gn_leaf_param_create(leaf_config, GN_DS18B20_PARAM_GPIO,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 27 },
			GN_LEAF_PARAM_ACCESS_NETWORK, GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->gpio_param);

	//get params for temp. init to 0
	for (int i = 0; i < GN_DS18B20_MAX_SENSORS; i++) {
		data->temp_param[i] = gn_leaf_param_create(leaf_config,
				GN_DS18B20_PARAM_SENSOR_NAMES[i], GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_NODE,
				GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
		gn_leaf_param_add_to_leaf(leaf_config, data->temp_param[i]);
	}

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_ds18b20_task(gn_leaf_handle_t leaf_config) {

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_ds18b20_data_t *data = descriptor->data;

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);


	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_DS18B20_PARAM_ACTIVE, &active);

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_DS18B20_PARAM_GPIO, &gpio);

	double update_time_sec;
	gn_leaf_param_get_double(leaf_config, GN_DS18B20_PARAM_UPDATE_TIME_SEC,
			&update_time_sec);


	ESP_LOGD(TAG, "[%s] gn_ds18b20_task", leaf_name);

	//init sensors

	//setup gpio
	//gpio_set_pull_mode(gpio, GPIO_PULLUP_ONLY);

	_scan_sensors(gpio, &data->sensor_count, &data->addrs[0]);

	//create a timer to update temps
	esp_timer_create_args_t sensor_timer_args =
			{ .callback = &gn_ds18b20_temp_sensor_collect, .arg = leaf_config, .name =
					"ds18b20_periodic" };

	esp_err_t ret = esp_timer_create(&sensor_timer_args, &data->sensor_timer);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR, "[%s] failed to init ds18b20 leaf timer", leaf_name);
		descriptor->status = GN_LEAF_STATUS_ERROR;
	}

	if (ret == ESP_OK && active == true) {

		ret = esp_timer_start_periodic(data->sensor_timer,
				update_time_sec * 1000000);
		if (ret != ESP_OK) {
			gn_log(TAG, GN_LOG_ERROR, "[%s] failed to start ds18b20 leaf timer", leaf_name);
			gn_leaf_get_descriptor(leaf_config)->status = GN_LEAF_STATUS_ERROR;
			gn_leaf_param_write_bool(data->active_param, GN_DS18B20_PARAM_ACTIVE,
			false);
			descriptor->status = GN_LEAF_STATUS_ERROR;
		}
	}

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	ESP_LOGD(TAG, "[%s] starting GUI...", leaf_name);

	lv_obj_t *label_temp_names[GN_DS18B20_MAX_SENSORS];
	lv_obj_t *label_temp[GN_DS18B20_MAX_SENSORS];
	lv_obj_t *label_title;
	//lv_obj_t *obj;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			//style from the container
			//lv_style_t *style = _cnt->styles->style;

			ESP_LOGD(TAG, "[%s] set layout", leaf_name);
			//lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			//obj = lv_obj_create(_cnt);
			//lv_obj_add_style(obj, style, 0);
			//lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			//lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 0, 2,
			//		LV_GRID_ALIGN_STRETCH, 0, 1);

			ESP_LOGD(TAG, "[%s] label_title", leaf_name);
			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title, leaf_name);
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, 0);
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

			ESP_LOGD(TAG, "[%s] actual values", leaf_name);
			for (int i = 0; i < data->sensor_count; i++) {

				//obj = lv_obj_create(_cnt);
				//lv_obj_add_style(obj, style, 0);
				//lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
				//lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 0, 1,
				//		LV_GRID_ALIGN_STRETCH, i+1, 1);

				label_temp_names[i] = lv_label_create(_cnt);
				//lv_obj_add_style(label_temp_names[i], style, 0);
				lv_label_set_text(label_temp_names[i],
						GN_DS18B20_PARAM_SENSOR_NAMES[i]);
				lv_obj_set_grid_cell(label_temp_names[i], LV_GRID_ALIGN_STRETCH,
						0, 1, LV_GRID_ALIGN_STRETCH, i + 1, 1);

				//lv_obj_align_to(label_temp_names[i], _cnt,
				//		LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25 * (i + 1));

				char _p[21];
				snprintf(_p, 20, ": %4.2f", 0.0);

				//obj = lv_obj_create(_cnt);
				//lv_obj_add_style(obj, style, 0);
				//lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
				//lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 1, 1,
				//		LV_GRID_ALIGN_STRETCH, i+1, 1);

				label_temp[i] = lv_label_create(_cnt);
				//lv_obj_add_style(label_temp[i], style, 0);
				lv_label_set_text(label_temp[i], _p);
				lv_obj_set_grid_cell(label_temp[i], LV_GRID_ALIGN_STRETCH, 1, 1,
						LV_GRID_ALIGN_STRETCH, i + 1, 1);

				//lv_obj_align_to(label_temp[i], label_temp_names[i],
				//		LV_ALIGN_RIGHT_MID, 0, 5);

			}

			ESP_LOGD(TAG, "[%s] end", leaf_name);
		}
		gn_display_leaf_refresh_end();
	}
#endif

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			//event arrived for this node
			switch (evt.id) {

			//what to do when network is connected
			case GN_NET_CONNECTED_EVENT:

				break;

				//what to do when network is disconnected
			case GN_NET_DISCONNECTED_EVENT:
				/*
				 if (data.gn_ds18b20_state != GN_DS18B20_STATE_STOP) {
				 data.gn_ds18b20_state = GN_DS18B20_STATE_STOP;
				 //stop update cycle
				 ESP_LOGD(TAG, "esp_timer_stop");
				 ESP_ERROR_CHECK(esp_timer_stop(temp_sensor_timer));
				 }
				 */
				break;

				//what to do when server is connected
			case GN_SRV_CONNECTED_EVENT:
				/*
				 if (data.gn_ds18b20_state != GN_DS18B20_STATE_RUNNING) {
				 data.gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
				 //start update cycle
				 ESP_LOGD(TAG, "esp_timer_start, frequency %f sec",
				 data.update_time_param->param_val->v.d);
				 ESP_ERROR_CHECK(
				 esp_timer_start_periodic(temp_sensor_timer,
				 data.update_time_param->param_val->v.d
				 * 1000000));

				 }
				 */
				break;

				//what to do when server is disconnected
			case GN_SRV_DISCONNECTED_EVENT:
				/*
				 if (data.gn_ds18b20_state != GN_DS18B20_STATE_STOP) {
				 data.gn_ds18b20_state = GN_DS18B20_STATE_STOP;
				 //stop update cycle
				 ESP_LOGD(TAG, "esp_timer_stop");
				 ESP_ERROR_CHECK(esp_timer_stop(temp_sensor_timer));
				 }
				 */
				break;

				//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "[%s] request to update param %s, data = '%s'",
						leaf_name, evt.param_name, evt.data);

				//parameter is update time
				if (gn_leaf_event_mask_param(&evt,
						data->update_time_param) == 0) {
					gn_leaf_param_write_double(leaf_config,
							GN_DS18B20_PARAM_UPDATE_TIME_SEC,
							(double) atof(evt.data));
					esp_timer_stop(data->sensor_timer);
					esp_timer_start_periodic(data->sensor_timer,
							update_time_sec * 1000000);

				} else if (gn_leaf_event_mask_param(&evt,
						data->active_param) == 0) {

					bool prev_active = active;
					int _active = atoi(evt.data);

					//execute change
					gn_leaf_param_write_bool(leaf_config, GN_DS18B20_PARAM_ACTIVE,
							_active == 0 ? false : true);
					active = _active;

					//stop timer if false
					if (_active == 0 && prev_active == true) {
						esp_timer_stop(data->sensor_timer);
					} else if (_active != 0 && prev_active == false) {
						esp_timer_start_periodic(data->sensor_timer,
								update_time_sec * 1000000);
					}

				} else if (gn_leaf_event_mask_param(&evt,
						data->gpio_param) == 0) {

					//check limits
					int _gpio = atoi(evt.data);
					if (_gpio >= 0 && gpio < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_write_double(leaf_config,
								GN_DS18B20_PARAM_GPIO, _gpio);
						gpio = _gpio;
					}
				}

				break;

			default:
				break;

			}

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
		//update GUI
		if (pdTRUE == gn_display_leaf_refresh_start()) {
			for (int i = 0; i < data->sensor_count; i++) {
				char _p[21];

				double temp;
				gn_leaf_param_get_double(leaf_config, GN_DS18B20_PARAM_SENSOR_NAMES[i], &temp);

				snprintf(_p, 20, "%4.2f", temp);
				lv_label_set_text(label_temp[i], _p);
			}
			gn_display_leaf_refresh_end();
		}
#endif

			vTaskDelay(1000 / portTICK_PERIOD_MS);

		}

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
