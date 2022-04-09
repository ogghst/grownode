// Copyright 2021 Adamo Ferro (adamo@af-projects.it)
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

#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "gn_leaf_mux_cms.h"

#define TAG "gn_leaf_mux_cms"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;

typedef struct {

	int addrs[GN_MUX_CMS_MAX_SENSORS][GN_MUX_CMS_ADDR_BITS];
	float moist[GN_MUX_CMS_MAX_SENSORS];

	gn_leaf_param_handle_t active_param;
	gn_leaf_param_handle_t n_sensors_param;
	gn_leaf_param_handle_t adc_channel_param;
	gn_leaf_param_handle_t sel_gpio0_param;
	gn_leaf_param_handle_t sel_gpio1_param;
	gn_leaf_param_handle_t sel_gpio2_param;
	gn_leaf_param_handle_t sel_gpio3_param;
	gn_leaf_param_handle_t en_gpio_param;
	gn_leaf_param_handle_t max_moist_param;
	gn_leaf_param_handle_t min_moist_param;
	gn_leaf_param_handle_t moist_param[GN_MUX_CMS_MAX_SENSORS];
	gn_leaf_param_handle_t trg_hig_param[GN_MUX_CMS_MAX_SENSORS];
	gn_leaf_param_handle_t trg_low_param[GN_MUX_CMS_MAX_SENSORS];
	gn_leaf_param_handle_t upd_time_sec_param;
	esp_adc_cal_characteristics_t *adc_chars;

	esp_timer_handle_t sensor_timer;

} gn_mux_cms_data_t;

void gn_mux_cms_task(gn_leaf_handle_t leaf_config);

void gn_mux_cms_sensor_collect(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_mux_cms_sensor_collect", leaf_name);

	gn_mux_cms_data_t *data = (gn_mux_cms_data_t*) gn_leaf_get_descriptor(
				leaf_config)->data;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_MUX_CMS_PARAM_ACTIVE, &active);

	if (active) {
		int sel_gpios[GN_MUX_CMS_ADDR_BITS];

		double n_sensors;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_N_SENSORS,
				&n_sensors);

		double adc_channel;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_ADC_CHANNEL,
				&adc_channel);

		double sel_gpio0;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO0,
				&sel_gpio0);
		sel_gpios[0] = (int)sel_gpio0;

		double sel_gpio1;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO1,
				&sel_gpio1);
		sel_gpios[1] = (int)sel_gpio1;

		double sel_gpio2;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO2,
				&sel_gpio2);
		sel_gpios[2] = (int)sel_gpio2;

		double sel_gpio3;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO3,
				&sel_gpio3);
		sel_gpios[3] = (int)sel_gpio3;

		double en_gpio;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_EN_GPIO,
				&en_gpio);

		double max_moist;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_MAX_MOIST, &max_moist);

		double min_moist;
		gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_MIN_MOIST, &min_moist);

		ESP_LOGD(TAG, "[%s] reading from cms sensors through mux on ADC channel %d..", leaf_name, (int)adc_channel);

		esp_err_t ret;
		for (int j = 0; j < n_sensors; j++) {

			bool trg_high;
			gn_leaf_param_get_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_HIGH[j], &trg_high);

			bool trg_low;
			gn_leaf_param_get_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_LOW[j], &trg_low);

			// properly set address GPIOs
			for (int i = 0; i < GN_MUX_CMS_ADDR_BITS; i++) {
//				ESP_LOGD(TAG, "[%s] [sensor %d] set addr GPIO %d to %d...", leaf_name, j+1, sel_gpios[i], data->addrs[j][i]);
				ret = gpio_set_level(sel_gpios[i], data->addrs[j][i]);
//				if (ret != ESP_OK) {
//					gn_log(TAG, GN_LOG_ERROR,
//							"[%s] [sensor %d] ...failed", leaf_name, j+1);
//				}
			}
//			ESP_LOGD(TAG, "[%s] [sensor %d] address set", leaf_name, j+1);

			vTaskDelay(100 / portTICK_PERIOD_MS);

			// enable mux
			gpio_set_level(en_gpio, GN_MUX_CMS_ENABLE_LEVEL);
//			ESP_LOGD(TAG, "[%s] [sensor %d] enable low", leaf_name, j+1);

			vTaskDelay(100 / portTICK_PERIOD_MS);


//			ESP_LOGD(TAG, "[%s] [sensor %d] get measurements...", leaf_name, j+1);
			double result = 0;
			// get analog measure from CMS with multisampling
			for (int s = 0; s < NO_OF_SAMPLES; s++) {
				result += adc1_get_raw((adc1_channel_t) adc_channel);
			}

			result /= NO_OF_SAMPLES;

			//Convert adc_reading to voltage in mV
			//uint32_t voltage = esp_adc_cal_raw_to_voltage(result, adc_chars);
			//ESP_LOGD(TAG, "Raw: %d\tVoltage: %dmV\n", result, voltage);

			ESP_LOGD(TAG, "[%s] [sensor %d]      raw data: %f", leaf_name, j+1, result);


			// disable mux
			gpio_set_level(en_gpio, !GN_MUX_CMS_ENABLE_LEVEL);
//			ESP_LOGD(TAG, "[%s] [sensor %d] enable high", leaf_name, j+1);



			//convert to a percentage (4095 = 12 bit width * 100)
			result = result / 40.95;

			ESP_LOGD(TAG, "[%s] [sensor %d]      output data: %f", leaf_name, j+1, result);

			//store parameter and notify network
			gn_leaf_param_write_double(leaf_config, GN_MUX_CMS_PARAM_SENSOR_NAMES[j], result);

			//if level is above maximum, trigger max
			if (result >= max_moist && trg_high == false) {
				gn_leaf_param_write_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_HIGH[j], true);
			} else

			//reset the trigger status if returning below max
			if (result < max_moist && trg_high == true) {
				gn_leaf_param_write_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_HIGH[j], false);
			} else

			//if level is below minimum, trigger min
			if (result <= min_moist && trg_low == false) {
				gn_leaf_param_write_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_LOW[j], true);
			} else

			//reset the trigger status if returning above min
			if (result > min_moist && trg_low == true) {
				gn_leaf_param_write_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_LOW[j], false);
			}
		}
	}
}

gn_leaf_descriptor_handle_t gn_mux_cms_config(
		gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_mux_cms_config", leaf_name);

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_MUX_CMS_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_mux_cms_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_mux_cms_data_t *data = malloc(sizeof(gn_mux_cms_data_t));

	data->active_param = gn_leaf_param_create(leaf_config, GN_MUX_CMS_PARAM_ACTIVE,
			GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b = true },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);

	data->n_sensors_param = gn_leaf_param_create(leaf_config,
			GN_MUX_CMS_PARAM_N_SENSORS, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 16 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);


	data->adc_channel_param = gn_leaf_param_create(leaf_config,
			GN_MUX_CMS_PARAM_ADC_CHANNEL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->sel_gpio0_param = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_SEL_GPIO0, GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 4 }, GN_LEAF_PARAM_ACCESS_ALL,
				GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->sel_gpio1_param = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_SEL_GPIO1, GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_ALL,
				GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->sel_gpio2_param = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_SEL_GPIO2, GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 5 }, GN_LEAF_PARAM_ACCESS_ALL,
				GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->sel_gpio3_param = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_SEL_GPIO3, GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 23 }, GN_LEAF_PARAM_ACCESS_ALL,
				GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->en_gpio_param = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_EN_GPIO, GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 2 }, GN_LEAF_PARAM_ACCESS_ALL,
				GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->max_moist_param = gn_leaf_param_create(leaf_config,
			GN_MUX_CMS_PARAM_MAX_MOIST, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 1000 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->min_moist_param = gn_leaf_param_create(leaf_config,
			GN_MUX_CMS_PARAM_MIN_MOIST, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 0 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);

	data->upd_time_sec_param = gn_leaf_param_create(leaf_config,
			GN_MUX_CMS_PARAM_UPDATE_TIME_SEC, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							20 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	gn_leaf_param_add_to_leaf(leaf_config, data->active_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->n_sensors_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->adc_channel_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->sel_gpio0_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->sel_gpio1_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->sel_gpio2_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->sel_gpio3_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->en_gpio_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->max_moist_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->min_moist_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->upd_time_sec_param);

	//get params for moist. init to 0
	for (int i = 0; i < GN_MUX_CMS_MAX_SENSORS; i++) {
		data->moist_param[i] = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_SENSOR_NAMES[i], GN_VAL_TYPE_DOUBLE,
				(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_NODE,
				GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
		gn_leaf_param_add_to_leaf(leaf_config, data->moist_param[i]);
	}

	//get params for trg_hig. init to 0
	for (int i = 0; i < GN_MUX_CMS_MAX_SENSORS; i++) {
		data->trg_hig_param[i] = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_TRG_HIGH[i], GN_VAL_TYPE_BOOLEAN,
				(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_NODE,
				GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
		gn_leaf_param_add_to_leaf(leaf_config, data->trg_hig_param[i]);
	}

	//get params for trg_low. init to 0
	for (int i = 0; i < GN_MUX_CMS_MAX_SENSORS; i++) {
		data->trg_low_param[i] = gn_leaf_param_create(leaf_config,
				GN_MUX_CMS_PARAM_TRG_LOW[i], GN_VAL_TYPE_BOOLEAN,
				(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_NODE,
				GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);
		gn_leaf_param_add_to_leaf(leaf_config, data->trg_low_param[i]);
	}

	// initialize address matrix (least significant bit in column i=0)
	for (int i = 0; i < GN_MUX_CMS_ADDR_BITS; i++) {
		for (int j = 0; j < GN_MUX_CMS_MAX_SENSORS; j++) {
			data->addrs[j][i] = (j % (int)pow(2, i+1)) / (int)pow(2, i);
		}
	}

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_mux_cms_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_mux_cms_task", leaf_name);

	esp_err_t ret;
	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_leaf_descriptor_handle_t descriptor = gn_leaf_get_descriptor(
			leaf_config);
	gn_mux_cms_data_t *data =
			(gn_mux_cms_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	bool active;
	gn_leaf_param_get_bool(leaf_config, GN_MUX_CMS_PARAM_ACTIVE, &active);

	double n_sensors;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_N_SENSORS,
				&n_sensors);

	double adc_channel;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_ADC_CHANNEL,
			&adc_channel);

	double sel_gpio0;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO0,
			&sel_gpio0);

	double sel_gpio1;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO1,
			&sel_gpio1);

	double sel_gpio2;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO2,
			&sel_gpio2);

	double sel_gpio3;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_SEL_GPIO3,
			&sel_gpio3);

	double en_gpio;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_EN_GPIO,
			&en_gpio);

//	double min_moist;
//	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_MIN_MOIST, &min_moist);
//
//	double max_moist;
//	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_MAX_MOIST, &max_moist);
//
//	bool trg_high;
//	gn_leaf_param_get_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_HIGH, &trg_high);
//
//	bool trg_low;
//	gn_leaf_param_get_bool(leaf_config, GN_MUX_CMS_PARAM_TRG_LOW, &trg_low);

	double update_time_sec;
	gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_UPDATE_TIME_SEC,
			&update_time_sec);

	ESP_LOGD(TAG,
			"[%s] configuring multiplexed moisture sensors\n"
			"\tn_sensors = %d\n"
			"\tsel gpios = %d %d %d %d"
			"\ten gpio = %d"
			"\tunit = %d, width = %d, channel = %d, atten = %d",
			leaf_name, (int)n_sensors, (int)sel_gpio0, (int)sel_gpio1, (int)sel_gpio2, (int)sel_gpio3, (int)en_gpio, unit, width, (int )adc_channel, atten);

	//configure ADC
	adc1_config_width(width);
	adc1_config_channel_atten((int) adc_channel, atten);

	//Characterize ADC
	data->adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width,
	DEFAULT_VREF, data->adc_chars);

	if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
		ESP_LOGD(TAG, "[%s] characterized using Two Point Value", leaf_name);
	} else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
		ESP_LOGD(TAG, "[%s] characterized using eFuse Vref", leaf_name);
	} else {
		ESP_LOGD(TAG, "[%s] characterized using Default Vref", leaf_name);
	}

	//setup GPIOs
	gpio_set_direction((int) sel_gpio0, GPIO_MODE_OUTPUT);
	gpio_set_direction((int) sel_gpio1, GPIO_MODE_OUTPUT);
	gpio_set_direction((int) sel_gpio2, GPIO_MODE_OUTPUT);
	gpio_set_direction((int) sel_gpio3, GPIO_MODE_OUTPUT);
	gpio_set_direction((int) en_gpio, GPIO_MODE_OUTPUT);
	gpio_set_level((int) sel_gpio0, 0);
	gpio_set_level((int) sel_gpio1, 0);
	gpio_set_level((int) sel_gpio2, 0);
	gpio_set_level((int) sel_gpio3, 0);
	gpio_set_level((int) en_gpio, !GN_MUX_CMS_ENABLE_LEVEL);

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
	const esp_timer_create_args_t mux_cms_timer_args = { .callback =
			&gn_mux_cms_sensor_collect, .arg = leaf_config, .name = "mux_cms_timer" };

	ret = esp_timer_create(&mux_cms_timer_args, &data->sensor_timer);
	if (ret != ESP_OK) {
		gn_log(TAG, GN_LOG_ERROR,
				"[%s] failed to init mux cms sensor timer",
				leaf_name);
	}

	if (ret == ESP_OK && active == true) {

		//first shot immediate
		gn_mux_cms_sensor_collect(leaf_config);

		//then start the periodic wakeup
		if (ret == ESP_OK) {

			ESP_LOGD(TAG, "[%s] starting timer, polling at %f sec", leaf_name, update_time_sec);

			ret = esp_timer_start_periodic(data->sensor_timer,
					update_time_sec * 1000000);
		}

		if (ret != ESP_OK) {
			gn_log(TAG, GN_LOG_ERROR,
					"[%s] failed to start mux capacitive moisture sensor timer",
					leaf_name);
			gn_leaf_get_descriptor(leaf_config)->status = GN_LEAF_STATUS_ERROR;
			gn_leaf_param_write_bool(leaf_config, GN_MUX_CMS_PARAM_ACTIVE, false);
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
					// TODO set limits as const in .h or use validator?
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
					gn_leaf_param_write_double(leaf_config,
							GN_MUX_CMS_PARAM_UPDATE_TIME_SEC, updtime);

				} else if (gn_leaf_event_mask_param(&evt, data->active_param)
						== 0) {

					bool prev_active = active;
					int _active = atoi(evt.data);

					//execute change
					gn_leaf_param_write_bool(leaf_config, GN_MUX_CMS_PARAM_ACTIVE,
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
			gn_leaf_param_get_double(leaf_config, GN_MUX_CMS_PARAM_ACT_LEVEL,
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
