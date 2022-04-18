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
#include "synapses/gn_hydroboard2_watering_control.h"
#include "gn_leaf_status_led.h"

#include "gn_hydroboard2.h"

void gn_configure_nft1(gn_node_handle_t node) {

	//leaves
	esp_log_level_set("gn_leaf_gpio", ESP_LOG_INFO);
	esp_log_level_set("gn_leaf_pwm", ESP_LOG_DEBUG);

	//leaves names
	const char *PUMP = "pump";
	const char *FAN = "fan";
	const char *LIGHTS = "lights";
	const char *SYN = "nft1_syn";

	gn_leaf_handle_t fan = gn_leaf_create(node, FAN, gn_gpio_config,
			4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(fan, GN_GPIO_PARAM_GPIO, 26);
	gn_leaf_param_init_bool(fan, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(fan, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t lights = gn_leaf_create(node, LIGHTS,
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(lights, GN_GPIO_PARAM_GPIO, 18);
	gn_leaf_param_init_bool(lights, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(lights, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t pump = gn_leaf_create(node, PUMP, gn_leaf_pwm_config, 4096,
			GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_bool(pump, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(pump, GN_LEAF_PWM_PARAM_GPIO, 5);
	gn_leaf_param_init_double(pump, GN_LEAF_PWM_PARAM_CHANNEL, 2);
	gn_leaf_param_init_double(pump, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_handle_t nft1_syn = gn_leaf_create(node, SYN,
			gn_syn_nft1_control_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_INTERVAL_SEC, 10);
	gn_leaf_param_init_double(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC, 2);
	gn_leaf_param_init_bool(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_ENABLE, true);
	gn_leaf_param_init_string(nft1_syn, GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF, PUMP);

}

