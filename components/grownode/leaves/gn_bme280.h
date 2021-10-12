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

#ifndef MAIN_GN_BME280_H_
#define MAIN_GN_BME280_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_LEAF_BME280_TYPE[GN_LEAF_DESC_TYPE_SIZE] = "bme280";

//parameters
static const char GN_BME280_PARAM_ACTIVE[] = "active"; /*!< whether the sensor is running*/
static const char GN_BME280_PARAM_SDA[] = "SDA"; /*!< gpio number for I2C master clock*/
static const char GN_BME280_PARAM_SCL[] = "SCL";  /*!< gpio number for I2C master data*/
static const char GN_BME280_PARAM_UPDATE_TIME_SEC[] = "upd_time_sec"; /*!< seconds between sensor sampling */

static const char GN_BME280_PARAM_TEMP[] = "temp"; /*!< temperature recorded */
static const char GN_BME280_PARAM_HUM[] = "hum"; /*!< humidity recorded */
static const char GN_BME280_PARAM_PRESS[] = "press"; /*!< pressure recorded */

gn_leaf_descriptor_handle_t gn_bme280_config(gn_leaf_config_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_BME280_H_ */
