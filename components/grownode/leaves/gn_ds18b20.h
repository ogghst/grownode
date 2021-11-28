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

#ifndef MAIN_GN_DS18B20_H_
#define MAIN_GN_DS18B20_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

#define GN_DS18B20_MAX_SENSORS 4

//define type
static const char GN_LEAF_DS18B20_TYPE[] = "ds18b20";

static const int32_t MIN_UPDATE_TIME_SEC = 3;
static const int32_t MAX_UPDATE_TIME_SEC = 3600;

//parameters
static const char GN_DS18B20_PARAM_ACTIVE[] = "active"; /*!< whether the sensor is running*/
static const char GN_DS18B20_PARAM_UPDATE_TIME_SEC[16] = "upd_time_sec"; /*!< seconds between sensor sampling */
static const char GN_DS18B20_PARAM_GPIO[5] = "gpio"; /*!< GPIO connected to the temp sensor */
static const char GN_DS18B20_PARAM_SENSOR_NAMES[GN_DS18B20_MAX_SENSORS][6] = { "temp1", "temp2",
		"temp3", "temp4" };

gn_leaf_descriptor_handle_t gn_ds18b20_config(gn_leaf_config_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_DS18B20_H_ */
