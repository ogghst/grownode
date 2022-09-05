// Copyright 2021 Adamo Ferro (adamo@af-project.it)
// based on the work of Nicola Muratori (nicola.muratori@gmail.com)
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

#include "bh1750.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_bh1750.h"

#define TAG "gn_leaf_bh1750"

typedef struct {

	i2c_dev_t i2c_dev;  //!< I2C device descriptor
	bh1750_mode_t mode;
	bh1750_resolution_t resolution;

	esp_timer_handle_t bh1750_sensor_timer;

	gn_leaf_param_handle_t sda_param;
	gn_leaf_param_handle_t scl_param;
	gn_leaf_param_handle_t update_time_param;
	gn_leaf_param_handle_t active_param;
	gn_leaf_param_handle_t lux_param;

} gn_bh1750_data_t;

/**
 * creates a BH1750 leaf defining all parameters
 *
 * @param	node			the parent node
 * @param	leaf_name		the name of the leaf to be created
 * @param	sda				the GPIO number for I2C SDA
 * @param	scl				the GPIO number for I2C SCL
 * @param	update_time_sec	the time interval between measurements [seconds]
 *
 * @return	an handle to the leaf initialized
 * @return	NULL		in case of errors
 */
gn_leaf_handle_t gn_bh1750_fastcreate(gn_node_handle_t node,
		const char *leaf_name, int sda, int scl, double update_time_sec) {

	if (node == NULL) {
		ESP_LOGE(TAG, "gn_bh1750_fastcreate - node is null");
		return NULL;
	}

	if (leaf_name == NULL) {
		ESP_LOGE(TAG, "gn_bh1750_fastcreate - leaf_name is null");
		return NULL;
	}

	if (sda < GPIO_NUM_0 || sda > GPIO_NUM_MAX) {
		ESP_LOGE(TAG, "gn_bh1750_fastcreate - sda gpio out of limits");
		return NULL;
	}

	if (scl < GPIO_NUM_0 || scl > GPIO_NUM_MAX) {
		ESP_LOGE(TAG, "gn_bh1750_fastcreate - scl gpio out of limits");
		return NULL;
	}

	if (update_time_sec <= 0) {
		ESP_LOGE(TAG, "gn_bh1750_fastcreate - update time must be positive");
		return NULL;
	}

	//creates the bh1750 leave
	gn_leaf_handle_t leaf = gn_leaf_create(node, leaf_name, gn_bh1750_config,
			4096, GN_LEAF_TASK_PRIORITY);

	if (leaf == NULL) {
		ESP_LOGE(TAG, "gn_bh1750_fastcreate - cannot create leaf %s",
				leaf_name);
		return NULL;
	}

	gn_leaf_param_init_double(leaf, GN_BH1750_PARAM_SDA, sda);
	gn_leaf_param_init_double(leaf, GN_BH1750_PARAM_SCL, scl);
	gn_leaf_param_init_double(leaf, GN_BH1750_PARAM_UPDATE_TIME_SEC,
			update_time_sec);

	ESP_LOGD(TAG, "[%s] bh1750 leaf created", leaf_name);

	return leaf;

}

void bh1750_sensor_collect(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "bh1750_sensor_collect");

	gn_bh1750_data_t *data = (gn_bh1750_data_t*) gn_leaf_get_descriptor(
			leaf_config)->data;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_BH1750_PARAM_ACTIVE, &active);

	if (active == true) {

		esp_err_t res;
		uint16_t lux;

		ESP_LOGD(TAG, "[%s] reading lux value...", leaf_name);

		// read data from sensor
		res = bh1750_read(&data->i2c_dev, &lux);

		if (res != ESP_OK) {
			gn_log(TAG, GN_LOG_ERROR, "[%s] sensor read error %d (%s)",
					leaf_name, res, esp_err_to_name(res));
//			goto fail;
		} else {

			ESP_LOGD(TAG, "[%s] Illuminance: %.2f lux\n", leaf_name,
					(double )lux);

			//store parameter and notify network
			gn_leaf_param_force_double(leaf_config, GN_BH1750_PARAM_LUX,
					(double) lux);

		}
	}

}

void gn_bh1750_task(gn_leaf_handle_t leaf_config);

gn_leaf_descriptor_handle_t gn_bh1750_config(gn_leaf_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_BH1750_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_bh1750_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	gn_bh1750_data_t *data = malloc(sizeof(gn_bh1750_data_t));

	//parameter definition. if found in flash storage, they will be created with found values instead of default

	data->active_param = gn_leaf_param_create(leaf_config,
			GN_BH1750_PARAM_ACTIVE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = true }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->active_param);

	data->sda_param = gn_leaf_param_create(leaf_config, GN_BH1750_PARAM_SDA,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 21 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->sda_param);

	data->scl_param = gn_leaf_param_create(leaf_config, GN_BH1750_PARAM_SCL,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 22 },
			GN_LEAF_PARAM_ACCESS_NODE, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->scl_param);

	data->update_time_param = gn_leaf_param_create(leaf_config,
			GN_BH1750_PARAM_UPDATE_TIME_SEC, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 20 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->update_time_param);

	data->lux_param = gn_leaf_param_create(leaf_config, GN_BH1750_PARAM_LUX,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->lux_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;

	ESP_LOGD(TAG, "bh1750 leaf configured");

	return descriptor;

}

void gn_bh1750_task(gn_leaf_handle_t leaf_config) {

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_bh1750_data_t *data = descriptor->data;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_BH1750_PARAM_ACTIVE, &active);
	double sda;
	gn_leaf_param_get_double(leaf_config, GN_BH1750_PARAM_SDA, &sda);
	double scl;
	gn_leaf_param_get_double(leaf_config, GN_BH1750_PARAM_SCL, &scl);
	double update_time;
	gn_leaf_param_get_double(leaf_config, GN_BH1750_PARAM_UPDATE_TIME_SEC,
			&update_time);
	double lux;
	gn_leaf_param_get_double(leaf_config, GN_BH1750_PARAM_LUX, &lux);

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);
	ESP_LOGD(TAG, "[%s] bh1750 task started", leaf_name);

	esp_err_t ret = i2cdev_init();
	data->mode = BH1750_MODE_CONTINUOUS;
	data->resolution = BH1750_RES_HIGH2;

	if (ret != ESP_OK) {
		gn_leaf_param_force_bool(leaf_config, GN_BH1750_PARAM_ACTIVE, false);
		gn_log(TAG, GN_LOG_ERROR, "[%s] i2cdev_init failed", leaf_name);
		descriptor->status = GN_LEAF_STATUS_ERROR;
	} else {
		memset(&data->i2c_dev, 0, sizeof(i2c_dev_t));
		ret = bh1750_init_desc(&data->i2c_dev, BH1750_ADDR_LO, 0, (int) sda,
				(int) scl);

		if (ret != ESP_OK) {
			gn_leaf_param_force_bool(leaf_config, GN_BH1750_PARAM_ACTIVE,
			false);
			gn_log(TAG, GN_LOG_ERROR,
					"[%s] failed to init bh1750 driver descriptor", leaf_name);
			descriptor->status = GN_LEAF_STATUS_ERROR;
		} else {

			ESP_LOGD(TAG, "[%s] bh1750_init_desc - OK, sda = %d, scl = %d",
					leaf_name, (int )sda, (int )scl);

			ret = bh1750_setup(&data->i2c_dev, data->mode, data->resolution);

			if (ret != ESP_OK) {
				gn_leaf_param_force_bool(leaf_config, GN_BH1750_PARAM_ACTIVE,
				false);
				gn_log(TAG, GN_LOG_ERROR, "[%s] failed to init bh1750 driver",
						leaf_name);
				descriptor->status = GN_LEAF_STATUS_ERROR;
			} else {

				ESP_LOGD(TAG,
						"[%s] bh1750_init - OK, SDA = %d, SCL = %d, port = %d",
						leaf_name, data->i2c_dev.cfg.sda_io_num,
						data->i2c_dev.cfg.scl_io_num, data->i2c_dev.port);
				//vTaskDelay(1000 / portTICK_PERIOD_MS);

				ESP_LOGD(TAG, "[%s] creating timer...", leaf_name);
				//create a timer to update temps
				esp_timer_create_args_t bh1750_sensor_timer_args = { .callback =
						&bh1750_sensor_collect, .arg = leaf_config, .name =
						"leaf_bh1750_sensor_collect" };

				ret = esp_timer_create(&bh1750_sensor_timer_args,
						&data->bh1750_sensor_timer);
				if (ret != ESP_OK) {
					gn_leaf_param_force_bool(leaf_config,
							GN_BH1750_PARAM_ACTIVE,
							false);
					gn_log(TAG, GN_LOG_ERROR,
							"[%s] failed to init bh1750 leaf timer", leaf_name);
					descriptor->status = GN_LEAF_STATUS_ERROR;
					//return descriptor;
				}

			}

		}
	}

	ESP_LOGD(TAG, "[%s] done initializing", leaf_name);
	//vTaskDelay(1000 / portTICK_PERIOD_MS);

	//start timer if needed
	if (ret == ESP_OK && active == true) {

		//first shot immediate
		bh1750_sensor_collect(leaf_config);

		ESP_LOGD(TAG, "[%s] starting timer, polling at %f sec", leaf_name,
				update_time);

		ret = esp_timer_start_periodic(data->bh1750_sensor_timer,
				update_time * 1000000);
		if (ret != ESP_OK) {
			gn_leaf_param_force_bool(leaf_config, GN_BH1750_PARAM_ACTIVE,
			false);
			gn_log(TAG, GN_LOG_ERROR, "[%s] failed to start bh1750 leaf timer",
					leaf_name);
			gn_leaf_get_descriptor(leaf_config)->status = GN_LEAF_STATUS_ERROR;
		}

	}

//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	lv_obj_t *label_title = NULL;

	lv_obj_t *label_lux_title = NULL;
	lv_obj_t *label_lux_value = NULL;

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

			ESP_LOGD(TAG, "label_lux_title");
			label_lux_title = lv_label_create(_cnt);
			lv_label_set_text(label_lux_title, "Temp");
			//lv_obj_add_style(trg_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_lux_title, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 1);

			ESP_LOGD(TAG, "label_lux_value");
			label_lux_value = lv_label_create(_cnt);
			lv_label_set_text(label_lux_value, "---");
			//lv_obj_add_style(trg_value, style, 0);
			//lv_obj_align_to(trg_value, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_lux_value, LV_GRID_ALIGN_STRETCH, 1, 1,
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

			ESP_LOGD(TAG, "[%s] received message: %d", leaf_name, evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "[%s] request to update param %s, data = '%s'",
						leaf_name, evt.param_name, evt.data);

				//parameter is update time
				if (gn_leaf_event_mask_param(&evt, data->update_time_param)
						== 0) {

					//check limits
					double updtime;
					if (gn_double_from_event(evt, &updtime) != GN_RET_OK) {
						break;
					}

					if (updtime < 10)
						updtime = 10;
					if (updtime > 600)
						updtime = 600;

					//execute change
					gn_leaf_param_force_double(leaf_config,
							GN_BH1750_PARAM_UPDATE_TIME_SEC, updtime);
					update_time = updtime;

					if (active == true) {
						esp_timer_stop(data->bh1750_sensor_timer);

						esp_timer_start_periodic(data->bh1750_sensor_timer,
								update_time * 1000000);
					}

				} else if (gn_leaf_event_mask_param(&evt, data->sda_param)
						== 0) {

					//check limits
					double _sda = 0;
					if (gn_double_from_event(evt, &_sda) != GN_RET_OK) {
						break;
					}

					if (_sda >= 0 && _sda < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_force_double(leaf_config,
								GN_BH1750_PARAM_SDA, _sda);
						sda = _sda;
					}

				} else if (gn_leaf_event_mask_param(&evt, data->scl_param)
						== 0) {

					//check limits
					double _scl = 0;
					if (gn_double_from_event(evt, &_scl) != GN_RET_OK) {
						break;
					}

					if (_scl >= 0 && scl < GPIO_NUM_MAX) {
						//execute change. this will have no effects until restart
						gn_leaf_param_force_double(leaf_config,
								GN_BH1750_PARAM_SDA, _scl);
						scl = _scl;
					}

				} else if (gn_leaf_event_mask_param(&evt, data->active_param)
						== 0) {

					bool prev_active = active;
					bool _active = 0;
					if (gn_bool_from_event(evt, &_active) != GN_RET_OK) {
						break;
					}

					//execute change
					gn_leaf_param_force_bool(leaf_config,
							GN_BH1750_PARAM_ACTIVE, _active);
					active = _active;

					//stop timer if false
					if (_active == 0 && prev_active == true) {
						esp_timer_stop(data->bh1750_sensor_timer);
					} else if (_active != 0 && prev_active == false) {
						esp_timer_start_periodic(data->bh1750_sensor_timer,
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
