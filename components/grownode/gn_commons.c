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
#include <errno.h>

#include "esp_log.h"
#include "gn_commons.h"
#include "grownode_intl.h"

#define TAG "gn_commons"

const char *gn_config_status_descriptions[14] = { "Not Initialized",
		"Initializing", "Generic Error", "Network Error", "Server Error",
		"Completed", "Started", "Bad Firmware URL", "Bad Provisioning Password",
		"Bad Server Base Topic", "Bad Server URL", "Bad Server Keepalive",
		"Bad SNTP URL", "Bad Server Discovery Prefix" };

inline size_t gn_leaf_event_mask_param(gn_leaf_parameter_event_handle_t evt,
		gn_leaf_param_handle_t param) {

	if (!evt || !param)
		return 1;

	gn_leaf_handle_intl_t _leaf_config =
			((gn_leaf_param_handle_intl_t) param)->leaf;

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

//validators

const double min = DBL_MIN;
const double zero = 0;
const double max = DBL_MAX;

const bool bool_false = false;
const bool bool_true = !bool_false;

gn_leaf_param_validator_result_t gn_validator_double_positive(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR_GENERIC;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "gn_validator_double_positive - param: %f", _p1);

	if (**(double**) param_value < zero) {
		memcpy(&param_value, &zero, sizeof(double));
		return GN_LEAF_PARAM_VALIDATOR_ERROR_BELOW_MIN;
	} else if (**(double**) param_value >= max) {
		memcpy(&param_value, &max, sizeof(double));
		return GN_LEAF_PARAM_VALIDATOR_ERROR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "gn_validator_double_positive - param: %f", _p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

gn_leaf_param_validator_result_t gn_validator_double(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR_GENERIC;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "gn_validator_double - param: %f", _p1);

	if (**(double**) param_value < min) {
		memcpy(&param_value, &min, sizeof(double));
		return GN_LEAF_PARAM_VALIDATOR_ERROR_BELOW_MIN;
	} else if (**(double**) param_value >= max) {
		memcpy(&param_value, &max, sizeof(double));
		return GN_LEAF_PARAM_VALIDATOR_ERROR_ABOVE_MAX;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_interval_validator - param: %f", _p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

gn_leaf_param_validator_result_t gn_validator_boolean(
		gn_leaf_param_handle_t param, void **param_value) {

	bool val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR_GENERIC;

	bool _p1 = **(bool**) param_value;
	ESP_LOGD(TAG, "gn_validator_boolean - param: %d", _p1);

	if (_p1 != false) {
		memcpy(&param_value, &bool_true, sizeof(bool_true));
	} else {
		memcpy(&param_value, &bool_false, sizeof(bool_false));
	}

	ESP_LOGD(TAG, "gn_validator_boolean - param: %d", _p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;

}

gn_err_t gn_bool_from_event(gn_leaf_parameter_event_t evt, bool *_ret) {

	ESP_LOGD(TAG, "gn_bool_from_payload: data='%.*s', len = %d", evt.data_len,
			evt.data, evt.data_len);

	memcpy(_ret, evt.data, sizeof(bool));
	/*
	if (evt.data[0] == true) {
		*_ret = true;
	} else if (evt.data[0] == false) {
		*_ret = false;
	} else {
		ESP_LOGW(TAG, "warning: payload '%.*s' cannot be converted as boolean",
				evt.data_len, evt.data);
		return GN_RET_ERR;
	}*/

	ESP_LOGD(TAG, "gn_bool_from_event: '%d'", *_ret);

	return GN_RET_OK;
}

gn_err_t gn_double_from_event(gn_leaf_parameter_event_t evt, double *_ret) {

	memcpy(_ret, evt.data, sizeof(double));
	/*
	char *payload = calloc(evt.data_len, sizeof(char));
	strncpy(payload, evt.data, evt.data_len);

	char *eptr;
	double result = strtod(payload, &eptr);

	if (result == 0) {
		If the value provided was out of range, display a warning message
		if (errno == ERANGE) {

			ESP_LOGW(TAG,
					"warning: payload '%.*s' cannot be converted as double",
					evt.data_len, evt.data);
			free(payload);
			return GN_RET_ERR;
		}
	}

	memcpy(_ret, &result, sizeof(double));

	free(payload);
	return GN_RET_OK;
	*/
	ESP_LOGD(TAG, "gn_double_from_event: '%f'", *_ret);

	return GN_RET_OK;

}

/*
gn_err_t gn_int_from_event(gn_leaf_parameter_event_t evt, int *_ret) {

	char *payload = calloc(evt.data_len, sizeof(char));
	strncpy(payload, evt.data, evt.data_len);

	char *eptr;
	long int _result = (int)strtol(payload, &eptr, 10);
	int result;
	if (_result >= INT_MIN && result <= INT_MAX) {
	    result = _result;
	} else {
		free(payload);
		return GN_RET_VALUE_OUT_OF_LIMIT;
	}

	if (result == 0) {
		// If the value provided was out of range, display a warning message
		if (errno == ERANGE) {

			ESP_LOGW(TAG,
					"warning: payload '%.*s' cannot be converted as double",
					evt.data_len, evt.data);
			free(payload);
			return GN_RET_ERR;
		}
	}

	memcpy(_ret, &result, sizeof(int));

	free(payload);
	return GN_RET_OK;
}
*/

gn_err_t gn_str_from_event(gn_leaf_parameter_event_t evt, char *_ret, int _ret_len) {
	strncpy(_ret, evt.data, _ret_len);
	ESP_LOGD(TAG, "gn_str_from_event: '%s'", _ret);
	return GN_RET_OK;
}

#ifdef __cplusplus
}
#endif //__cplusplus

