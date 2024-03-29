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

//switch from LOGI to LOGD
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
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

#include "bmp280.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_bme280.h"

#define TAG "gn_leaf_bme280"

typedef struct {

	bmp280_params_t params;
	bmp280_t dev;

	esp_timer_handle_t bme280_sensor_timer;

	gn_leaf_param_handle_t sda_param;
	gn_leaf_param_handle_t scl_param;
	gn_leaf_param_handle_t update_time_param;
	gn_leaf_param_handle_t active_param;
	gn_leaf_param_handle_t temp_param;
	gn_leaf_param_handle_t press_param;
	gn_leaf_param_handle_t hum_param;

} gn_bme280_data_t;

void bme280_sensor_collect(gn_leaf_handle_t leaf_config) {

	ESP_LOGD(TAG, "bme280_sensor_collect");

	gn_bme280_data_t *data = (gn_bme280_data_t*) gn_leaf_get_descriptor(
			leaf_config)->data;

	//struct leaf_data *data = (struct leaf_data*) _data;

	ESP_LOGD(TAG, "bme280_sensor_collect");

	//struct leaf_data *data = (struct leaf_data*) arg;

	float pressure, temperature, humidity;

	esp_err_t res = bmp280_read_float(&data->dev, &temperature, &pressure,
			&humidity);

	//read data from sensors using GPIO parameter
	if (res != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR, "Sensors read error %d (%s)", res,
				esp_err_to_name(res));
		goto fail;
	}

	ESP_LOGD(TAG, "Pressure: %.2f Pa, Temperature: %.2f C, Humidity: %.2f\n",
			pressure, temperature, humidity);

	//store parameter and notify network
	gn_leaf_param_force_double(leaf_config, GN_BME280_PARAM_TEMP, temperature);
	gn_leaf_param_force_double(leaf_config, GN_BME280_PARAM_HUM, humidity);
	gn_leaf_param_force_double(leaf_config, GN_BME280_PARAM_PRESS, pressure);

	fail: return;

}

void gn_bme280_task(gn_leaf_handle_t leaf_config);

gn_leaf_descriptor_handle_t gn_bme280_config(gn_leaf_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_BME280_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_bme280_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	gn_bme280_data_t *data = malloc(sizeof(gn_bme280_data_t));

	//parameter definition. if found in flash storage, they will be created with found values instead of default

	data->active_param = gn_leaf_param_create(leaf_config,
			GN_BME280_PARAM_ACTIVE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = true }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->active_param);

	data->sda_param = gn_leaf_param_create(leaf_config, GN_BME280_PARAM_SDA,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 21 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->sda_param);

	data->scl_param = gn_leaf_param_create(leaf_config, GN_BME280_PARAM_SCL,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 22 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->scl_param);

	data->update_time_param = gn_leaf_param_create(leaf_config,
			GN_BME280_PARAM_UPDATE_TIME_SEC, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 120 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->update_time_param);

	data->temp_param = gn_leaf_param_create(leaf_config, GN_BME280_PARAM_TEMP,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->temp_param);

	data->hum_param = gn_leaf_param_create(leaf_config, GN_BME280_PARAM_HUM,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->hum_param);

	data->press_param = gn_leaf_param_create(leaf_config, GN_BME280_PARAM_PRESS,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->press_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_bme280_task(gn_leaf_handle_t leaf_config) {

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_bme280_data_t *data = descriptor->data;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_BME280_PARAM_ACTIVE, &active);
	double sda;
	gn_leaf_param_get_double(leaf_config, GN_BME280_PARAM_SDA, &sda);
	double scl;
	gn_leaf_param_get_double(leaf_config, GN_BME280_PARAM_SCL, &scl);
	double update_time;
	gn_leaf_param_get_double(leaf_config, GN_BME280_PARAM_UPDATE_TIME_SEC,
			&update_time);
	double temp;
	gn_leaf_param_get_double(leaf_config, GN_BME280_PARAM_TEMP, &temp);
	double hum;
	gn_leaf_param_get_double(leaf_config, GN_BME280_PARAM_HUM, &hum);
	double press;
	gn_leaf_param_get_double(leaf_config, GN_BME280_PARAM_PRESS, &press);

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);
	ESP_LOGD(TAG, "%s - bme280 task started", leaf_name);

	esp_err_t ret = i2cdev_init();

	if (ret != ESP_OK) {
		gn_leaf_param_force_bool(leaf_config, GN_BME280_PARAM_ACTIVE, false);
		gn_log(TAG, GN_LOG_ERROR, "i2cdev_init failed");
		descriptor->status = GN_LEAF_STATUS_ERROR;
	} else {
		//bme280 initialization
		ret = bmp280_init_default_params(&data->params);

		if (ret != ESP_OK) {
			gn_leaf_param_force_bool(leaf_config, GN_BME280_PARAM_ACTIVE,
					false);
			gn_log(TAG, GN_LOG_ERROR,
					"failed to init bmp280 default parameters");
			descriptor->status = GN_LEAF_STATUS_ERROR;
			//return descriptor;
		} else {

			ESP_LOGD(TAG, "bmp280_init_default_params - OK");
			//vTaskDelay(1000 / portTICK_PERIOD_MS);

			memset(&data->dev, 0, sizeof(bmp280_t));
			ret = bmp280_init_desc(&data->dev, BMP280_I2C_ADDRESS_0, 0,
					(int) sda, (int) scl);

			//ret = bmp280_init_desc(&data.dev, BMP280_I2C_ADDRESS_0, 0, 21, 22);
			if (ret != ESP_OK) {
				gn_leaf_param_force_bool(leaf_config, GN_BME280_PARAM_ACTIVE,
				false);
				gn_log(TAG, GN_LOG_ERROR,
						"failed to init bmp280 driver descriptor");
				descriptor->status = GN_LEAF_STATUS_ERROR;
				//return descriptor;
			} else {

				ESP_LOGD(TAG, "bmp280_init_desc - OK, sda = %d, scl = %d",
						(int )sda, (int )scl);
				//vTaskDelay(1000 / portTICK_PERIOD_MS);

				ret = bmp280_init(&data->dev, &data->params);

				if (ret != ESP_OK) {
					gn_leaf_param_force_bool(leaf_config,
							GN_BME280_PARAM_ACTIVE,
							false);
					gn_log(TAG, GN_LOG_ERROR, "failed to init bmp280 driver");
					descriptor->status = GN_LEAF_STATUS_ERROR;
					//return descriptor;
				} else {

					ESP_LOGD(TAG,
							"bmp280_init - OK, SDA = %d, SCL = %d, port = %d, type = %s",
							data->dev.i2c_dev.cfg.sda_io_num,
							data->dev.i2c_dev.cfg.scl_io_num,
							data->dev.i2c_dev.port,
							(data->dev.id == BME280_CHIP_ID) ? "BME280" : "BMP280");
					//vTaskDelay(1000 / portTICK_PERIOD_MS);

					ESP_LOGD(TAG, "creating timer...");
					//create a timer to update temps
					esp_timer_create_args_t bme280_sensor_timer_args = {
							.callback = &bme280_sensor_collect, .arg =
									leaf_config, .name =
									"leaf_bme280_sensor_collect" };

					ret = esp_timer_create(&bme280_sensor_timer_args,
							&data->bme280_sensor_timer);
					if (ret != ESP_OK) {
						gn_leaf_param_force_bool(leaf_config,
								GN_BME280_PARAM_ACTIVE,
								false);
						gn_log(TAG, GN_LOG_ERROR,
								"failed to init bme280 leaf timer");
						descriptor->status = GN_LEAF_STATUS_ERROR;
						//return descriptor;
					}

				}

			}
		}
	}

	ESP_LOGD(TAG, "done initializing");
	//vTaskDelay(1000 / portTICK_PERIOD_MS);

	//start timer if needed
	if (ret == ESP_OK && active == true) {

		ESP_LOGD(TAG, "starting timer, polling at %f sec", update_time);

		ret = esp_timer_start_periodic(data->bme280_sensor_timer,
				update_time * 1000000);
		if (ret != ESP_OK) {
			gn_leaf_param_force_bool(leaf_config, GN_BME280_PARAM_ACTIVE,
					false);
			gn_log(TAG, GN_LOG_ERROR, "failed to start bme280 leaf timer");
			gn_leaf_get_descriptor(leaf_config)->status = GN_LEAF_STATUS_ERROR;
		}

	}

//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	lv_obj_t *label_title = NULL;

	lv_obj_t *label_temp_title = NULL;
	lv_obj_t *label_temp_value = NULL;

	lv_obj_t *label_hum_title = NULL;
	lv_obj_t *label_hum_value = NULL;

	lv_obj_t *label_press_title = NULL;
	lv_obj_t *label_press_value = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			//style from the container
			//lv_style_t *style = _cnt->styles->style;

			ESP_LOGD(TAG, "Set Layout");
			//lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			ESP_LOGD(TAG, "label_title");
			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title, leaf_name);
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

			ESP_LOGD(TAG, "label_temp_title");
			label_temp_title = lv_label_create(_cnt);
			lv_label_set_text(label_temp_title, "Temp");
			//lv_obj_add_style(trg_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_temp_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "label_temp_value");
			label_temp_value = lv_label_create(_cnt);
			lv_label_set_text(label_temp_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_temp_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "label_hum_title");
			label_hum_title = lv_label_create(_cnt);
			lv_label_set_text(label_hum_title, "Humidity");
			//lv_obj_add_style(trg_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_hum_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "label_hum_value");
			label_hum_value = lv_label_create(_cnt);
			lv_label_set_text(label_hum_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_hum_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "label_press_title");
			label_press_title = lv_label_create(_cnt);
			lv_label_set_text(label_press_title, "Pressure");
			//lv_obj_add_style(trg_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_press_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "label_press_value");
			label_press_value = lv_label_create(_cnt);
			lv_label_set_text(label_press_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_press_value, LV_GRID_ALIGN_STRETCH, 1, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "end");

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

				//parameter is update time
				if (gn_leaf_event_mask_param(&evt, data->update_time_param)
						== 0) {

					//check limits
					double updtime;
					if (gn_event_payload_to_double(evt, &updtime) != GN_RET_OK) {
						break;
					}
					if (updtime < 10)
						updtime = 10;
					if (updtime > 600)
						updtime = 600;

					//execute change
					gn_leaf_param_force_double(leaf_config,
							GN_BME280_PARAM_UPDATE_TIME_SEC, updtime);
					update_time = updtime;

					if (active == true) {
						esp_timer_stop(data->bme280_sensor_timer);

						esp_timer_start_periodic(data->bme280_sensor_timer,
								update_time * 1000000);
					}

				} else if (gn_leaf_event_mask_param(&evt, data->sda_param)
						== 0) {

					//check limits
					double _sda = 0;
					if (gn_event_payload_to_double(evt, &_sda) != GN_RET_OK) {
						break;
					}

					if (_sda >= 0 && _sda < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_force_double(leaf_config,
								GN_BME280_PARAM_SDA, _sda);
						sda = _sda;
					}

				} else if (gn_leaf_event_mask_param(&evt, data->scl_param)
						== 0) {

					//check limits
					double _scl = 0;
					if (gn_event_payload_to_double(evt, &_scl) != GN_RET_OK) {
						break;
					}


					if (_scl >= 0 && scl < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_force_double(leaf_config,
								GN_BME280_PARAM_SDA, _scl);
						scl = _scl;
					}

				} else if (gn_leaf_event_mask_param(&evt, data->active_param)
						== 0) {

					bool prev_active = active;

					bool _active = false;
					if (gn_event_payload_to_bool(evt, &_active) != GN_RET_OK) {
						break;
					}

					//stop timer if false
					if (_active == false && prev_active == true) {
						esp_timer_stop(data->bme280_sensor_timer);
					} else if (_active == true && prev_active == false) {
						esp_timer_start_periodic(data->bme280_sensor_timer,
								update_time * 1000000);
					}

				}

				break;

				//what to do when network is connected
			case GN_NET_CONNECTED_EVENT:
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

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
