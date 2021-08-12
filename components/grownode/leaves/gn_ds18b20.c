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

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "lvgl_helpers.h"

#include "ds18x20.h"
#include "gn_ds18b20.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char *TAG = "gn_ds18b20";

#define MAX_SENSORS 4
static const char sensor_names[MAX_SENSORS][6] = { "temp1", "temp2", "temp3",
		"temp4" };

const size_t GN_DS18B20_STATE_STOP = 0;
const size_t GN_DS18B20_STATE_RUNNING = 1;

void _scan_sensors(int gpio, bool *scanned, size_t *sensor_count,
		ds18x20_addr_t *addrs) {

	if (*scanned == true)
		return;

	esp_err_t res;
	*scanned = false;
	*sensor_count = 0;

	res = ds18x20_scan_devices(gpio, addrs, MAX_SENSORS, sensor_count);
	if (res != ESP_OK) {
		ESP_LOGD(TAG, "Sensors scan error %d (%s)", res, esp_err_to_name(res));
		return;
	}

	ESP_LOGD(TAG, "%d sensors detected", *sensor_count);

	// If there were more sensors found than we have space to handle,
	// just report the first MAX_SENSORS..
	if (*sensor_count > MAX_SENSORS) {
		*sensor_count = MAX_SENSORS;
	}

	*scanned = true;
}

struct leaf_data {
	gn_leaf_config_handle_t leaf_config;
	size_t sensor_count;
	ds18x20_addr_t addrs[MAX_SENSORS];
	float temp[MAX_SENSORS];
	gn_leaf_param_handle_t temp_param[MAX_SENSORS];
	size_t gn_ds18b20_state;
	bool scanned;
	gn_leaf_param_handle_t update_time_param;
	gn_leaf_param_handle_t gpio_param;
};

static void temp_sensor_collect(void *arg) {

	ESP_LOGD(TAG, "temp_sensor_collect");

	struct leaf_data *data = (struct leaf_data*) arg;

	esp_err_t res;

	if (data->gn_ds18b20_state == GN_DS18B20_STATE_RUNNING) {

		//read data from sensors using GPIO parameter
		res = ds18x20_measure_and_read_multi(data->gpio_param->param_val->v.d,
				data->addrs, data->sensor_count, data->temp);

		if (res != ESP_OK) {
			ESP_LOGD(TAG, "Sensors read error %d (%s)", res,
					esp_err_to_name(res));
			goto fail;
		}

		for (int j = 0; j < data->sensor_count; j++) {
			float temp_c = data->temp[j];
			float temp_f = (temp_c * 1.8) + 32;
			ESP_LOGD(TAG, "Sensor %08x%08x (%s) reports %.3f �C (%.3f �F)",
					(uint32_t )(data->addrs[j] >> 32),
					(uint32_t )data->addrs[j],
					(data->addrs[j] & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20",
					temp_c, temp_f);

			//store parameter and notify network
			gn_leaf_param_set_double(data->leaf_config,
					data->temp_param[j]->name, temp_c);
		}

		fail: return;

	}
}

void gn_ds18b20_task(gn_leaf_config_handle_t leaf_config) {

	struct leaf_data data;
	data.sensor_count = 0;
	data.gn_ds18b20_state = GN_DS18B20_STATE_STOP;
	data.scanned = false;
	data.leaf_config = leaf_config;

	gn_leaf_event_t evt;

	//create a timer to update temps
	esp_timer_handle_t temp_sensor_timer;
	const esp_timer_create_args_t temp_sensor_timer_args = { .callback =
			&temp_sensor_collect, .arg = (void*) &data,
	/* name is optional, but may help identify the timer when debugging */
	.name = "periodic" };
	ESP_ERROR_CHECK(esp_timer_create(&temp_sensor_timer_args, &temp_sensor_timer));

	//parameter definition. if found in flash storage, they will be created with found values instead of default

	//get update time in sec, default 30
	data.update_time_param = gn_leaf_param_create(leaf_config,
			"update_time_sec", GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 30 });
	gn_leaf_param_add(leaf_config, data.update_time_param);

	//get gpio from params. default 27
	data.gpio_param = gn_leaf_param_create(leaf_config, "gpio",
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 27 });
	gn_leaf_param_add(leaf_config, data.gpio_param);

	//setup gpio
	gpio_set_pull_mode(data.gpio_param->param_val->v.d, GPIO_PULLUP_ONLY);

	//init sensors
	_scan_sensors(data.gpio_param->param_val->v.d, &data.scanned,
			&data.sensor_count, &data.addrs[0]);

	//get params for temp. init to 0
	for (int i = 0; i < data.sensor_count; i++) {
		data.temp_param[i] = gn_leaf_param_create(leaf_config, sensor_names[i],
				GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 });
		gn_leaf_param_add(leaf_config, data.temp_param[i]);
	}

	//if mqtt is connected, start sensor callback
	if (gn_mqtt_get_status() == GN_SERVER_CONNECTED) {
		ESP_LOGD(TAG, "esp_timer_start, frequency %f sec", data.update_time_param->param_val->v.d);
		data.gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
		ESP_ERROR_CHECK(
				esp_timer_start_periodic(temp_sensor_timer,
						data.update_time_param->param_val->v.d
								* 1000000));
	}

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	static lv_obj_t *label_temp_names[MAX_SENSORS];
	static lv_obj_t *label_temp[MAX_SENSORS];

	//parent container where adding elements
	lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);

	if (_cnt) {

		//style from the container
		lv_style_t *style = lv_style_list_get_local_style(&_cnt->style_list);

		if (pdTRUE == gn_display_leaf_refresh_start()) {
			for (int i = 0; i < data.sensor_count; i++) {
				label_temp_names[i] = lv_label_create(_cnt, NULL);
				lv_label_set_text(label_temp_names[i], sensor_names[i]);
				lv_obj_add_style(label_temp_names[i], LV_LABEL_PART_MAIN,
						style);

				char _p[21];
				snprintf(_p, 20, "temp: %4.2f",
						data.temp_param[i]->param_val->v.d);
				label_temp[i] = lv_label_create(_cnt, NULL);
				lv_label_set_text(label_temp[i], _p);
				lv_obj_add_style(label_temp[i], LV_LABEL_PART_MAIN, style);

			}
			gn_display_leaf_refresh_end();
		}

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
			case GN_NETWORK_CONNECTED_EVENT:

				break;

				//what to do when network is disconnected
			case GN_NETWORK_DISCONNECTED_EVENT:
				if (data.gn_ds18b20_state != GN_DS18B20_STATE_STOP) {
					data.gn_ds18b20_state = GN_DS18B20_STATE_STOP;
					//stop update cycle
					ESP_LOGD(TAG, "esp_timer_stop");
					ESP_ERROR_CHECK(esp_timer_stop(temp_sensor_timer));
				}
				break;

				//what to do when server is connected
			case GN_SERVER_CONNECTED_EVENT:
				if (data.gn_ds18b20_state != GN_DS18B20_STATE_RUNNING) {
					data.gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
					//start update cycle
					ESP_LOGD(TAG, "esp_timer_start, frequency %f sec", data.update_time_param->param_val->v.d);
					ESP_ERROR_CHECK(
							esp_timer_start_periodic(temp_sensor_timer,
									data.update_time_param->param_val->v.d
											* 1000000));

				}
				break;

				//what to do when server is disconnected
			case GN_SERVER_DISCONNECTED_EVENT:
				if (data.gn_ds18b20_state != GN_DS18B20_STATE_STOP) {
					data.gn_ds18b20_state = GN_DS18B20_STATE_STOP;
					//stop update cycle
					ESP_LOGD(TAG, "esp_timer_stop");
					ESP_ERROR_CHECK(esp_timer_stop(temp_sensor_timer));
				}
				break;

			default:
				break;
			}

		}

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
		if (pdTRUE == gn_display_leaf_refresh_start()) {
			for (int i = 0; i < data.sensor_count; i++) {
				char _p[21];
				snprintf(_p, 20, "temp: %4.2f",
						data.temp_param[i]->param_val->v.d);
				lv_label_set_text(label_temp[i], _p);
			}
			gn_display_leaf_refresh_end();
		}
#endif

	}

	ESP_ERROR_CHECK(esp_timer_stop(temp_sensor_timer));

}

#ifdef __cplusplus
}
#endif //__cplusplus