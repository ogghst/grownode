// Copyright 2021 Adamo Ferro (adamo@af-projects.it)
// based on the work of Nicola Muratori (nicola.muratori@gmail.com)
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

#ifndef COMPONENTS_GROWNODE_LEAVES_GN_LEAF_MUX_CMS_H_
#define COMPONENTS_GROWNODE_LEAVES_GN_LEAF_MUX_CMS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

#define GN_MUX_CMS_ADDR_BITS 4		// TODO find a way to fully parameterize leaf depending on this define (in particular, the address GPOI handling)
#define GN_MUX_CMS_MAX_SENSORS 16

#define GN_MUX_CMS_ENABLE_LEVEL 0

//define type
static const char GN_LEAF_MUX_CMS_TYPE[] = "mux_cms";

//parameters
static const char GN_MUX_CMS_PARAM_ACTIVE[] = "active"; /*!< whether the sensor is running*/
static const char GN_MUX_CMS_PARAM_N_SENSORS[] = "n_sensors"; /*!< number of capacitive moisture sensors to handle, max 16. stored, read/write */
static const char GN_MUX_CMS_PARAM_ADC_CHANNEL[] = "adc_ch"; /*!< adc channel for sensing. stored, read/write */
static const char GN_MUX_CMS_PARAM_SEL_GPIO0[] = "sel_gpio0"; /*!< gpios used for mux channel selection. stored, read/write */
static const char GN_MUX_CMS_PARAM_SEL_GPIO1[] = "sel_gpio1";
static const char GN_MUX_CMS_PARAM_SEL_GPIO2[] = "sel_gpio2";
static const char GN_MUX_CMS_PARAM_SEL_GPIO3[] = "sel_gpio3";
static const char GN_MUX_CMS_PARAM_EN_GPIO[] = "en_gpio"; /*!< gpio used for enabling mux. stored, read/write */
static const char GN_MUX_CMS_PARAM_SENSOR_NAMES[GN_MUX_CMS_MAX_SENSORS][6] = { "cms01", "cms02",
		"cms03", "cms04", "cms05", "cms06", "cms07", "cms08", "cms09", "cms10", "cms11", "cms12",
		"cms13", "cms14", "cms15", "cms16"};
static const char GN_MUX_CMS_PARAM_MAX_MOIST[] = "max_moist"; /*!< minimum moisture. stored, read/write */
static const char GN_MUX_CMS_PARAM_MIN_MOIST[] = "min_moist"; /*!< maximum moisture. stored, read/write */
static const char GN_MUX_CMS_PARAM_TRG_HIGH[GN_MUX_CMS_MAX_SENSORS][10] = { "trg_hig01", "trg_hig02",
		"trg_hig03", "trg_hig04", "trg_hig05", "trg_hig06", "trg_hig07", "trg_hig08", "trg_hig09",
		"trg_hig10", "trg_hig11", "trg_hig12", "trg_hig13", "trg_hig14", "trg_hig15", "trg_hig16" }; /*!< humidity level above max level = 1. volatile, read only */
static const char GN_MUX_CMS_PARAM_TRG_LOW[GN_MUX_CMS_MAX_SENSORS][10] = { "trg_low01", "trg_low02",
		"trg_low03", "trg_low04", "trg_low05", "trg_low06", "trg_low07", "trg_low08", "trg_low09",
		"trg_low10", "trg_low11", "trg_low12", "trg_low13", "trg_low14", "trg_low15", "trg_low16" }; /*!<  humidity level below min level = 1. volatile, read only */
static const char GN_MUX_CMS_PARAM_UPDATE_TIME_SEC[] = "upd_time_sec"; /*!< seconds between sensor sampling. stored, read/write */

gn_leaf_descriptor_handle_t gn_mux_cms_config(gn_leaf_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* COMPONENTS_GROWNODE_LEAVES_GN_LEAF_MUX_CMS_H_ */
