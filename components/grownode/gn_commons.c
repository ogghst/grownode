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

#ifdef __cplusplus
extern "C" {
#endif

#include "float.h"

#include "esp_log.h"
#include "gn_commons.h"
#include "grownode_intl.h"

#define TAG "gn_commons"

inline size_t gn_leaf_event_mask_param(gn_leaf_parameter_event_handle_t evt,
		gn_leaf_param_handle_t param) {

	if (!evt || !param)
		return 1;

	gn_leaf_config_handle_intl_t _leaf_config =
			((gn_leaf_param_handle_intl_t) param)->leaf_config;

	ESP_LOGD(TAG, "gn_common_leaf_event_mask_param (%s, %s/%s)", evt->leaf_name,
			_leaf_config->name, ((gn_leaf_param_handle_intl_t ) param)->name);

	if (strcmp(evt->leaf_name, _leaf_config->name) == 0
			&& strcmp(evt->param_name,
					((gn_leaf_param_handle_intl_t) param)->name) == 0)
		return 0;
	return 1;
}

uint64_t inline gn_hash(const char *key) {
	uint64_t h = 525201411107845655ull;
	for (; *key; ++key) {
		h ^= *key;
		h *= 0x5bd1e9955bd1e995;
		h ^= h >> 47;
	}
	return h;
}

/*
 * hashes to a len bytes max
 */
void inline gn_hash_str(const char *key, char *buf, size_t len) {

	uint64_t r = gn_hash(key);
	snprintf(buf, len, "%llu", r);
	ESP_LOGD(TAG, "gn_common_hash_str(%s) - %s", key, buf);

}

gn_leaf_param_validator_result_t gn_validator_double_positive(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	double min = 0;
	double max = DBL_MAX;

	if (min > **(double**) param_value) {
		memcpy(param_value, &min, sizeof(min));
		return GN_LEAF_PARAM_VALIDATOR_BELOW_MIN;
	} else if (max < **(double**) param_value) {
		memcpy(param_value, &max, sizeof(max));
		return GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

gn_leaf_param_validator_result_t _gn_validator_double(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	double min = DBL_MIN;
	double max = DBL_MAX;

	if (min > **(double**) param_value) {
		memcpy(param_value, &min, sizeof(min));
		return GN_LEAF_PARAM_VALIDATOR_BELOW_MIN;
	} else if (max < **(double**) param_value) {
		memcpy(param_value, &max, sizeof(max));
		return GN_LEAF_PARAM_VALIDATOR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}






#ifdef __cplusplus
}
#endif //__cplusplus

