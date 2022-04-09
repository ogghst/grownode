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

#ifndef MAIN_GN_BH1750_H_
#define MAIN_GN_BH1750_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

//define type
static const char GN_LEAF_BH1750_TYPE[] = "bh1750";

//parameters
static const char GN_BH1750_PARAM_ACTIVE[] = "active"; /*!< @brief whether the sensor is running

 | Name                                     |    Type                 | 	Access		                    |	Storage		                     |	Default          | Description	     |
 | ---------------------------------------- |  ---------------------- |---------------------------------    | ---------------------------------- | ----------------- | ----------------- |
 | GN_BH1750_PARAM_ACTIVE			        | GN_VAL_TYPE_BOOLEAN     | GN_LEAF_PARAM_ACCESS_NETWORK        | GN_LEAF_PARAM_STORAGE_PERSISTED    |  true             |   whether the sensor is running                |

 */

static const char GN_BH1750_PARAM_SDA[] = "SDA"; /*!< @brief gpio number for I2C master clock

 | Name                                     |    Type                 | 	Access		                    |	Storage		                     |	Default          | Description	     |
 | ---------------------------------------- |  ---------------------- |---------------------------------    | ---------------------------------- | ----------------- | ----------------- |
 | GN_BH1750_PARAM_SDA   			        | GN_VAL_TYPE_DOUBLE      | GN_LEAF_PARAM_ACCESS_NETWORK        | GN_LEAF_PARAM_STORAGE_PERSISTED    |  21               |   gpio number for I2C master data                |

 */

static const char GN_BH1750_PARAM_SCL[] = "SCL"; /*!< @brief gpio number for I2C master data

 | Name                                     |    Type                 | 	Access		                    |	Storage		                     |	Default          | Description	     |
 | ---------------------------------------- |  ---------------------- |---------------------------------    | ---------------------------------- | ----------------- | ----------------- |
 | GN_BH1750_PARAM_SCL   			        | GN_VAL_TYPE_DOUBLE      | GN_LEAF_PARAM_ACCESS_NETWORK        | GN_LEAF_PARAM_STORAGE_PERSISTED    |  22               |   gpio number for I2C master clock                |

 */

static const char GN_BH1750_PARAM_UPDATE_TIME_SEC[] = "upd_time_sec"; /*!< @brief seconds between sensor sampling

 | Name                                     |    Type                 | 	Access		                    |	Storage		                     |	Default          | Description	     |
 | ---------------------------------------- |  ---------------------- |---------------------------------    | ---------------------------------- | ----------------- | ----------------- |
 | GN_BH1750_PARAM_UPDATE_TIME_SEC         | GN_VAL_TYPE_DOUBLE      | GN_LEAF_PARAM_ACCESS_NETWORK         | GN_LEAF_PARAM_STORAGE_PERSISTED    |  120               |   seconds between sensor sampling                |

 */

static const char GN_BH1750_PARAM_LUX[] = "lux"; /*!< @brief temperature recorded

 | Name                                     |    Type                 | 	Access		                    |	Storage		                     |	Default          | Description	     |
 | ---------------------------------------- |  ---------------------- |---------------------------------    | ---------------------------------- | ----------------- | ----------------- |
 | GN_BH1750_PARAM_TEMP                     | GN_VAL_TYPE_DOUBLE      | GN_LEAF_PARAM_ACCESS_NETWORK         | GN_LEAF_PARAM_STORAGE_VOLATILE    |  0                |   lux recorded                |

 */

/**
 * @brief configures the leaf
 */
gn_leaf_descriptor_handle_t gn_bh1750_config(gn_leaf_handle_t leaf_config);

gn_leaf_handle_t gn_bh1750_fastcreate(gn_node_handle_t node, const char *leaf_name, int sda, int scl, double update_time);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_BH1750_H_ */
