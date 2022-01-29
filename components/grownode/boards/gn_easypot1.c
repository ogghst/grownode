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

#include "gn_easypot1.h"

#define TAG "gn_easypot1"

gn_leaf_handle_t moist, temp, led_moist, led_temp;

double moist_last, temp_last;

//sets the tresholds
const double moist_min = 1;
const double moist_max = 3;
const double temp_min = 15;
const double temp_max = 28;

//milliseconds to blink for low and high values
const double blink_time_high = 300;
const double blink_time_low = 2000;

void moisture_callback(const gn_leaf_handle_t moist) {

	double moist_act;
	gn_leaf_param_get_double(moist, GN_CMS_PARAM_ACT_LEVEL, &moist_act);
	gn_log(TAG, GN_LOG_INFO, "easypot1 - measuring moisture: %f", moist_act);

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

}

void temp_callback(const gn_leaf_handle_t temp) {

	double temp_act;
	gn_leaf_param_get_double(temp, GN_CMS_PARAM_ACT_LEVEL, &temp_act);
	gn_log(TAG, GN_LOG_INFO, "easypot1 - measuring temp: %f", temp_act);

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

}

/**
 * @brief
 *
 */
void gn_configure_easypot1(gn_node_handle_t node) {

	//leaves
	//esp_log_level_set("gn_leaf_led", ESP_LOG_INFO);

	//creates the moisture sensor
	moist = gn_leaf_create(node, "moist", gn_capacitive_moisture_sensor_config,
			4096);
	//set the channel 4
	gn_leaf_param_init_double(moist, GN_CMS_PARAM_ADC_CHANNEL, 4); //GPIO12
	//set update time
	gn_leaf_param_init_double(moist, GN_CMS_PARAM_UPDATE_TIME_SEC, 5);
	//set initial status to active (on)
	gn_leaf_param_init_bool(moist, GN_CMS_PARAM_ACTIVE, true);

	//creates the temperature sensor
	temp = gn_leaf_create(node, "temp", gn_ds18b20_config, 4096);
	//set GPIO
	gn_leaf_param_init_double(temp, GN_DS18B20_PARAM_GPIO, 0);
	//set update time
	gn_leaf_param_init_double(temp, GN_DS18B20_PARAM_UPDATE_TIME_SEC, 5);
	//set initial status to active (on)
	gn_leaf_param_init_bool(temp, GN_DS18B20_PARAM_ACTIVE, true);

	//create the temp led leaf
	led_temp = gn_leaf_create(node, "led_temp", gn_led_config, 4096);
	gn_leaf_param_init_double(led_temp, GN_LED_PARAM_GPIO, 1);

	//create the moisture led leaf
	led_moist = gn_leaf_create(node, "led_moist", gn_led_config, 4096);
	gn_leaf_param_init_double(led_moist, GN_LED_PARAM_GPIO, 2);

	//creates a timer that checks moisture every seconds, using esp_timer API
	esp_timer_handle_t timer_moisture_handler;
	esp_timer_create_args_t timer_moisture_args = { .callback =
			&moisture_callback, .name = "moist_timer" };
	esp_timer_create(&timer_moisture_args, &timer_moisture_handler);
	esp_timer_start_periodic(timer_moisture_handler, 1 * 1000000);

	//creates a timer that checks temperature every seconds, using esp_timer API
	esp_timer_handle_t timer_temp_handler;
	esp_timer_create_args_t timer_temp_args = { .callback = &temp_callback,
			.name = "temp_timer" };
	esp_timer_create(&timer_temp_args, &timer_temp_handler);
	esp_timer_start_periodic(timer_temp_handler, 1 * 1000000);

}

