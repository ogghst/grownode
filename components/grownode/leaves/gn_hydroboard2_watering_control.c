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

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_commons.h"

#include "gn_bme280.h"
#include "gn_ds18b20.h"
#include "gn_pwm.h"
#include "gn_relay.h"
#include "gn_capacitive_water_level.h"

#include "gn_hydroboard2_watering_control.h"

#define TAG "gn_leaf_hb2_watering_control"

static  char *PLT_FAN = "plt_fan";
static  char *PLT_PUMP = "plt_pump";
static  char *WAT_PUMP = "wat_pump";
static  char *PLT_COOL = "plt_b";
static  char *PLT_HOT = "plt_a";
static  char *ENV_FAN = "env_fan";
static  char *BME280 = "env_thp";
static  char *DS18B20 = "temps";
static  char *WAT_LEV = "wat_lev";
static  char *LIGHT_1 = "lig_1";
static  char *LIGHT_2 = "lig_2";

void gn_hb2_watering_control_task(gn_leaf_config_handle_t leaf_config);

typedef enum {
	HCC_HEATING, HCC_COOLING, HCC_OFF
} gn_hcc_status;

typedef enum {
	WAT_WAIT, WAT_ON, WAT_OFF
} gn_wat_status;

typedef struct {

	//esp_timer_handle_t watering_timer;

	gn_leaf_param_handle_t param_watering_interval;
	gn_leaf_param_handle_t param_watering_time;
	gn_leaf_param_handle_t param_active;
	gn_leaf_param_handle_t param_watering_t_temp;

	//child leaves name
	gn_leaf_config_handle_t param_leaf_plt_fan_name;
	gn_leaf_config_handle_t param_leaf_plt_pump_name;
	gn_leaf_config_handle_t param_leaf_wat_pump_name;
	gn_leaf_config_handle_t param_leaf_plt_cool_name;
	gn_leaf_config_handle_t param_leaf_plt_hot_name;
	gn_leaf_config_handle_t param_leaf_env_fan_name;
	gn_leaf_config_handle_t param_leaf_bme280_name;
	gn_leaf_config_handle_t param_leaf_ds18b20_name;
	gn_leaf_config_handle_t param_leaf_wat_lev_name;
	gn_leaf_config_handle_t param_leaf_light_1_name;
	gn_leaf_config_handle_t param_leaf_light_2_name;

	//child leaves
	gn_leaf_config_handle_t leaf_cwl;
	gn_leaf_config_handle_t leaf_ds18b20;
	gn_leaf_config_handle_t leaf_bme280;
	gn_leaf_config_handle_t leaf_plt_hot;
	gn_leaf_config_handle_t leaf_plt_cool;
	gn_leaf_config_handle_t leaf_plt_pump;
	gn_leaf_config_handle_t leaf_plt_fan;
	gn_leaf_config_handle_t leaf_wat_pump;
	gn_leaf_config_handle_t leaf_light_1_name;
	gn_leaf_config_handle_t leaf_light_2_name;

	gn_hcc_status hcc_cycle;
	int64_t hcc_cycle_start;

	gn_wat_status wat_cycle;
	int64_t wat_cycle_cumulative_time_ms;

} gn_hb2_watering_control_data_t;

gn_leaf_param_validator_result_t _gn_hb2_watering_interval_validator(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	if (GN_HYDROBOARD2_MIN_WATERING_INTERVAL > **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MIN_WATERING_INTERVAL,
				sizeof(GN_HYDROBOARD2_MIN_WATERING_INTERVAL));
		return GN_LEAF_PARAM_VALIDATOR_BELOW_MIN;
	} else if (GN_HYDROBOARD2_MAX_WATERING_INTERVAL
			< **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MAX_WATERING_INTERVAL,
				sizeof(GN_HYDROBOARD2_MAX_WATERING_INTERVAL));
		return GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

gn_leaf_param_validator_result_t _gn_hb2_watering_time_validator(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_time_validator - param: %d", (int )_p1);

	if (GN_HYDROBOARD2_MIN_WATERING_TIME > **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MIN_WATERING_TIME,
				sizeof(GN_HYDROBOARD2_MIN_WATERING_TIME));
		return GN_LEAF_PARAM_VALIDATOR_BELOW_MIN;
	} else if (GN_HYDROBOARD2_MAX_WATERING_TIME < **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MAX_WATERING_TIME,
				sizeof(GN_HYDROBOARD2_MAX_WATERING_TIME));
		return GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_time_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

gn_leaf_param_validator_result_t _gn_hb2_watering_target_temp_validator(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_temp_validator - param: %d", (int )_p1);

	if (GN_HYDROBOARD2_MIN_WATERING_TARGET_TEMP > **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MIN_WATERING_TARGET_TEMP,
				sizeof(GN_HYDROBOARD2_MIN_WATERING_TARGET_TEMP));
		return GN_LEAF_PARAM_VALIDATOR_BELOW_MIN;
	} else if (GN_HYDROBOARD2_MAX_WATERING_TARGET_TEMP
			< **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MAX_WATERING_TARGET_TEMP,
				sizeof(GN_HYDROBOARD2_MAX_WATERING_TARGET_TEMP));

		return GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_temp_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

inline static void _gn_hb2_watering_control_stop_watering(
		gn_hb2_watering_control_data_t *data) {
	gn_log(TAG, GN_LOG_INFO, "Stop Watering Cycle");
	gn_leaf_param_update_bool(data->leaf_wat_pump, GN_LEAF_PWM_PARAM_TOGGLE,
	false);
	data->wat_cycle = WAT_OFF;
	data->wat_cycle_cumulative_time_ms = 0;
}

inline static void _gn_hb2_watering_control_start_watering(
		gn_hb2_watering_control_data_t *data) {
	gn_log(TAG, GN_LOG_INFO, "Start Watering Cycle");
	gn_leaf_param_update_bool(data->leaf_wat_pump, GN_LEAF_PWM_PARAM_TOGGLE,
	true);
	data->wat_cycle = WAT_ON;
	data->wat_cycle_cumulative_time_ms = 0;
}

inline static void _gn_hb2_watering_control_stop_hcc(
		gn_hb2_watering_control_data_t *data) {
	gn_log(TAG, GN_LOG_INFO, "Stop Water Temp Setup Cycle");
	gn_leaf_param_update_bool(data->leaf_plt_hot, GN_RELAY_PARAM_TOGGLE, false);
	gn_leaf_param_update_bool(data->leaf_plt_cool, GN_RELAY_PARAM_TOGGLE, false);
	gn_leaf_param_update_bool(data->leaf_plt_pump, GN_LEAF_PWM_PARAM_TOGGLE,
	false);
	gn_leaf_param_update_bool(data->leaf_plt_fan, GN_LEAF_PWM_PARAM_TOGGLE,
	false);
	data->hcc_cycle = HCC_OFF;
}

inline static void _gn_hb2_watering_control_start_hcc_heating(
		gn_hb2_watering_control_data_t *data) {
	gn_log(TAG, GN_LOG_INFO, "Start Heating Cycle");
	gn_leaf_param_update_bool(data->leaf_plt_hot, GN_RELAY_PARAM_TOGGLE, true);
	gn_leaf_param_update_bool(data->leaf_plt_cool, GN_RELAY_PARAM_TOGGLE, false);
	gn_leaf_param_update_bool(data->leaf_plt_pump, GN_LEAF_PWM_PARAM_TOGGLE,
	true);
	gn_leaf_param_update_bool(data->leaf_plt_fan, GN_LEAF_PWM_PARAM_TOGGLE, true);
	data->hcc_cycle = HCC_HEATING;
	//store time of start water temp control cycle
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	data->hcc_cycle_start = (int64_t) tv_now.tv_sec * 1000000L
			+ (int64_t) tv_now.tv_usec;

}

inline static void _gn_hb2_watering_control_start_hcc_cooling(
		gn_hb2_watering_control_data_t *data) {
	gn_log(TAG, GN_LOG_INFO, "Start Cooling Cycle");
	gn_leaf_param_update_bool(data->leaf_plt_hot, GN_RELAY_PARAM_TOGGLE, false);
	gn_leaf_param_update_bool(data->leaf_plt_cool, GN_RELAY_PARAM_TOGGLE, true);
	gn_leaf_param_update_bool(data->leaf_plt_pump, GN_LEAF_PWM_PARAM_TOGGLE,
	true);
	gn_leaf_param_update_bool(data->leaf_plt_fan, GN_LEAF_PWM_PARAM_TOGGLE, true);
	data->hcc_cycle = HCC_COOLING;
	//store time of start water temp control cycle
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	data->hcc_cycle_start = (int64_t) tv_now.tv_sec * 1000000L
			+ (int64_t) tv_now.tv_usec;

}

inline static bool _gn_hb2_watering_control_hcc_temp_low(double p_wat_temp,
		double p_wat_t_temp) {
	//ESP_LOGD(TAG, "_gn_watering_control_hcc_temp_low. Actual: %f, Target %f",
	//		p_wat_temp, p_wat_t_temp);
	return p_wat_temp < (p_wat_t_temp - 1);
}

inline static bool _gn_hb2_watering_control_hcc_temp_high(double p_wat_temp,
		double p_wat_t_temp) {
	//ESP_LOGD(TAG, "_gn_watering_control_hcc_temp_high. Actual: %f, Target %f",
	//		p_wat_temp, p_wat_t_temp);
	return p_wat_temp > (p_wat_t_temp + 1);
}

inline static bool _gn_hb2_watering_control_hcc_temp_ok(double p_wat_temp,
		double p_wat_t_temp) {
	return (!_gn_hb2_watering_control_hcc_temp_low(p_wat_temp, p_wat_t_temp)
			&& !_gn_hb2_watering_control_hcc_temp_high(p_wat_temp, p_wat_t_temp));
}

void _gn_hb2_watering_callback_intl(gn_leaf_config_handle_t leaf_config) {

	ESP_LOGD(TAG, "_gn_hb2_watering_callback");

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_hb2_watering_control_data_t *data = descriptor->data;

	if (data->wat_cycle != WAT_WAIT) {
		ESP_LOGW(TAG,
				"Another Watering cycle is already active, not possible to start another.");
		return;
	}

	data->wat_cycle = WAT_OFF;

	gn_err_t ret;

	bool p_wat_active = false;
	double p_wat_int_sec = 0;
	double p_wat_time_sec = 0;
	double p_wat_t_temp = 0;

	double p_cwl = 0;
	bool p_cwl_active = false;
	bool p_cwl_trg_high = false;
	bool p_cwl_trg_low = false;

	double p_wat_temp = 0;
	double p_plt_temp = 0;
	bool p_ds18b20_temp_active = false;

	double p_amb_temp = 0;
	bool p_amb_temp_active = true; //TODO test, move to false again!

	bool p_plt_a_status = false;
	bool p_plt_b_status = false;

	bool p_plt_pump_toggle = false;
	double p_plt_pump_power = 0;

	bool p_wat_pump = false;

	struct timeval tv_now;
	int64_t time_us;

	//gets watering interval, out of the loop to configure it
	ret = gn_leaf_param_get_double(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC, &p_wat_int_sec);
	if (ret != GN_RET_OK) {
		gn_log(TAG, GN_LOG_ERROR, "watering interval not found");
	}

	//infinite loop
	while (true) {

		vTaskDelay((p_wat_int_sec * 1000L) / portTICK_PERIOD_MS);

		data->wat_cycle = WAT_OFF;

		ret = gn_leaf_param_get_bool(leaf_config,
				GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE, &p_wat_active);
		if (ret != GN_RET_OK) {
			gn_log(TAG, GN_LOG_ERROR,
					"watering cycle active parameter not found");
		}

		//watering cycle
		while (p_wat_active) {

			//check if watering interval is true
			ret = gn_leaf_param_get_bool(leaf_config,
					GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE, &p_wat_active);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR,
						"watering cycle active parameter not found");
			}

			//gets watering interval
			ret = gn_leaf_param_get_double(leaf_config,
					GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC,
					&p_wat_int_sec);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "watering interval not found");
				break;
			}

			//gets watering time
			ret = gn_leaf_param_get_double(leaf_config,
					GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TIME_SEC,
					&p_wat_time_sec);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "watering time not found");
				break;
			}

			//gets watering target temp
			ret = gn_leaf_param_get_double(leaf_config,
					GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TARGET_TEMP,
					&p_wat_t_temp);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "watering target temp not found");
				break;
			}

			//gets water level sensor active
			ret = gn_leaf_param_get_bool(leaf_config,
					GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE, &p_cwl_active);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR,
						"watering level active flag not found");
				break;
			}

			//gets water level sensor and value
			ret = gn_leaf_param_get_double(data->leaf_cwl,
					GN_CWL_PARAM_ACT_LEVEL, &p_cwl);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "actual water level not found");
				break;
			}

			//gets water too high value
			ret = gn_leaf_param_get_bool(data->leaf_cwl, GN_CWL_PARAM_TRG_HIGH,
					&p_cwl_trg_high);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR,
						"actual water trigger high value not found");
				break;
			}

			//gets water too low value
			ret = gn_leaf_param_get_bool(data->leaf_cwl, GN_CWL_PARAM_TRG_LOW,
					&p_cwl_trg_low);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR,
						"actual water trigger low value not found");
				break;
			}

			//gets db18b20 temperature active
			ret = gn_leaf_param_get_bool(data->leaf_ds18b20,
					GN_DS18B20_PARAM_ACTIVE, &p_ds18b20_temp_active);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR,
						"watering temperature active flag not found");
				break;
			}

			//gets water temperature and value
			ret = gn_leaf_param_get_double(data->leaf_ds18b20,
					GN_DS18B20_PARAM_SENSOR_NAMES[1], &p_plt_temp);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "water temperature not found");
				break;
			}

			//gets peltier temperature and value
			ret = gn_leaf_param_get_double(data->leaf_ds18b20,
					GN_DS18B20_PARAM_SENSOR_NAMES[0], &p_wat_temp);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "peltier temperature not found");
				break;
			}

			//gets ambient temperature active flag
			ret = gn_leaf_param_get_bool(data->leaf_bme280,
					GN_BME280_PARAM_ACTIVE, &p_amb_temp_active);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR,
						"ambient temperature active flag not found");
				break;
			}

			//gets ambient temperature and value
			ret = gn_leaf_param_get_double(data->leaf_bme280,
					GN_BME280_PARAM_TEMP, &p_amb_temp);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "ambient temperature not found");
				break;
			}

			//gets peltier A status
			ret = gn_leaf_param_get_bool(data->leaf_plt_hot,
					GN_RELAY_PARAM_TOGGLE, &p_plt_a_status);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "peltier A status not found");
				break;
			}

			//gets peltier B status
			ret = gn_leaf_param_get_bool(data->leaf_plt_cool,
					GN_RELAY_PARAM_TOGGLE, &p_plt_b_status);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "peltier B status not found");
				break;
			}

			//gets water pump status
			ret = gn_leaf_param_get_bool(data->leaf_wat_pump,
					GN_LEAF_PWM_PARAM_TOGGLE, &p_wat_pump);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "water pump status not found");
				break;
			}

			//gets plt pump status
			ret = gn_leaf_param_get_bool(data->leaf_plt_pump,
					GN_LEAF_PWM_PARAM_TOGGLE, &p_plt_pump_toggle);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "hcc pump status not found");
				break;
			}

			//gets hcc power status
			ret = gn_leaf_param_get_double(data->leaf_plt_pump,
					GN_LEAF_PWM_PARAM_POWER, &p_plt_pump_power);
			if (ret != GN_RET_OK) {
				gn_log(TAG, GN_LOG_ERROR, "hcc pump power not found");
				break;
			}

			// watering algorithm start
			ESP_LOGD(TAG, "Start Watering Check Cycle. Status = %d",
					(int )data->hcc_cycle);

			//if some of the sensors are not active, stop every operation and exit
			if (!p_cwl_active || !p_ds18b20_temp_active) {
				gn_log(TAG, GN_LOG_WARNING, "Sensors not active");
				_gn_hb2_watering_control_stop_hcc(data);
				_gn_hb2_watering_control_stop_watering(data);
				break;

			}

			//check water level. if out of allowable range - send error and end of cycle
			if (p_cwl_trg_low == true) {
				gn_log(TAG, GN_LOG_WARNING,
						"Not Enough Water to start watering cycle");
				_gn_hb2_watering_control_stop_hcc(data);
				_gn_hb2_watering_control_stop_watering(data);
				break;

			} else if (p_cwl_trg_high == true) {
				gn_log(TAG, GN_LOG_WARNING,
						"Water level too high to start watering cycle");
				_gn_hb2_watering_control_stop_hcc(data);
				_gn_hb2_watering_control_stop_watering(data);
				break;

			}

			//check need of cooling
			if (data->hcc_cycle != HCC_COOLING
					&& _gn_hb2_watering_control_hcc_temp_high(p_wat_temp,
							p_wat_t_temp)) {
				_gn_hb2_watering_control_start_hcc_cooling(data);
			}

			//check need of heating
			if (data->hcc_cycle != HCC_HEATING
					&& _gn_hb2_watering_control_hcc_temp_low(p_wat_temp,
							p_wat_t_temp)) {
				_gn_hb2_watering_control_start_hcc_heating(data);
			}

			// make sure to return to normal parameter if temp within target
			if (data->hcc_cycle != HCC_OFF
					&& _gn_hb2_watering_control_hcc_temp_ok(p_wat_temp,
							p_wat_t_temp)) {
				_gn_hb2_watering_control_stop_hcc(data);
			}

			//if water temp into normal threshold then start watering
			if (_gn_hb2_watering_control_hcc_temp_ok(p_wat_temp, p_wat_t_temp)
					&& data->wat_cycle == WAT_OFF) {
				_gn_hb2_watering_control_start_watering(data);
			}

			//check if water temp climate time is below maximum time
			if (data->hcc_cycle != HCC_OFF) {

				//get current time
				gettimeofday(&tv_now, NULL);
				time_us = (int64_t) tv_now.tv_sec * 1000000L
						+ (int64_t) tv_now.tv_usec;

				//check last hcc activation
				if ((time_us - data->hcc_cycle_start)
						> (((int64_t) GN_HYDROBOARD2_MAX_HCC_CYCLE_TIME_SEC)
								* 1000000L)) {
					gn_log(TAG, GN_LOG_INFO,
							"Maximum Water Temp Climate Cycle (%l) reached, ending",
							GN_HYDROBOARD2_MAX_HCC_CYCLE_TIME_SEC);
					_gn_hb2_watering_control_stop_hcc(data);
					break;
				}

			}

			//add time to the watering time
			if (data->wat_cycle == WAT_ON) {
				data->wat_cycle_cumulative_time_ms +=
						GN_HYDROBOARD2_WAT_CTR_CYCLE_TIME_MS;
				ESP_LOGD(TAG, "Cumulative time (msec): %llu",
						data->wat_cycle_cumulative_time_ms);
			}

			//if out of maximum watering time - end of cycle
			if (data->wat_cycle_cumulative_time_ms > (p_wat_time_sec * 1000L)) {
				gn_log(TAG, GN_LOG_INFO,
						"Maximum Watering time (%l) reached, ending",
						p_wat_time_sec);
				_gn_hb2_watering_control_stop_watering(data);
				break;
			}

			//wait until next cycle
			vTaskDelay(
					GN_HYDROBOARD2_WAT_CTR_CYCLE_TIME_MS / portTICK_PERIOD_MS);

		}

		data->wat_cycle = WAT_WAIT;
		//gn_log(TAG, GN_LOG_INFO, "Ending Watering Cycle");

	}

}

/*
 void _gn_watering_callback(gn_leaf_config_handle_t leaf_config) {
 //retrieves status descriptor from config
 gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
 leaf_config);
 gn_watering_control_data_t *data = descriptor->data;
 if (data->wat_cycle != WAT_WAIT) return;
 xTaskCreate((void*)_gn_watering_callback_intl, "_gn_watering_callback_intl", 4096,
 leaf_config, 1, NULL);
 }
 */

gn_leaf_descriptor_handle_t gn_hb2_watering_control_config(
		gn_leaf_config_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_HYDROBOARD2_WATERING_CONTROL_TYPE,
	GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_hb2_watering_control_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;
	descriptor->data = NULL;

	//gn_node_config_handle_t node_config = gn_leaf_get_node(leaf_config);

	gn_hb2_watering_control_data_t *data = malloc(
			sizeof(gn_hb2_watering_control_data_t));

	data->wat_cycle = WAT_WAIT;
	data->wat_cycle_cumulative_time_ms = 0;
	data->hcc_cycle = HCC_OFF;
	data->hcc_cycle_start = 0;

	data->param_watering_time = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TIME_SEC, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 20 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, _gn_hb2_watering_time_validator);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_watering_time);

	data->param_watering_interval = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 60 * 60 },
			GN_LEAF_PARAM_ACCESS_NETWORK, GN_LEAF_PARAM_STORAGE_PERSISTED,
			_gn_hb2_watering_interval_validator);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_watering_interval);

	data->param_watering_t_temp = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TARGET_TEMP,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 22 },
			GN_LEAF_PARAM_ACCESS_NETWORK, GN_LEAF_PARAM_STORAGE_PERSISTED,
			_gn_hb2_watering_target_temp_validator);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_watering_t_temp);

	data->param_active = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b =
					false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_active);

	//parameters for leaves
	data->param_leaf_plt_fan_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_FAN, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = PLT_FAN },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_plt_fan_name);

	data->param_leaf_plt_pump_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_PUMP, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = PLT_PUMP },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_plt_pump_name);

	data->param_leaf_wat_pump_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_PUMP, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = WAT_PUMP },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_wat_pump_name);

	data->param_leaf_plt_cool_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_COOL, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = PLT_COOL },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_plt_cool_name);

	data->param_leaf_plt_hot_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_HOT, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = PLT_HOT },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_plt_hot_name);

	data->param_leaf_env_fan_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_ENV_FAN, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = ENV_FAN },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_env_fan_name);

	data->param_leaf_bme280_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_BME280, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = BME280 },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_bme280_name);

	data->param_leaf_ds18b20_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_DS18B20, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = DS18B20 },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_ds18b20_name);

	data->param_leaf_wat_lev_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_LEV, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = WAT_LEV },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_wat_lev_name);

	data->param_leaf_light_1_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_1, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = LIGHT_1 },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_light_1_name);

	data->param_leaf_light_2_name = gn_leaf_param_create(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_2, GN_VAL_TYPE_STRING,
			(gn_val_t ) { .s = LIGHT_2 },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config, data->param_leaf_light_1_name);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;

	descriptor->data = data;
	return descriptor;

	/*
	fail:
	descriptor->status = GN_LEAF_STATUS_ERROR;
	descriptor->data = data;
	return descriptor;
	*/
}

void gn_hb2_watering_control_task(gn_leaf_config_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);
	ESP_LOGD(TAG, "%s - gn_hb2_watering_control_task", leaf_name);

	gn_node_config_handle_t node_config = gn_leaf_get_node(leaf_config);

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_hb2_watering_control_data_t *data = descriptor->data;

	char plt_fan_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_FAN, plt_fan_name, 16);
	data->leaf_plt_fan =
			gn_leaf_get_config_handle(node_config, plt_fan_name);
	if (data->leaf_plt_fan == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find plt_fan leaf");
		goto fail;
	}

	char plt_pump_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_PUMP, plt_pump_name, 16), data->leaf_plt_pump =
			gn_leaf_get_config_handle(node_config, plt_pump_name);
	if (data->leaf_plt_pump == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find hcc_pump leaf");
		goto fail;
	}

	char wat_pump_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_PUMP, wat_pump_name, 16), data->leaf_wat_pump =
			gn_leaf_get_config_handle(node_config, wat_pump_name);
	if (data->leaf_wat_pump == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find wat_pump leaf");
		goto fail;
	}

	char plt_cool_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_COOL, plt_cool_name, 16), data->leaf_plt_cool =
			gn_leaf_get_config_handle(node_config, plt_cool_name);
	if (data->leaf_plt_cool == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find plt_b leaf");
		goto fail;
	}

	char plt_hot_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_HOT, plt_hot_name, 16), data->leaf_plt_hot =
			gn_leaf_get_config_handle(node_config, plt_hot_name);
	if (data->leaf_plt_hot == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find plt_a leaf");
		goto fail;
	}

	char env_fan_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_ENV_FAN, env_fan_name, 16);
	//TODO not yet implemented

	char bme280_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_BME280, bme280_name, 16);
	data->leaf_bme280 = gn_leaf_get_config_handle(node_config, bme280_name);
	if (data->leaf_bme280 == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find bme280 leaf");
		goto fail;
	}

	char ds18b20_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_DS18B20, ds18b20_name, 16);
	data->leaf_ds18b20 = gn_leaf_get_config_handle(node_config, ds18b20_name);
	if (data->leaf_ds18b20 == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find ds18b20 leaf");
		goto fail;
	}

	char wat_lev_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_LEV, wat_lev_name, 16);
	data->leaf_cwl = gn_leaf_get_config_handle(node_config, wat_lev_name);
	if (data->leaf_cwl == NULL) {
		gn_log(TAG, GN_LOG_ERROR, "not possible to find cwl leaf");
		goto fail;
	}

	char light_1_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_1, light_1_name, 16);
	//TODO not yet implemented

	char light_2_name[16];
	gn_leaf_param_get_string(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_1, light_2_name, 16);
	//TODO not yet implemented

	gn_leaf_event_subscribe(leaf_config, GN_LEAF_PARAM_CHANGED_EVENT);

	double p_wat_int_sec;
	gn_leaf_param_get_double(leaf_config,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC, &p_wat_int_sec);

	bool p_active;
	gn_leaf_param_get_bool(leaf_config, GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE,
			&p_active);

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

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

		}

		gn_display_leaf_refresh_end();
	}

#endif

	xTaskCreate((void*) _gn_hb2_watering_callback_intl,
			"_gn_hb2_watering_callback_intl", 4096, leaf_config, 1, NULL);

	//task cycle
	while (true) {

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			//ESP_LOGD(TAG, "event %d", evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change for this node
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				//ESP_LOGD(TAG, "request to update param %s, data = '%s'",
				//		evt.param_name, evt.data);

				//parameter is watering interval
				if (gn_leaf_event_mask_param(&evt,
						data->param_watering_interval) == 0) {
					gn_leaf_param_set_double(leaf_config,
							GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC,
							(double) atof(evt.data));
					gn_leaf_param_get_double(leaf_config,
							GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC,
							&p_wat_int_sec);
					//esp_timer_stop(data->watering_timer);
					//esp_timer_start_periodic(data->watering_timer,
					//		p_wat_int_sec * 1000000);
				} else
				//parameter is watering time
				if (gn_leaf_event_mask_param(&evt,
						data->param_watering_time) == 0) {
					gn_leaf_param_set_double(leaf_config,
							GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TIME_SEC,
							(double) atof(evt.data));
				} else if (gn_leaf_event_mask_param(&evt,
						data->param_watering_t_temp) == 0) {
					gn_leaf_param_set_double(leaf_config,
							GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TARGET_TEMP,
							(double) atof(evt.data));
				} else
				//parameter is active
				if (gn_leaf_event_mask_param(&evt, data->param_active)
						== 0) {

					int _active = atoi(evt.data);

					//execute change
					gn_leaf_param_set_bool(leaf_config,
							GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE,
							_active == 0 ? false : true);

					p_active = _active;

					//stop timer if false
					//if (_active == 0 && prev_active == true) {
					//	esp_timer_stop(data->watering_timer);
					//} else if (_active != 0 && prev_active == false) {
					//	esp_timer_start_periodic(data->watering_timer,
					//			p_wat_int_sec * 1000000);
					//}

				}
				break;

			case GN_LEAF_PARAM_CHANGED_EVENT:

				//ESP_LOGD(TAG, "notified update param %s, leaf %s, data = '%s'",
				//		evt.param_name, evt.leaf_name, evt.data);

				break;

			default:
				break;

			}

		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);

	}

	//in case of error, the leaf goes in an infinite loop doing nothing
	fail: descriptor->status = GN_LEAF_STATUS_ERROR;
	while(true) {
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
