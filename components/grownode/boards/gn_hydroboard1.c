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
#include "synapses/gn_watering_control.h"

#include "gn_hydroboard1.h"

void gn_configure_hydroboard1(gn_node_handle_t node) {

	gn_leaf_handle_t lights1in = gn_leaf_create(node, "lights1in",
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(lights1in, GN_GPIO_PARAM_GPIO, 25);
	gn_leaf_param_init_bool(lights1in, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t lights2in = gn_leaf_create(node, "lights2in",
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(lights2in, GN_GPIO_PARAM_GPIO, 5);
	gn_leaf_param_init_bool(lights2in, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t plt_a = gn_leaf_create(node, "plt_a",
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(plt_a, GN_GPIO_PARAM_GPIO, 23);
	gn_leaf_param_init_bool(plt_a, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t plt_b = gn_leaf_create(node, "plt_b",
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(plt_b, GN_GPIO_PARAM_GPIO, 17);
	gn_leaf_param_init_bool(plt_b, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t waterpumpin = gn_leaf_create(node, "waterpumpin",
			gn_gpio_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(waterpumpin, GN_GPIO_PARAM_GPIO, 19);
	gn_leaf_param_init_bool(waterpumpin, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_handle_t waterlevelin = gn_leaf_create(node, "waterlevelin",
			gn_capacitive_water_level_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_bool(waterlevelin, GN_CWL_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(waterlevelin, GN_CWL_PARAM_TOUCH_CHANNEL, 2);
	gn_leaf_param_init_double(waterlevelin, GN_CWL_PARAM_UPDATE_TIME_SEC, 10);
	gn_leaf_param_init_double(waterlevelin, GN_CWL_PARAM_MIN_LEVEL, 0);
	gn_leaf_param_init_double(waterlevelin, GN_CWL_PARAM_MAX_LEVEL, 2048);

	gn_leaf_handle_t hcc_speed = gn_leaf_create(node, "hcc",
			gn_pump_hs_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_bool(hcc_speed, GN_PUMP_HS_PARAM_CHANNEL, 0);
	gn_leaf_param_init_double(hcc_speed, GN_PUMP_HS_PARAM_GPIO_POWER, 18);
	gn_leaf_param_init_double(hcc_speed, GN_PUMP_HS_PARAM_POWER, 0);
	gn_leaf_param_init_double(hcc_speed, GN_PUMP_HS_PARAM_GPIO_TOGGLE, 26);
	gn_leaf_param_init_bool(hcc_speed, GN_PUMP_HS_PARAM_TOGGLE, false);

	gn_leaf_handle_t fan_speed = gn_leaf_create(node, "fan",
			gn_pump_hs_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_bool(fan_speed, GN_PUMP_HS_PARAM_CHANNEL, 1);
	gn_leaf_param_init_double(fan_speed, GN_PUMP_HS_PARAM_GPIO_POWER, 27);
	gn_leaf_param_init_double(fan_speed, GN_PUMP_HS_PARAM_POWER, 0);
	gn_leaf_param_init_double(fan_speed, GN_PUMP_HS_PARAM_GPIO_TOGGLE, 33);
	gn_leaf_param_init_bool(fan_speed, GN_PUMP_HS_PARAM_TOGGLE, false);

	gn_leaf_handle_t bme280 = gn_leaf_create(node, "bme280",
			gn_bme280_config, 8192, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(bme280, GN_BME280_PARAM_SDA, 21);
	gn_leaf_param_init_double(bme280, GN_BME280_PARAM_SCL, 22);
	gn_leaf_param_init_bool(bme280, GN_BME280_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(bme280, GN_BME280_PARAM_UPDATE_TIME_SEC, 10);

	gn_leaf_handle_t ds18b20 = gn_leaf_create(node, "ds18b20",
			gn_ds18b20_config, 4096, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_double(ds18b20, GN_DS18B20_PARAM_GPIO, 4);
	gn_leaf_param_init_bool(ds18b20, GN_DS18B20_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(ds18b20, GN_DS18B20_PARAM_UPDATE_TIME_SEC, 5);

	/*
	 gn_leaf_config_handle_t watering_control = gn_leaf_create(node,
	 "watering_control", gn_watering_control_config, 4096);
	 gn_leaf_param_init_double(watering_control,
	 GN_WAT_CTR_PARAM_WATERING_INTERVAL_SEC, 60 * 1);
	 gn_leaf_param_init_double(watering_control,
	 GN_WAT_CTR_PARAM_WATERING_TIME_SEC, 20);
	 gn_leaf_param_init_double(watering_control,
	 GN_WAT_CTR_PARAM_WATERING_TARGET_TEMP, 20);
	 gn_leaf_param_init_bool(watering_control, GN_WAT_CTR_PARAM_ACTIVE, true);
	 */

}

