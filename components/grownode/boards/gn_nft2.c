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

#include <gn_gpio.h>
#include <gn_leaf_pwm_relay.h>
#include <stdio.h>

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_event.h"
#include "soc/touch_sensor_channel.h"
#include "hal/touch_sensor_types.h"

#include "grownode.h"
#include "gn_ds18b20.h"
#include "gn_capacitive_water_level.h"
#include "gn_pwm.h"
#include "gn_bme280.h"
#include "synapses/gn_hydroboard2_watering_control.h"
#include "gn_leaf_status_led.h"

#include "gn_nft2.h"

#define TAG "gn_nft2"

//leaves names
const char *PWM1 = "pwm1";
const char *PWM2 = "pwm2";
const char *PWM3 = "pwm3";
const char *LED = "led";
const char *WAT_LEV = "wat_lev";
const char *BME280 = "bme280";
const char *DS18B20 = "ds18b20";

esp_timer_handle_t watering_cycle_start_timer;
esp_timer_handle_t watering_cycle_stop_timer;

int32_t watering_cycle_start_usec = 10 * 1000 * 1000;
int32_t watering_cycle_stop_usec = 2 * 1000 * 1000;

/*
 * what to do when watering time stops
 */
void watering_timer_stop_callback(void *arg) {

	ESP_LOGI(TAG, "watering_timer_stop_callback");

	gn_node_handle_t node = (gn_node_handle_t) arg;
	//gets the pump leaf
	gn_leaf_handle_t pwm1 = gn_leaf_get_config_handle(node, PWM1);
	gn_leaf_param_set_bool(pwm1, GN_LEAF_PWM_PARAM_TOGGLE, false);

}

/*
 * what to do when it's watering time
 */
void watering_timer_start_callback(void *arg) {

	ESP_LOGI(TAG, "watering_timer_start_callback");

	gn_node_handle_t node = (gn_node_handle_t) arg;
	//gets the pump leaf
	gn_leaf_handle_t pwm1 = gn_leaf_get_config_handle(node, PWM1);
	gn_leaf_param_set_bool(pwm1, GN_LEAF_PWM_PARAM_TOGGLE, true);

	//stops after a while
	esp_timer_start_once(watering_cycle_stop_timer, watering_cycle_stop_usec);
}

/**
 * starts the timer once the node has started
 */
void gn_board_nft2_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {

	switch (event_id) {

	case GN_NODE_STARTED_EVENT:

		vTaskDelay(1000 / portTICK_PERIOD_MS);

		ESP_LOGI(TAG, "gn_board_nft2_event_handler - GN_NODE_STARTED_EVENT");
		esp_timer_start_periodic(watering_cycle_start_timer,
				watering_cycle_start_usec);
		break;
	}

}

void gn_configure_nft2(gn_node_handle_t node) {

	//leaves
	esp_log_level_set("gn_leaf_gpio", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_pump_hs", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_ds18b20", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_cwl", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_bme280", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_status_led", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_pwm", ESP_LOG_DEBUG);

	gn_leaf_handle_t leaf_led = gn_leaf_create(node, LED, gn_gpio_config, 4096,
	GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_double(leaf_led, GN_GPIO_PARAM_GPIO, 19);
	gn_leaf_param_force_bool(leaf_led, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_force_bool(leaf_led, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t leaf_pwm1 = gn_leaf_create(node, PWM1, gn_leaf_pwm_config,
			4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_bool(leaf_pwm1, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_force_double(leaf_pwm1, GN_LEAF_PWM_PARAM_GPIO, 16);
	gn_leaf_param_force_double(leaf_pwm1, GN_LEAF_PWM_PARAM_CHANNEL, 0);
	gn_leaf_param_force_double(leaf_pwm1, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_handle_t leaf_pwm2 = gn_leaf_create(node, PWM2, gn_leaf_pwm_config,
			4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_bool(leaf_pwm2, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_force_double(leaf_pwm2, GN_LEAF_PWM_PARAM_GPIO, 17);
	gn_leaf_param_force_double(leaf_pwm2, GN_LEAF_PWM_PARAM_CHANNEL, 1);
	gn_leaf_param_force_double(leaf_pwm2, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_handle_t leaf_pwm3 = gn_leaf_create(node, PWM3, gn_leaf_pwm_config,
			4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_bool(leaf_pwm3, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_force_double(leaf_pwm3, GN_LEAF_PWM_PARAM_GPIO, 18);
	gn_leaf_param_force_double(leaf_pwm3, GN_LEAF_PWM_PARAM_CHANNEL, 2);
	gn_leaf_param_force_double(leaf_pwm3, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_handle_t wat_lev = gn_leaf_create(node, WAT_LEV,
			gn_capacitive_water_level_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_bool(wat_lev, GN_CWL_PARAM_ACTIVE, true);
	gn_leaf_param_force_double(wat_lev, GN_CWL_PARAM_TOUCH_CHANNEL,
	TOUCH_PAD_GPIO0_CHANNEL);
	gn_leaf_param_force_double(wat_lev, GN_CWL_PARAM_UPDATE_TIME_SEC, 10);
	gn_leaf_param_force_double(wat_lev, GN_CWL_PARAM_MIN_LEVEL, 0);
	gn_leaf_param_force_double(wat_lev, GN_CWL_PARAM_MAX_LEVEL, 2048);

	gn_leaf_handle_t bme280 = gn_leaf_create(node, BME280, gn_bme280_config,
			8192, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_double(bme280, GN_BME280_PARAM_SDA, 21);
	gn_leaf_param_force_double(bme280, GN_BME280_PARAM_SCL, 22);
	gn_leaf_param_force_bool(bme280, GN_BME280_PARAM_ACTIVE, true);
	gn_leaf_param_force_double(bme280, GN_BME280_PARAM_UPDATE_TIME_SEC, 10);

	gn_leaf_handle_t ds18b20 = gn_leaf_create(node, DS18B20, gn_ds18b20_config,
			4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_force_double(ds18b20, GN_DS18B20_PARAM_GPIO, 4);
	gn_leaf_param_force_bool(ds18b20, GN_DS18B20_PARAM_ACTIVE, true);
	gn_leaf_param_force_double(ds18b20, GN_DS18B20_PARAM_UPDATE_TIME_SEC, 5);


	/*
	const esp_timer_create_args_t watering_timer_start_args = { .callback =
			&watering_timer_start_callback, .name = "watering_start", .arg = node };

	esp_timer_create(&watering_timer_start_args, &watering_cycle_start_timer);

	const esp_timer_create_args_t watering_timer_stop_args = { .callback =
			&watering_timer_stop_callback, .name = "watering_stop", .arg =
			node };

	esp_timer_create(&watering_timer_stop_args, &watering_cycle_stop_timer);
	*/

	//register for events
	//esp_event_handler_instance_register_with(
	//		gn_node_get_event_loop(node), GN_BASE_EVENT, GN_EVENT_ANY_ID,
	//		gn_board_nft2_event_handler, node, NULL);


}

