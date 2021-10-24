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

#ifndef MAIN_GN_PUMP_H_
#define MAIN_GN_PUMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_LEAF_PUMP_TYPE[GN_LEAF_DESC_TYPE_SIZE] = "pump";

//parameters
static const char GN_PUMP_PARAM_TOGGLE[] = "toggle"; /*!< 0 = off, 1 = on */
static const char GN_PUMP_PARAM_POWER[] = "power"; /*!< 0 = off, 1024 = maximum */
static const char GN_PUMP_PARAM_CHANNEL[] = "channel"; /*!< 0 = channel A, 1 = channel B */
static const char GN_PUMP_PARAM_GPIO_POWER[] = "gpio_power"; /*!< the GPIO to connect the pump (must be PWM capable) */
static const char GN_PUMP_PARAM_GPIO_TOGGLE[] = "gpio_toggle"; /*!< the GPIO to connect the pump (must be PWM capable) */


gn_leaf_descriptor_handle_t gn_pump_config(gn_leaf_config_handle_t leaf_config);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_PUMP_H_ */
