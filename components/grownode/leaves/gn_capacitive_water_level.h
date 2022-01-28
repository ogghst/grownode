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

#ifndef COMPONENTS_GROWNODE_LEAVES_GN_CAPACITIVE_WATER_LEVEL_H_
#define COMPONENTS_GROWNODE_LEAVES_GN_CAPACITIVE_WATER_LEVEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_LEAF_CWL_TYPE[] = "cwl";

//parameters
static const char GN_CWL_PARAM_ACTIVE[] = "active"; /*!< whether the sensor is running*/
static const char GN_CWL_PARAM_TOUCH_CHANNEL[] = "touch_ch"; /*!< touch channel for sensing. stored, read/write */
static const char GN_CWL_PARAM_MAX_LEVEL[] = "max_level"; /*!< minimum water level. stored, read/write */
static const char GN_CWL_PARAM_MIN_LEVEL[] = "min_level"; /*!< maximum water level. stored, read/write */
static const char GN_CWL_PARAM_ACT_LEVEL[] = "act_level"; /*!< actual water level. volatile, read only */
static const char GN_CWL_PARAM_TRG_HIGH[] = "trg_hig"; /*!< water level above max level = 1. volatile, read only */
static const char GN_CWL_PARAM_TRG_LOW[] = "trg_low"; /*!<  water level below min level = 1. volatile, read only */
static const char GN_CWL_PARAM_UPDATE_TIME_SEC[] = "upd_time_sec"; /*!< seconds between sensor sampling. stored, read/write */

gn_leaf_descriptor_handle_t gn_capacitive_water_level_config(gn_leaf_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* COMPONENTS_GROWNODE_LEAVES_GN_CAPACITIVE_WATER_LEVEL_H_ */
