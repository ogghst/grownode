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

#ifndef MAIN_GN_LEAD_STATUS_LED_H_
#define MAIN_GN_LEAD_STATUS_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_LEAF_STATUS_LED_TYPE[] = "status_led";

//parameters
static const char GN_LEAF_STATUS_LED_PARAM_GPIO[] = "gpio"; /*!< the GPIO to connect the relay */

gn_leaf_descriptor_handle_t gn_leaf_status_led_config(gn_leaf_handle_t leaf_config);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_LEAD_STATUS_LED_H_ */
