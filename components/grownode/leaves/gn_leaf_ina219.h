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

#ifndef MAIN_GN_LEAF_INA219_H_
#define MAIN_GN_LEAF_INA219_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_LEAF_INA219_TYPE[] = "ina219";

//parameters
static const char GN_LEAF_INA219_PARAM_ACTIVE[] = "active"; /*!< whether INA219 shall be enabled */
static const char GN_LEAF_INA219_PARAM_IP[] = "ip"; /*!< ip address of the server */
static const char GN_LEAF_INA219_PARAM_PORT[] = "port"; /*!< port of the server */
static const char GN_LEAF_INA219_PARAM_SAMPLING_CYCLES[] = "samp_cycles"; /*!< sampling cycles */
static const char GN_LEAF_INA219_PARAM_SAMPLING_INTERVAL[] = "samp_interval"; /*!< sampling interval, (approx) in msec */
static const char GN_LEAF_INA219_PARAM_SDA[] = "sda"; /*!< SDA PIN */
static const char GN_LEAF_INA219_PARAM_SCL[] = "scl"; /*!< SCL PIN */
static const char GN_LEAF_INA219_PARAM_WORKING_MODE[] = "working_mode"; /*!< 0 = through UDP via influxdb protocol, 1 = parameters, 2 both*/
static const char GN_LEAF_INA219_PARAM_POWER[] = "power"; /*!< last power measured, in mW*/
static const char GN_LEAF_INA219_PARAM_VOLTAGE[] = "voltage"; /*!< last voltage measured, in mV */
static const char GN_LEAF_INA219_PARAM_SHUNT_VOLTAGE[] = "shunt_voltage"; /*!< last shunt voltage measured, in mV */
static const char GN_LEAF_INA219_PARAM_BUS_VOLTAGE[] = "bus_voltage"; /*!< last bus voltage measured, in mV */
static const char GN_LEAF_INA219_PARAM_CURRENT[] = "current"; /*!< last current measured, in mA */




gn_leaf_descriptor_handle_t gn_leaf_ina219_config(gn_leaf_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_LEAF_INA219_H_ */
