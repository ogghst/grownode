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

#include "gn_easypot1.h"

#define TAG "gn_easypot1"

void measuring_callback(const gn_leaf_config_handle_t cms) {

	double moisture;
	gn_leaf_param_get_double(cms, GN_CMS_PARAM_ACT_LEVEL, &moisture);

	gn_log(TAG, GN_LOG_INFO, "easypot1 - measuring moisture: %f", moisture);

}

/**
 * @brief
 *
 */
void gn_configure_easypot1(gn_node_config_handle_t node) {

	//creates the moisture sensor
	gn_leaf_config_handle_t cms = gn_leaf_create(node, "cms",
			gn_capacitive_moisture_sensor_config, 4096);

	//set the channel 0
	gn_leaf_param_init_double(cms, GN_CMS_PARAM_ADC_CHANNEL, 7); //GPIO35

	//set update time
	gn_leaf_param_init_double(cms, GN_CMS_PARAM_UPDATE_TIME_SEC, 5);

	//set initial status to active (on)
	gn_leaf_param_init_bool(cms, GN_CMS_PARAM_ACTIVE, true);

	//creates a timer that checks moisture every 5 seconds, using esp_timer API

	esp_timer_handle_t timer_handler;
	esp_timer_create_args_t timer_args = { .callback = &measuring_callback,
			.arg = cms, .name = "easypot1_timer" };
	esp_timer_create(&timer_args, &timer_handler);
	esp_timer_start_periodic(timer_handler, 5 * 1000000);

}

