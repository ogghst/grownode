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

#ifndef GN_SYN_NFT1_CONTROL_H_
#define GN_SYN_NFT1_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_SYN_NFT1_CONTROL_TYPE[] = "syn_nft1_ctrl";

//parameters
static const char GN_SYN_NFT1_CONTROL_PARAM_INTERVAL_SEC[] = "interval"; /*!< how many sec between two watering  */
static const char GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC[] = "duration"; /*!< how long a watering last */
static const char GN_SYN_NFT1_CONTROL_PARAM_ENABLE[] = "enable"; /*!< whether the watering is active */
static const char GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF[] = "pump"; /*!< the pump leaf to control (gn_pwm)*/

/**
 * this synapse takes a pump leaf (of type gn_pwm) and controls interval and duration of enable time
 */
gn_leaf_descriptor_handle_t gn_syn_nft1_control_config(gn_leaf_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* GN_SYN_NFT1_CONTROL_H_ */
