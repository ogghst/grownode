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

void gn_configure_hydroboard2(gn_node_config_handle_t node) {

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
	const char *PLT_FAN = "plt_fan";
	const char *PLT_PUMP = "plt_pump";
	const char *WAT_PUMP = "wat_pump";
	const char *PLT_COOL = "plt_b";
	const char *PLT_HOT = "plt_a";
	const char *ENV_FAN = "env_fan";
	const char *BME280 = "env_thp";
	const char *DS18B20 = "temps";
	const char *WAT_LEV = "wat_lev";
	const char *LIGHT_1 = "lig_1";
	const char *LIGHT_2 = "lig_2";

	gn_leaf_config_handle_t lights1in = gn_leaf_create(node, LIGHT_1,
			gn_gpio_config, 4096);
	gn_leaf_param_init_double(lights1in, GN_GPIO_PARAM_GPIO, 25);
	gn_leaf_param_init_bool(lights1in, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(lights1in, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_config_handle_t lights2in = gn_leaf_create(node, LIGHT_2,
			gn_gpio_config, 4096);
	gn_leaf_param_init_double(lights2in, GN_GPIO_PARAM_GPIO, 33);
	gn_leaf_param_init_bool(lights2in, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(lights2in, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_config_handle_t plt_a = gn_leaf_create(node, PLT_HOT,
			gn_gpio_config, 4096);
	gn_leaf_param_init_double(plt_a, GN_GPIO_PARAM_GPIO, 5);
	gn_leaf_param_init_bool(plt_a, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(plt_a, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_config_handle_t plt_b = gn_leaf_create(node, PLT_COOL,
			gn_gpio_config, 4096);
	gn_leaf_param_init_double(plt_b, GN_GPIO_PARAM_GPIO, 23);
	gn_leaf_param_init_bool(plt_b, GN_GPIO_PARAM_INVERTED, true);
	gn_leaf_param_init_bool(plt_b, GN_GPIO_PARAM_TOGGLE, false);

	gn_leaf_config_handle_t wat_pump = gn_leaf_create(node, WAT_PUMP,
			gn_leaf_pwm_config, 4096);
	gn_leaf_param_init_bool(wat_pump, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(wat_pump, GN_LEAF_PWM_PARAM_GPIO, 16);
	gn_leaf_param_init_double(wat_pump, GN_LEAF_PWM_PARAM_CHANNEL, 0);
	gn_leaf_param_init_double(wat_pump, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_config_handle_t plt_pump = gn_leaf_create(node, PLT_PUMP,
			gn_leaf_pwm_config, 4096);
	gn_leaf_param_init_bool(plt_pump, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(plt_pump, GN_LEAF_PWM_PARAM_GPIO, 19);
	gn_leaf_param_init_double(plt_pump, GN_LEAF_PWM_PARAM_CHANNEL, 1);
	gn_leaf_param_init_double(plt_pump, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_config_handle_t plt_fan = gn_leaf_create(node, PLT_FAN,
			gn_leaf_pwm_config, 4096);
	gn_leaf_param_init_bool(plt_fan, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(plt_fan, GN_LEAF_PWM_PARAM_GPIO, 18);
	gn_leaf_param_init_double(plt_fan, GN_LEAF_PWM_PARAM_CHANNEL, 2);
	gn_leaf_param_init_double(plt_fan, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_config_handle_t env_fan = gn_leaf_create(node, ENV_FAN,
			gn_leaf_pwm_config, 4096);
	gn_leaf_param_init_bool(env_fan, GN_LEAF_PWM_PARAM_TOGGLE, false);
	gn_leaf_param_init_double(env_fan, GN_LEAF_PWM_PARAM_GPIO, 27);
	gn_leaf_param_init_double(env_fan, GN_LEAF_PWM_PARAM_CHANNEL, 3);
	gn_leaf_param_init_double(env_fan, GN_LEAF_PWM_PARAM_POWER, 0);

	gn_leaf_config_handle_t wat_lev = gn_leaf_create(node, WAT_LEV,
			gn_capacitive_water_level_config, 4096);
	gn_leaf_param_init_bool(wat_lev, GN_CWL_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(wat_lev, GN_CWL_PARAM_TOUCH_CHANNEL, 1);
	gn_leaf_param_init_double(wat_lev, GN_CWL_PARAM_UPDATE_TIME_SEC, 10);
	gn_leaf_param_init_double(wat_lev, GN_CWL_PARAM_MIN_LEVEL, 0);
	gn_leaf_param_init_double(wat_lev, GN_CWL_PARAM_MAX_LEVEL, 2048);

	gn_leaf_config_handle_t env_thp = gn_leaf_create(node, BME280,
			gn_bme280_config, 8192);
	gn_leaf_param_init_double(env_thp, GN_BME280_PARAM_SDA, 21);
	gn_leaf_param_init_double(env_thp, GN_BME280_PARAM_SCL, 22);
	gn_leaf_param_init_bool(env_thp, GN_BME280_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(env_thp, GN_BME280_PARAM_UPDATE_TIME_SEC, 10);

	gn_leaf_config_handle_t temps = gn_leaf_create(node, DS18B20,
			gn_ds18b20_config, 4096);
	gn_leaf_param_init_double(temps, GN_DS18B20_PARAM_GPIO, 4);
	gn_leaf_param_init_bool(temps, GN_DS18B20_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(temps, GN_DS18B20_PARAM_UPDATE_TIME_SEC, 5);

	gn_leaf_config_handle_t watering_control = gn_leaf_create(node,
			"watering_control", gn_hb2_watering_control_config, 4096);
	gn_leaf_param_init_double(watering_control,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC, 60 * 1);
	gn_leaf_param_init_double(watering_control,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TIME_SEC, 20);
	gn_leaf_param_init_double(watering_control,
			GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TARGET_TEMP, 20);
	gn_leaf_param_init_bool(watering_control,
			GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE, true);

	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_FAN, PLT_FAN);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_PUMP, PLT_PUMP);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_PUMP, WAT_PUMP);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_COOL, PLT_COOL);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_HOT, PLT_HOT);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_ENV_FAN, ENV_FAN);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_BME280, BME280);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_DS18B20, DS18B20);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_LEV, WAT_LEV);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_1, LIGHT_1);
	gn_leaf_param_init_string(watering_control, GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_2, LIGHT_2);

	gn_leaf_config_handle_t led = gn_leaf_create(node, "led",
			gn_leaf_status_led_config, 4096);
	gn_leaf_param_init_double(led, GN_LEAF_STATUS_LED_PARAM_GPIO, 32);

}

