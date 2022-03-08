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

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "lvgl_helpers.h"
#endif

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "ina219.h"

#include "gn_commons.h"

#include "gn_leaf_ina219.h"

#define TAG "gn_leaf_ina219"

#define I2C_PORT 0
#define I2C_ADDR INA219_ADDR_GND_GND

typedef struct {
	gn_leaf_param_handle_t gn_leaf_ina219_active_param;
	gn_leaf_param_handle_t gn_leaf_ina219_ip_param;
	gn_leaf_param_handle_t gn_leaf_ina219_port_param;
	gn_leaf_param_handle_t gn_leaf_ina219_sampling_ms_param;
	gn_leaf_param_handle_t gn_leaf_ina219_sda_param;
	gn_leaf_param_handle_t gn_leaf_ina219_scl_param;
	ina219_t dev;
} gn_leaf_ina219_data_t;

void gn_leaf_ina219_task(gn_leaf_handle_t leaf_config);

gn_leaf_descriptor_handle_t gn_leaf_ina219_config(gn_leaf_handle_t leaf_config) {

	ESP_LOGD(TAG, "ina219 configuring..");

	gn_leaf_descriptor_handle_t descriptor =
			(gn_leaf_descriptor_handle_t) malloc(sizeof(gn_leaf_descriptor_t));
	strncpy(descriptor->type, GN_LEAF_INA219_TYPE, GN_LEAF_DESC_TYPE_SIZE);
	descriptor->callback = gn_leaf_ina219_task;
	descriptor->status = GN_LEAF_STATUS_NOT_INITIALIZED;

	gn_leaf_ina219_data_t *data = malloc(sizeof(gn_leaf_ina219_data_t));

	data->gn_leaf_ina219_active_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_ACTIVE, GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b =
					true }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_boolean);

	data->gn_leaf_ina219_ip_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_IP, GN_VAL_TYPE_STRING, (gn_val_t ) { .s =
							malloc(16) }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->gn_leaf_ina219_port_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_PORT, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_sampling_ms_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_SAMPLING_MS, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 100 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_sda_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_SDA, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 26 }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_scl_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_SCL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 27 }, GN_LEAF_PARAM_ACCESS_NODE,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_active_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_ip_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_port_param);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_leaf_ina219_sampling_ms_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_sda_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_scl_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_leaf_ina219_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);
	ESP_LOGD(TAG, "[%s] - Initializing ..", leaf_name);

	gn_leaf_parameter_event_t evt;

	bool active = true;
	gn_leaf_param_get_bool(leaf_config, GN_LEAF_INA219_PARAM_ACTIVE, &active);

	char ip[16];
	gn_leaf_param_get_string(leaf_config, GN_LEAF_INA219_PARAM_IP, ip, 16);

	double port = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_PORT, &port);

	double sampling_ms = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_SAMPLING_MS,
			&sampling_ms);

	double sda = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_SDA, &sda);

	double scl = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_SCL, &scl);

	//retrieves status descriptor from config
	gn_leaf_ina219_data_t *data =
			(gn_leaf_ina219_data_t*) gn_leaf_get_descriptor(leaf_config)->data;

	//setup driver
	memset(&data->dev, 0, sizeof(ina219_t));

	esp_err_t esp_ret;
	esp_ret = ina219_init_desc(&data->dev, I2C_ADDR, I2C_PORT, (int) sda,
			(int) scl);
	ESP_LOGD(TAG, "ina219_init_desc (%d,%d,%d,%d): %s", I2C_ADDR, I2C_PORT,
			(int )sda, (int )scl, esp_err_to_name(esp_ret));

	esp_ret = ina219_init(&data->dev);
	ESP_LOGD(TAG, "ina219_init: %s", esp_err_to_name(esp_ret));

	esp_ret = ina219_configure(&data->dev, INA219_BUS_RANGE_16V,
			INA219_GAIN_0_125, INA219_RES_12BIT_1S, INA219_RES_12BIT_1S,
			INA219_MODE_CONT_SHUNT);
	ESP_LOGD(TAG, "ina219_configure: %s", esp_err_to_name(esp_ret));

	esp_ret = ina219_calibrate(&data->dev, 5.0, 0.1); // 5A max current, 0.1 Ohm shunt resistance

	ESP_LOGD(TAG, "ina219_calibrate: %s", esp_err_to_name(esp_ret));

	float bus_voltage, shunt_voltage, current, power;

	int addr_family = 0;
	int ip_protocol = 0;

	struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = inet_addr(ip);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons((uint16_t )port);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

	int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
	}
	ESP_LOGI(TAG, "Socket created, sending to %s:%d", ip, (int )port);

	char current_msg[80];
	char bus_voltage_msg[80];
	char shunt_voltage_msg[80];
	char load_voltage_msg[80];
	char power_msg[80];

//task cycle
	while (true) {

		//ESP_LOGD(TAG, "task cycle..");

		//check for messages and cycle every 100ms
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(sampling_ms)) == pdPASS) {

			ESP_LOGD(TAG, "%s - received message: %d", leaf_name, evt.id);

			//event arrived for this node
			switch (evt.id) {

			//parameter change
			case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

				//parameter is update time
				if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_active_param) == 0) {

					//bool prev_active = active;
					int _active = atoi(evt.data);

					//execute change
					gn_leaf_param_write_bool(leaf_config,
							GN_LEAF_INA219_PARAM_ACTIVE,
							_active == 0 ? false : true);

					active = _active;

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_ip_param) == 0) {

					//execute change
					gn_leaf_param_write_string(leaf_config,
							GN_LEAF_INA219_PARAM_IP, evt.data);
					strncpy(ip, evt.data, 16);

					//restart messaging configuration
					shutdown(sock, 0);
					close(sock);

					dest_addr.sin_addr.s_addr = inet_addr(ip);
					dest_addr.sin_port = htons((uint16_t )port);
					sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
					if (sock < 0) {
						ESP_LOGE(TAG, "Unable to create socket: errno %d",
								errno);
					}
					ESP_LOGI(TAG, "Socket created, sending to %s:%d", ip,
							(int )port);

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_port_param) == 0) {

					port = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_PORT, port);

					//restart messaging configuration
					shutdown(sock, 0);
					close(sock);

					dest_addr.sin_addr.s_addr = inet_addr(ip);
					dest_addr.sin_port = htons((uint16_t )port);
					sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
					if (sock < 0) {
						ESP_LOGE(TAG, "Unable to create socket: errno %d",
								errno);
					}
					ESP_LOGI(TAG, "Socket created, sending to %s:%d", ip,
							(int )port);

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_sampling_ms_param) == 0) {

					sampling_ms = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_SAMPLING_MS, sampling_ms);

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_sda_param) == 0) {

					sda = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_SDA, sda);

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_scl_param) == 0) {

					scl = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_SCL, scl);

				}

				break;

			default:
				break;

			}

		}

		if (active) {

			//retrieve sensor info
			ESP_ERROR_CHECK(ina219_get_bus_voltage(&data->dev, &bus_voltage));
			ESP_ERROR_CHECK(
					ina219_get_shunt_voltage(&data->dev, &shunt_voltage));
			ESP_ERROR_CHECK(ina219_get_current(&data->dev, &current));
			ESP_ERROR_CHECK(ina219_get_power(&data->dev, &power));

			ESP_LOGD(TAG,
					"VBUS: %.04f V, VSHUNT: %.04f mV, IBUS: %.04f mA, PBUS: %.04f mW\n",
					bus_voltage, shunt_voltage * 1000, current * 1000,
					power * 1000);

			int err;

			snprintf(current_msg, 80, "ina219,sensor=current value=%f",
					current * 1000);
			err = sendto(sock, current_msg, strlen(current_msg), 0,
					(struct sockaddr*) &dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			ESP_LOGD(TAG, "Message sent: %s to %s:%d", current_msg, ip,
					(int )port);

			snprintf(bus_voltage_msg, 80, "ina219,sensor=voltage value=%f",
					bus_voltage);
			err = sendto(sock, bus_voltage_msg, strlen(bus_voltage_msg), 0,
					(struct sockaddr*) &dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			ESP_LOGD(TAG, "Message sent: %s to %s:%d", bus_voltage_msg, ip,
					(int )port);

			snprintf(shunt_voltage_msg, 80,
					"ina219,sensor=shunt_voltage value=%f",
					shunt_voltage * 1000);
			err = sendto(sock, shunt_voltage_msg, strlen(shunt_voltage_msg), 0,
					(struct sockaddr*) &dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			ESP_LOGD(TAG, "Message sent: %s to %s:%d", shunt_voltage_msg, ip,
					(int )port);

			snprintf(load_voltage_msg, 80, "ina219,sensor=load_voltage value=%f",
					power * 1000);
			err = sendto(sock, load_voltage_msg, strlen(load_voltage_msg), 0,
					(struct sockaddr*) &dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			ESP_LOGD(TAG, "Message sent: %s to %s:%d", load_voltage_msg, ip,
					(int )port);

			snprintf(power_msg, 80, "ina219,sensor=power value=%f",
					power * 1000);
			err = sendto(sock, power_msg, strlen(power_msg), 0,
					(struct sockaddr*) &dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			ESP_LOGD(TAG, "Message sent: %s to %s:%d", power_msg, ip,
					(int )port);

		}

		//vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
