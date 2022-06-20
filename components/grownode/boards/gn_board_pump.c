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

#include "gn_board_pump.h"

#include <gn_gpio.h>
#include <stdio.h>

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_event.h"
#include "soc/touch_sensor_channel.h"
#include "hal/touch_sensor_types.h"

#include "grownode.h"
#include "gn_pump_hs.h"
#include "gn_ds18b20.h"
#include "gn_capacitive_water_level.h"
#include "gn_pwm.h"
#include "gn_bme280.h"

#include "synapses/gn_syn_nft1_control.h"

#include "gn_leaf_status_led.h"


#define TAG "gn_pump"

//leaves names
const char *PWM = "act";
const char *SYN = "ctrl";

/*
esp_timer_handle_t watering_cycle_start_timer;
esp_timer_handle_t watering_cycle_stop_timer;

int32_t watering_cycle_start_usec = 10 * 1000 * 1000;
int32_t watering_cycle_stop_usec = 2 * 1000 * 1000;
*/

/*
 * what to do when watering time stops
 */
/*
void watering_timer_stop_callback(void *arg) {

	ESP_LOGI(TAG, "watering_timer_stop_callback");

	gn_node_handle_t node = (gn_node_handle_t) arg;
	//gets the pump leaf
	gn_leaf_handle_t pwm = gn_leaf_get_config_handle(node, PWM);
	gn_leaf_param_set_bool(pwm, GN_LEAF_PWM_PARAM_TOGGLE, false);

}
*/

/*
 * what to do when it's watering time
 */
/*
void watering_timer_start_callback(void *arg) {

	ESP_LOGI(TAG, "watering_timer_start_callback");

	gn_node_handle_t node = (gn_node_handle_t) arg;
	//gets the pump leaf
	gn_leaf_handle_t pwm = gn_leaf_get_config_handle(node, PWM);
	gn_leaf_param_set_bool(pwm, GN_LEAF_PWM_PARAM_TOGGLE, true);

	//stops after a while
	esp_timer_start_once(watering_cycle_stop_timer, watering_cycle_stop_usec);
}
*/

/**
 * starts the timer once the node has started
 */
/*
void gn_board_pump_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {

	switch (event_id) {

	case GN_NODE_STARTED_EVENT:

		vTaskDelay(1000 / portTICK_PERIOD_MS);

		ESP_LOGI(TAG, "gn_board_pwm_event_handler - GN_NODE_STARTED_EVENT");
		esp_timer_start_periodic(watering_cycle_start_timer,
				watering_cycle_start_usec);
		break;
	}

}
*/


void gn_configure_pump(gn_node_handle_t node) {

	//leaves
	esp_log_level_set("gn_leaf_pwm", ESP_LOG_DEBUG);
	esp_log_level_set("gn_syn_nft1_contro", ESP_LOG_DEBUG);

	gn_leaf_handle_t leaf_pwm = gn_leaf_create(node, PWM, gn_leaf_pwm_config,
			4096, GN_LEAF_TASK_PRIORITY);

	gn_leaf_param_init_bool(leaf_pwm, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(leaf_pwm, GN_LEAF_PWM_PARAM_GPIO, 16);
	gn_leaf_param_init_double(leaf_pwm, GN_LEAF_PWM_PARAM_CHANNEL, 0);
	gn_leaf_param_init_double(leaf_pwm, GN_LEAF_PWM_PARAM_POWER, 60);

	gn_leaf_handle_t nft1_syn = gn_leaf_create(node, SYN,
			gn_syn_nft1_control_config, 4096, GN_LEAF_TASK_PRIORITY);

	gn_leaf_param_init_double(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_INTERVAL_SEC, 600);
	gn_leaf_param_init_double(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC, 30);
	gn_leaf_param_init_bool(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_ENABLE, true);
	gn_leaf_param_init_string(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF, PWM);


/*
	const esp_timer_create_args_t watering_timer_start_args = { .callback =
			&watering_timer_start_callback, .name = "watering_start", .arg = node };

	esp_timer_create(&watering_timer_start_args, &watering_cycle_start_timer);

	const esp_timer_create_args_t watering_timer_stop_args = { .callback =
			&watering_timer_stop_callback, .name = "watering_stop", .arg =
			node };

	esp_timer_create(&watering_timer_stop_args, &watering_cycle_stop_timer);

	//register for events
	esp_event_handler_instance_register_with(
			gn_node_get_event_loop(node), GN_BASE_EVENT, GN_EVENT_ANY_ID,
			gn_board_pump_event_handler, node, NULL);
*/

}

