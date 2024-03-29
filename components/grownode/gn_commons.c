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

/**
 * put the bool value into the event payload
 */
gn_err_t gn_bool_to_event_payload(bool val, gn_leaf_parameter_event_handle_t evt) {

	ESP_LOGD(TAG, "gn_event_from_bool: val='%d'", val);
	memcpy(evt->data, &val, sizeof(bool));
	evt->data_len = sizeof(bool);
	//evt->data[0] = val;
	//evt->data_len = sizeof(bool);
	return GN_RET_OK;

}

/**
 * put the double value into the event payload
 */
gn_err_t gn_double_to_event_payload(double val, gn_leaf_parameter_event_handle_t evt) {

	memcpy(evt->data, &val, sizeof(double));
	evt->data_len = sizeof(double);
	return GN_RET_OK;

}

/**
 * put the string value into the event payload
 */
gn_err_t gn_string_to_event_payload(char *val, int val_len, gn_leaf_parameter_event_handle_t evt) {

	strncpy(evt->data, val, val_len);
	return GN_RET_OK;
}

/**
 * extract the payload from the event and puts in the pointer
 * @return
 */
gn_err_t gn_event_payload_to_bool(gn_leaf_parameter_event_t evt, bool *_ret) {

	memcpy(_ret, evt.data, sizeof(bool));
	return GN_RET_OK;
}

gn_err_t gn_event_payload_to_double(gn_leaf_parameter_event_t evt, double *_ret) {

	memcpy(_ret, evt.data, sizeof(double));
	return GN_RET_OK;

}

gn_err_t gn_event_payload_to_string(gn_leaf_parameter_event_t evt, char *_ret, int _ret_len) {

	strncpy(_ret, evt.data, _ret_len);
	return GN_RET_OK;
}

#ifdef __cplusplus
}
#endif //__cplusplus

