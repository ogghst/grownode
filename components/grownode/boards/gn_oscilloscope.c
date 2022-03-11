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
#include "gn_leaf_ina219.h"

#include "gn_oscilloscope.h"

#define TAG "gn_oscilloscope"



/**
 * @brief
 *
 */
void gn_configure_oscilloscope(gn_node_handle_t node) {

	esp_log_level_set("gn_leaf_ina219", esp_log_level_get(TAG));

	gn_leaf_handle_t ina219 = gn_leaf_create(node, "ina219", gn_leaf_ina219_config, 8192, GN_LEAF_TASK_PRIORITY);
	gn_leaf_param_init_bool(ina219, GN_LEAF_INA219_PARAM_ACTIVE, true);

	char* ip = calloc(16, sizeof(char));
	strncpy(ip, "192.168.1.20", 16);

	gn_leaf_param_init_string(ina219, GN_LEAF_INA219_PARAM_IP, ip);
	gn_leaf_param_init_double(ina219, GN_LEAF_INA219_PARAM_PORT, 8094);
	gn_leaf_param_init_double(ina219, GN_LEAF_INA219_PARAM_SAMPLING_TICKS, 50);
	gn_leaf_param_init_double(ina219, GN_LEAF_INA219_PARAM_SDA, 26);
	gn_leaf_param_init_double(ina219, GN_LEAF_INA219_PARAM_SCL, 27);


}

