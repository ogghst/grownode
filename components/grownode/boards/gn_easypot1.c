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

#include <stdio.h>

#include "esp_log.h"

#include "grownode.h"
#include "gn_capacitive_moisture_sensor.h"
#include "gn_ds18b20.h"
#include "gn_led.h"
#include "gn_gpio.h"
#include "soc/touch_sensor_channel.h"

#include "gn_easypot1.h"

#define TAG "gn_easypot1"

gn_leaf_handle_t moist, temp, led_moist, led_temp;

double moist_last, temp_last;

//sets the tresholds
const double moist_min = 20;
const double moist_max = 70;
const double temp_min = 15;
const double temp_max = 28;

//milliseconds to blink for low and high values
const double blink_time_high = 300;
const double blink_time_low = 2000;

void _gn_easypot1_callback() {

	double temp_act = 0;
	gn_leaf_param_get_double(temp, GN_DS18B20_PARAM_SENSOR_NAMES[0], &temp_act);
	gn_log(TAG, GN_LOG_DEBUG, "easypot1 - measuring temp: %f", temp_act);

	//turn on the LED if low with a specific frequency
	if (temp_act < temp_min && temp_last >= temp_min) {
		gn_leaf_param_set_bool(led_temp, GN_LED_PARAM_TOGGLE, true);
		gn_leaf_param_set_double(led_temp, GN_LED_PARAM_BLINK_TIME_MS,
				blink_time_low);
	}

	//turn on the LED if high with a specific frequency
	else if (temp_act > temp_max && temp_last <= temp_max) {
		gn_leaf_param_set_bool(led_temp, GN_LED_PARAM_TOGGLE, true);
		gn_leaf_param_set_double(led_temp, GN_LED_PARAM_BLINK_TIME_MS,
				blink_time_high);
	}

	//turn off the LED if under normal threshold
	else if (temp_act <= temp_max && temp_act >= temp_min) {
		gn_leaf_param_set_bool(led_temp, GN_LED_PARAM_TOGGLE, false);
	}

	temp_last = temp_act;

	double moist_act = 0;
	gn_leaf_param_get_double(moist, GN_CMS_PARAM_ACT_LEVEL, &moist_act);
	gn_log(TAG, GN_LOG_DEBUG, "easypot1 - measuring moisture: %f", moist_act);

	//turn on the LED if low with a specific frequency
	if (moist_act < moist_min && moist_last >= moist_min) {
		gn_leaf_param_set_bool(led_moist, GN_LED_PARAM_TOGGLE, true);
		gn_leaf_param_set_double(led_moist, GN_LED_PARAM_BLINK_TIME_MS,
				blink_time_low);
	}

	//turn on the LED if high with a specific frequency
	else if (moist_act > moist_max && moist_last <= moist_max) {
		gn_leaf_param_set_bool(led_moist, GN_LED_PARAM_TOGGLE, true);
		gn_leaf_param_set_double(led_moist, GN_LED_PARAM_BLINK_TIME_MS,
				blink_time_high);
	}

	//turn off the LED if under normal threshold
	else if (moist_act <= moist_max && moist_act >= moist_min) {
		gn_leaf_param_set_bool(led_moist, GN_LED_PARAM_TOGGLE, false);
	}

	moist_last = moist_act;

	gn_log(TAG, GN_LOG_DEBUG, "easypot1 - callback ended");

}

/**
 * @brief
 *
 */
void gn_configure_easypot1(gn_node_handle_t node) {

	//leaves
	esp_log_level_set("gn_leaf_led", esp_log_level_get(TAG));
	esp_log_level_set("gn_leaf_gpio", esp_log_level_get(TAG));
	esp_log_level_set("gn_leaf_cms", esp_log_level_get(TAG));
	esp_log_level_set("gn_leaf_ds18b20", esp_log_level_get(TAG));

	//creates the moisture sensor
	moist = gn_leaf_create(node, "moist", gn_capacitive_moisture_sensor_config,
			4096);
	//set the channel
	gn_leaf_param_init_double(moist, GN_CMS_PARAM_ADC_CHANNEL, 5); //gpio33
	//set update time
	gn_leaf_param_init_double(moist, GN_CMS_PARAM_UPDATE_TIME_SEC, 15);
	//set initial status to active (on)
	gn_leaf_param_init_bool(moist, GN_CMS_PARAM_ACTIVE, true);

	//creates the temperature sensor
	temp = gn_leaf_create(node, "temp", gn_ds18b20_config, 4096);
	//set GPIO
	gn_leaf_param_init_double(temp, GN_DS18B20_PARAM_GPIO, 26);
	//set update time
	gn_leaf_param_init_double(temp, GN_DS18B20_PARAM_UPDATE_TIME_SEC, 15);
	//set initial status to active (on)
	gn_leaf_param_init_bool(temp, GN_DS18B20_PARAM_ACTIVE, true);

	//create the temp led leaf
	led_temp = gn_leaf_create(node, "led_temp", gn_led_config, 4096);
	gn_leaf_param_init_double(led_temp, GN_LED_PARAM_GPIO, 14);

	//create the moisture led leaf
	led_moist = gn_leaf_create(node, "led_moist", gn_led_config, 4096);
	gn_leaf_param_init_double(led_moist, GN_LED_PARAM_GPIO, 27);

	//creates a timer that checks temperature and moisture, using esp_timer API
	esp_timer_handle_t sensor_temp_handler;
	esp_timer_create_args_t sensor_temp_args = { .callback = &_gn_easypot1_callback,
			.name = "easypot1_timer" };
	esp_timer_create(&sensor_temp_args, &sensor_temp_handler);
	esp_timer_start_periodic(sensor_temp_handler, 30 * 1000000);

}

