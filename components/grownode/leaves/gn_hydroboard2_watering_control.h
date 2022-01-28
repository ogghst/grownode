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

#ifndef HYDROBOARD2_GN_WATERING_CONTROL_H_
#define HYDROBOARD2_GN_WATERING_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

static const int32_t GN_HYDROBOARD2_MIN_WATERING_INTERVAL = 10; //10 secs
static const int32_t GN_HYDROBOARD2_MAX_WATERING_INTERVAL = 60*60*24*30; //one month

//TODO maybe transfer this parameter on leaf itself?
static const int32_t GN_HYDROBOARD2_MIN_WATERING_TIME = 1; //1 sec
static const int32_t GN_HYDROBOARD2_MAX_WATERING_TIME = 60*60; //one hour

static const int32_t GN_HYDROBOARD2_MIN_WATERING_TARGET_TEMP = 12;
static const int32_t GN_HYDROBOARD2_MAX_WATERING_TARGET_TEMP = 30;

//TODO make a parameter (maybe on leaf itself?)
static const int64_t GN_HYDROBOARD2_MAX_HCC_CYCLE_TIME_SEC = 60 * 5;

//define type
static const char GN_LEAF_HYDROBOARD2_WATERING_CONTROL_TYPE[] = "watering_control";


static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_FAN[] = "leaf_PLT_FAN";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_PUMP[] = "leaf_PLT_PUMP";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_PUMP[] = "leaf_WAT_PUMP";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_COOL[] = "leaf_PLT_COOL";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_PLT_HOT[] = "leaf_PLT_HOT";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_ENV_FAN[] = "leaf_ENV_FAN";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_BME280[] = "leaf_BME280";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_DS18B20[] = "leaf_DS18B20";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_WAT_LEV[] = "leaf_WAT_LEV";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_1[] = "leaf_LIGHT_1";
static const char  GN_HYDROBOARD2_WAT_CTR_PARAM_LEAF_LIGHT_2[] = "leaf_LIGHT_2";


static const char GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_INTERVAL_SEC[] = "wat_int_sec"; //time between two watering cycles
static const char GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TIME_SEC[] = "wat_time_sec"; //time of one watering cycle
static const char GN_HYDROBOARD2_WAT_CTR_PARAM_WATERING_TARGET_TEMP[] = "wat_t_temp";
static const char GN_HYDROBOARD2_WAT_CTR_PARAM_ACTIVE[] = "active";

static const int32_t GN_HYDROBOARD2_WAT_CTR_CYCLE_TIME_MS = 1000L;

gn_leaf_descriptor_handle_t gn_hb2_watering_control_config(gn_leaf_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* HYDROBOARD2_GN_WATERING_CONTROL_H_ */
