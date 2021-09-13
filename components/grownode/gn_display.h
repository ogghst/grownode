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

#ifndef MAIN_GN_DISPLAY_H_
#define MAIN_GN_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

esp_err_t gn_init_display(gn_config_handle_t conf);

void gn_display_leaf_start(gn_leaf_config_handle_t leaf_config);

gn_display_container_t gn_display_setup_leaf_display(gn_leaf_config_handle_t leaf_config);

BaseType_t gn_display_leaf_refresh_start();

BaseType_t gn_display_leaf_refresh_end();

static SemaphoreHandle_t _gn_xGuiSemaphore;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_DISPLAY_H_ */
