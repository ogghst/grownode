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

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_commons.h"

#include "gn_pwm.h"

#include "gn_syn_nft1_control.h"

#define TAG "gn_syn_nft1_control"

void gn_syn_nft1_control_task(gn_leaf_handle_t leaf_config);

typedef struct {
	gn_leaf_param_handle_t gn_syn_nft1_control_watering_interval_param;
	gn_leaf_param_handle_t gn_syn_nft1_control_watering_duration_param;
	gn_leaf_param_handle_t gn_syn_nft1_control_watering_enable_param;
	gn_leaf_param_handle_t gn_syn_nft1_control_pump_leaf_param;

	esp_timer_handle_t watering_cycle_start_timer;
	esp_timer_handle_t watering_cycle_stop_timer;

} gn_syn_nft1_control_data_t;

/**
 * stops timers and restart with new interval
 */
void _gn_syn_nft1_change_interval(gn_syn_nft1_control_data_t *data) {

	//stop timers
	esp_timer_stop(data->watering_cycle_start_timer);
	esp_timer_stop(data->watering_cycle_stop_timer);

	double interval;
	gn_leaf_param_get_value(data->gn_syn_nft1_control_watering_interval_param,
			&interval);

	//restart with new duration
	esp_timer_start_periodic(data->watering_cycle_start_timer,
			interval * 1000 * 1000);

}

/**
 * stops timers and restart with new duration
 */
void _gn_syn_nft1_change_duration(gn_syn_nft1_control_data_t *data) {

	_gn_syn_nft1_change_interval(data);

}

/*
 * what to do when watering time stops
 */
void watering_timer_stop_callback(void *arg) {

	ESP_LOGD(TAG, "watering_timer_stop_callback");

	gn_leaf_handle_t leaf = (gn_leaf_handle_t) arg;
	gn_node_handle_t node = gn_leaf_get_node(leaf);
	char pump_leaf[16];
	gn_leaf_param_get_string(leaf, GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF,
			&pump_leaf[0], 16);

	//gets the pump leaf
	gn_leaf_handle_t pump = gn_leaf_get_config_handle(node, pump_leaf);
	gn_leaf_param_set_bool(pump, GN_LEAF_PWM_PARAM_TOGGLE, false);

}

/*
 * what to do when it's watering time
 */
void watering_timer_start_callback(void *arg) {

	ESP_LOGD(TAG, "watering_timer_start_callback");

	gn_leaf_handle_t leaf = (gn_leaf_handle_t) arg;
	gn_node_handle_t node = gn_leaf_get_node(leaf);

	/*
	char nft1_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf, nft1_name);
	ESP_LOGD(TAG, "nft1 leaf name: %s",nft1_name);
	*/

	char pump_param_leaf[16];
	gn_leaf_param_get_string(leaf, GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF,
			pump_param_leaf, 16);

	//ESP_LOGD(TAG, "pump parameter leaf: %s",pump_param_leaf);

	double duration = 0;
	gn_leaf_param_get_double(leaf, GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC,
			&duration);

	bool enable;
	gn_leaf_param_get_bool(leaf, GN_SYN_NFT1_CONTROL_PARAM_ENABLE, &enable);

	//if enabled, starts the timer and then stops after duration interval
	if (enable) {

		//gets the pump leaf
		gn_leaf_handle_t pump = gn_leaf_get_config_handle(node, pump_param_leaf);

		/*
		char pump_leaf_name[GN_LEAF_NAME_SIZE];
		gn_leaf_get_name(pump, pump_leaf_name);

		ESP_LOGD(TAG, "pump leaf: %s",pump_leaf_name);
		*/

		gn_leaf_param_set_bool(pump, GN_LEAF_PWM_PARAM_TOGGLE, true);

		//gets the timer
		gn_leaf_descriptor_handle_t desc = gn_leaf_get_descriptor(leaf);
		gn_syn_nft1_control_data_t *data =
				(gn_syn_nft1_control_data_t*) desc->data;

		//stops after a while
		esp_timer_start_once(data->watering_cycle_stop_timer,
				duration * 1000 * 1000);
	}

}

gn_leaf_descriptor_handle_t gn_syn_nft1_control_config(
		gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_syn_nft1_control_config", leaf_name);

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_SYN_NFT1_CONTROL_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_syn_nft1_control_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_syn_nft1_control_data_t *data = malloc(
			sizeof(gn_syn_nft1_control_data_t));

	data->gn_syn_nft1_control_watering_duration_param = gn_leaf_param_create(
			leaf_config, GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 20 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED,
			gn_validator_double_positive);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_syn_nft1_control_watering_duration_param);

	data->gn_syn_nft1_control_watering_interval_param = gn_leaf_param_create(
			leaf_config, GN_SYN_NFT1_CONTROL_PARAM_INTERVAL_SEC,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 1200 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED,
			gn_validator_double_positive);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_syn_nft1_control_watering_interval_param);

	data->gn_syn_nft1_control_watering_enable_param = gn_leaf_param_create(
			leaf_config, GN_SYN_NFT1_CONTROL_PARAM_ENABLE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b =
					false }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_boolean);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_syn_nft1_control_watering_enable_param);

	data->gn_syn_nft1_control_pump_leaf_param = gn_leaf_param_create(
			leaf_config, GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF,
			GN_VAL_TYPE_STRING, (gn_val_t ) { .s = "" },
			GN_LEAF_PARAM_ACCESS_NODE_INTERNAL, GN_LEAF_PARAM_STORAGE_VOLATILE,
			NULL);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_syn_nft1_control_pump_leaf_param);

	const esp_timer_create_args_t watering_timer_start_args = { .callback =
			&watering_timer_start_callback, .name = "watering_start", .arg =
			leaf_config };

	esp_timer_create(&watering_timer_start_args,
			&data->watering_cycle_start_timer);

	const esp_timer_create_args_t watering_timer_stop_args = { .callback =
			&watering_timer_stop_callback, .name = "watering_stop", .arg =
			leaf_config };

	esp_timer_create(&watering_timer_stop_args,
			&data->watering_cycle_stop_timer);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_syn_nft1_control_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);
	ESP_LOGD(TAG, "[%s] gn_syn_nft1_control_task", leaf_name);

	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_syn_nft1_control_data_t *data =
			(gn_syn_nft1_control_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	double duration;
	gn_leaf_param_get_double(leaf_config,
			GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC, &duration);

	double interval;
	gn_leaf_param_get_double(leaf_config,
			GN_SYN_NFT1_CONTROL_PARAM_INTERVAL_SEC, &interval);

	bool enable;
	gn_leaf_param_get_bool(leaf_config, GN_SYN_NFT1_CONTROL_PARAM_ENABLE,
			&enable);

	char pump_leaf[16];
	gn_leaf_param_get_string(leaf_config, GN_SYN_NFT1_CONTROL_PARAM_PUMP_LEAF,
			pump_leaf, 16);

	ESP_LOGD(TAG,
			"configuring - duration %d, interval %d, enable %d, pump leaf '%s'",
			(int )duration, (int )interval, enable, pump_leaf);

	esp_timer_start_periodic(data->watering_cycle_start_timer,
			interval * 1000 * 1000);

	//task cycle
	while (true) {

		ESP_LOGD(TAG, "[%s] task cycle..", leaf_name);

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
		portMAX_DELAY) == pdPASS) {

			ESP_LOGD(TAG, "[%s] received message: %d", leaf_name, evt.id);

			//event arrived for this node
			switch (evt.id) {

				//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				ESP_LOGD(TAG, "[%s] request to update param %s, data = '%s'",
						leaf_name, evt.param_name, evt.data);

				if (gn_leaf_event_mask_param(&evt,
						data->gn_syn_nft1_control_watering_duration_param)
						== 0) {
					gn_leaf_param_force_double(leaf_config,
							GN_SYN_NFT1_CONTROL_PARAM_DURATION_SEC,
							atoi(evt.data));
					_gn_syn_nft1_change_duration(data);
				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_syn_nft1_control_watering_interval_param)
						== 0) {
					gn_leaf_param_force_double(leaf_config,
							GN_SYN_NFT1_CONTROL_PARAM_INTERVAL_SEC,
							atoi(evt.data));
					_gn_syn_nft1_change_interval(data);
				}
				 else if (gn_leaf_event_mask_param(&evt,
						data->gn_syn_nft1_control_watering_enable_param) == 0) {
					gn_leaf_param_force_bool(leaf_config,
							GN_SYN_NFT1_CONTROL_PARAM_ENABLE,
							atoi(evt.data) == 0 ? false : true);
				}

				break;

			default:
				break;

			}

		}

	}
}

#ifdef __cplusplus
}
#endif //__cplusplus
