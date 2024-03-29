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
#include <stdio.h>

#include "esp_log.h"

#include "grownode.h"
#include "gn_blink.h"

#define TAG "gn_blink"

void led_blink_callback(const gn_leaf_handle_t blink) {

	bool status = false;
	//gets the previous parameter status
	gn_leaf_param_get_bool(blink, GN_GPIO_PARAM_TOGGLE, &status);
	ESP_LOGD(TAG, "blinking - old status = %d", status);

	//invert the status
	status = !status;

	ESP_LOGI(TAG, "blinking - new status = %d", status);

	//set the new parameter
	gn_leaf_param_set_bool(blink, GN_GPIO_PARAM_TOGGLE, status);

}

/**
 * @brief this example makes a led blinking, either via network commands or after a timer
 *
 * here we use the gn_relay leaf attached to GPIO2 to let the LED blink
 *
 * Thanks to Adamo Ferro for the idea
 *
 */
void gn_configure_blink(gn_node_handle_t node) {

	esp_log_level_set("gn_leaf_gpio", esp_log_level_get(TAG));

	//fastcreate call
	gn_leaf_handle_t blink = gn_gpio_fastcreate(node, "blink", 2, false, false);

	//creates a timer that fires the led blinking every 5 seconds, using esp_timer API

	esp_timer_handle_t timer_handler;
	esp_timer_create_args_t timer_args = { .callback = &led_blink_callback,
			.arg = blink, .name = "blink_timer" };
	esp_timer_create(&timer_args, &timer_handler);
	esp_timer_start_periodic(timer_handler, 10 * 1000000);

}

