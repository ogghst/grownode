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

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "esp_event.h"

#include "grownode.h"
#include "gn_pump_hs.h"
#include "gn_ds18b20.h"
#include "gn_capacitive_water_level.h"
#include "gn_pwm.h"
#include "gn_bme280.h"
#include "gn_hydroboard2_watering_control.h"
#include "gn_leaf_status_led.h"

#include "gn_hydroboard2.h"

void gn_configure_nft1(gn_node_handle_t node) {

	//leaves
	esp_log_level_set("gn_leaf_gpio", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_pump_hs", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_ds18b20", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_cwl", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_bme280", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_status_led", ESP_LOG_INFO);

	esp_log_level_set("gn_leaf_pump_control", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_hb2_watering_control", ESP_LOG_DEBUG);
	esp_log_level_set("gn_leaf_pwm", ESP_LOG_INFO);

	//leaves names
	const char *PUMP = "pump";
	const char *LIGHT = "light";

	gn_leaf_handle_t lights = gn_leaf_create(node, LIGHT,
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(lights, GN_GPIO_PARAM_GPIO, 26);
	gn_leaf_param_init_bool(lights, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(lights, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t pump = gn_leaf_create(node, PUMP,
			gn_leaf_pwm_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_bool(pump, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(pump, GN_LEAF_PWM_PARAM_GPIO, 34);
	gn_leaf_param_init_double(pump, GN_LEAF_PWM_PARAM_CHANNEL, 2);
	gn_leaf_param_init_double(pump, GN_LEAF_PWM_PARAM_POWER, 0);

}

