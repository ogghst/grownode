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

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "lvgl_helpers.h"
#endif

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_gpio.h"

#define TAG "gn_leaf_gpio"

/**
 * creates a GPIO leaf defining all parameters
 *
 * @param	node		the parent node
 * @param	leaf_name	the name of the leaf to be created
 * @param	gpio		the GPIO number to attach this leaf
 * @param	inverted	if the GPIO need to work in inverted mode (toggle = true -> gpio = false)
 * @param	toggled		the initial status
 *
 * @return	an handle to the leaf initialized
 * @return	NULL		in case of errors
 */
gn_leaf_handle_t gn_gpio_fastcreate(gn_node_handle_t node,
		const char *leaf_name, int gpio, bool inverted, bool toggled) {

	if (node == NULL) {
		ESP_LOGE(TAG, "gn_gpio_fastcreate - node is null");
		return NULL;
	}

	if (leaf_name == NULL) {
		ESP_LOGE(TAG, "gn_gpio_fastcreate - leaf_name is null");
		return NULL;
	}

	if (gpio < GPIO_NUM_0 || gpio > GPIO_NUM_MAX) {
		ESP_LOGE(TAG, "gn_gpio_fastcreate - gpio out of limits");
		return NULL;
	}

	//creates the blink leave
	gn_leaf_handle_t leaf = gn_leaf_create(node, leaf_name, gn_gpio_config,
			4096, GN_LEAF_TASK_PRIORITY);

	if (leaf == NULL) {
		ESP_LOGE(TAG, "gn_gpio_fastcreate - cannot create leaf %s", leaf_name);
		return NULL;
	}

	gn_leaf_param_init_double(leaf, GN_GPIO_PARAM_GPIO, gpio);
	gn_leaf_param_init_bool(leaf, GN_GPIO_PARAM_INVERTED, inverted);
	gn_leaf_param_init_bool(leaf, GN_GPIO_PARAM_TOGGLE, toggled);

	return leaf;

}

void gn_gpio_task(gn_leaf_handle_t leaf_config);

typedef struct {
	gn_leaf_param_handle_t gn_gpio_toggle_param;
	gn_leaf_param_handle_t gn_gpio_inverted_param;
	gn_leaf_param_handle_t gn_gpio_gpio_param;
} gn_gpio_data_t;

gn_leaf_descriptor_handle_t gn_gpio_config(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_gpio_config", leaf_name);

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_GPIO_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_gpio_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_gpio_data_t *data = malloc(sizeof(gn_gpio_data_t));

	data->gn_gpio_toggle_param = gn_leaf_param_create(leaf_config,
			GN_GPIO_PARAM_TOGGLE, GN_VAL_TYPE_BOOLEAN,
			(gn_val_t ) { .b = false }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->gn_gpio_inverted_param = gn_leaf_param_create(leaf_config,
			GN_GPIO_PARAM_INVERTED, GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b =
					false }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->gn_gpio_gpio_param = gn_leaf_param_create(leaf_config,
			GN_GPIO_PARAM_GPIO, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 32 },
			GN_LEAF_PARAM_ACCESS_NETWORK, GN_LEAF_PARAM_STORAGE_PERSISTED,
			NULL);

	gn_leaf_param_add_to_leaf(leaf_config, data->gn_gpio_toggle_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_gpio_inverted_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_gpio_gpio_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_gpio_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	ESP_LOGD(TAG, "[%s] gn_gpio_task", leaf_name);

	//const size_t GN_GPIO_STATE_STOP = 0;
	//const size_t GN_GPIO_STATE_RUNNING = 1;

	//size_t gn_gpio_state = GN_GPIO_STATE_RUNNING;
	gn_leaf_parameter_event_t evt;

	//retrieves status descriptor from config
	gn_gpio_data_t *data =
			(gn_gpio_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_GPIO_PARAM_GPIO, &gpio);

	bool status;
	gn_leaf_param_get_bool(leaf_config, GN_GPIO_PARAM_TOGGLE, &status);

	bool inverted;
	gn_leaf_param_get_bool(leaf_config, GN_GPIO_PARAM_INVERTED, &inverted);

	ESP_LOGD(TAG, "configuring - gpio %d, status %d, inverted %d", (int )gpio,
			status ? 1 : 0, inverted ? 1 : 0);

	//setup
	gpio_set_direction((int) gpio, GPIO_MODE_OUTPUT);
	gpio_set_level((int) gpio,
			status ? (inverted ? 0 : 1) : (inverted ? 1 : 0));

	//setup screen, if defined in sdkconfig
#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	lv_obj_t *label_status = NULL;
	lv_obj_t *label_title = NULL;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		//parent container where adding elements
		lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf(leaf_config);

		if (_cnt) {

			//style from the container
			lv_style_t *style = _cnt->styles->style;

			lv_obj_set_layout(_cnt, LV_LAYOUT_GRID);
			lv_coord_t col_dsc[] = { 90, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
			lv_coord_t row_dsc[] = { 20, 20, 20, LV_GRID_FR(1),
			LV_GRID_TEMPLATE_LAST };
			lv_obj_set_grid_dsc_array(_cnt, col_dsc, row_dsc);

			label_title = lv_label_create(_cnt);
			lv_label_set_text(label_title, leaf_name);
			//lv_obj_add_style(label_title, style, 0);
			//lv_obj_align_to(label_title, _cnt, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
			lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 2,
					LV_GRID_ALIGN_STRETCH, 0, 1);

			label_status = lv_label_create(_cnt);
			//lv_obj_add_style(label_status, style, 0);
			lv_label_set_text(label_status,
					"status: off");
			//lv_obj_align_to(label_status, label_title, LV_ALIGN_BOTTOM_LEFT,
			//		LV_PCT(10), LV_PCT(10));
			lv_obj_set_grid_cell(label_status, LV_GRID_ALIGN_STRETCH, 0, 1,
					LV_GRID_ALIGN_STRETCH, 1, 2);

		}

		gn_display_leaf_refresh_end();

	}

#endif

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

				//parameter is status
				if (gn_leaf_event_mask_param(&evt, data->gn_gpio_toggle_param)
						== 0) {

					int _active = atoi(evt.data);

					//notify change
					ESP_LOGD(TAG, "written: %d", _active);

					gn_leaf_param_write_bool(leaf_config, GN_GPIO_PARAM_TOGGLE,
							_active == 0 ? false : true);

					status = _active;

					ESP_LOGD(TAG, "[%s] - gpio %d, toggle %d, inverted %d",
							leaf_name, (int )gpio, status ? 1 : 0,
							inverted ? 1 : 0);

					//update sensor using the parameter values
					gpio_set_level((int) gpio,
							status ? (inverted ? 0 : 1) : (inverted ? 1 : 0));

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
								status ?
								"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
#endif

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_gpio_inverted_param) == 0) {

					int _inverted = atoi(evt.data);

					//notify change
					gn_leaf_param_write_bool(leaf_config,
							GN_GPIO_PARAM_INVERTED,
							_inverted == 0 ? false : true);

					inverted = _inverted;

					ESP_LOGD(TAG, "[%s] - gpio %d, toggle %d, inverted %d",
							leaf_name, (int )gpio, status ? 1 : 0,
							inverted ? 1 : 0);

					//update sensor using the parameter values
					gpio_set_level((int) gpio,
							status ? (inverted ? 0 : 1) : (inverted ? 1 : 0));

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
						status ?
						"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
#endif

				}

				break;

				//what to do when network is connected
			case GN_NET_CONNECTED_EVENT:
				//gn_pump_state = GN_PUMP_STATE_RUNNING;
				break;

				//what to do when network is disconnected
			case GN_NET_DISCONNECTED_EVENT:
				//gn_gpio_state = GN_GPIO_STATE_STOP;
				break;

				//what to do when server is connected
			case GN_SRV_CONNECTED_EVENT:
				//gn_gpio_state = GN_GPIO_STATE_RUNNING;
				break;

				//what to do when server is disconnected
			case GN_SRV_DISCONNECTED_EVENT:
				//gn_gpio_state = GN_GPIO_STATE_STOP;
				break;

			default:
				break;

			}

		}

		//ESP_LOGD(TAG, "sleeping..");
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
