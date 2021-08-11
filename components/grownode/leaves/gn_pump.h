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
//#include "gn_mqtt_protocol.h"

void gn_pump_task(gn_leaf_config_handle_t leaf_config);

void gn_pump_display_config(gn_leaf_config_handle_t leaf_config, void* leaf_container);

//void gn_pump_display_task(gn_leaf_config_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_PUMP_H_ */
