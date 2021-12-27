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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "gn_commons.h"

#include "gn_leaf_status_led.h"

#define TAG "gn_leaf_status_led"

void gn_leaf_status_led_task(gn_leaf_config_handle_t leaf_config);

typedef struct {
	gn_leaf_param_handle_t gn_leaf_status_led_gpio_param;
} gn_leaf_status_led_data_t;

gn_leaf_descriptor_handle_t gn_leaf_status_led_config(
		gn_leaf_config_handle_t leaf_config) {

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_STATUS_LED_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_leaf_status_led_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_leaf_status_led_data_t *data = malloc(sizeof(gn_leaf_status_led_data_t));

	data->gn_leaf_status_led_gpio_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_STATUS_LED_PARAM_GPIO, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 32 }, GN_LEAF_PARAM_ACCESS_WRITE,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	gn_leaf_param_add(leaf_config, data->gn_leaf_status_led_gpio_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void blink(int gpio, int time, int blinks) {

	ESP_LOGD(TAG, "blinking - gpio: %d, time: %d, blinks = %d", gpio, time, blinks);

	for (int i = 0; i < blinks; i++) {
		gpio_set_level((int) gpio, 1);
		vTaskDelay(time / portTICK_PERIOD_MS);
		gpio_set_level((int) gpio, 0);
		vTaskDelay(time / portTICK_PERIOD_MS);
	}

}

void gn_leaf_led_status_event_handler(void *handler_args, esp_event_base_t base,
		int32_t event_id, void *event_data) {

	ESP_LOGD(TAG, "gn_leaf_led_status_event_handler: %d", event_id);


	gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t)handler_args;

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_STATUS_LED_PARAM_GPIO, &gpio);

	//gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_args;

	gn_leaf_parameter_event_t *evt = (gn_leaf_parameter_event_t*) event_data;

	switch (event_id) {

	case GN_LEAF_PARAM_CHANGED_EVENT:
		blink((int)gpio, 100, 1);

	default:
		blink((int)gpio, 100, 1);
	}

}

void gn_leaf_status_led_task(gn_leaf_config_handle_t leaf_config) {

	ESP_LOGD(TAG, "Initializing leaf status led %s..",
			gn_leaf_get_config_name(leaf_config));

	gn_leaf_parameter_event_t evt;

//retrieves status descriptor from config
	gn_leaf_status_led_data_t *data =
			(gn_leaf_status_led_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	double gpio;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_STATUS_LED_PARAM_GPIO, &gpio);

	ESP_LOGD(TAG, "assigning to gpio %d", (int )gpio);

//setup led
	gpio_set_direction((int) gpio, GPIO_MODE_OUTPUT);
	gpio_set_level((int) gpio, 0);

//register for events
	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(gn_leaf_get_config_event_loop(leaf_config), GN_BASE_EVENT, GN_EVENT_ANY_ID, gn_leaf_led_status_event_handler, leaf_config, NULL));

//task cycle
	while (true) {

		//ESP_LOGD(TAG, "task cycle..");

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			ESP_LOGD(TAG, "%s - received message: %d",
					gn_leaf_get_config_name(leaf_config), evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:
				//what to do when network is connected
				break;

			case GN_NET_CONNECTED_EVENT:
				//gn_pump_state = GN_PUMP_STATE_RUNNING;
				blink(gpio, 100, 3);
				break;

				//what to do when network is disconnected
			case GN_NET_DISCONNECTED_EVENT:
				blink(gpio, 500, 3);
				break;

				//what to do when server is connected
			case GN_SRV_CONNECTED_EVENT:
				blink(gpio, 100, 5);
				break;

				//what to do when server is disconnected
			case GN_SRV_DISCONNECTED_EVENT:
				blink(gpio, 500, 3);
				break;

			case GN_NET_MSG_RECV:
				blink(gpio, 200, 1);
				break;

			default:
				break;

			}

		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
