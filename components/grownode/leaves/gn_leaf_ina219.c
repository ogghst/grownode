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

#define IP_STRING_SIZE 16

typedef struct {
	gn_leaf_param_handle_t gn_leaf_ina219_active_param;
	gn_leaf_param_handle_t gn_leaf_ina219_ip_param;
	gn_leaf_param_handle_t gn_leaf_ina219_port_param;
	gn_leaf_param_handle_t gn_leaf_ina219_sampling_cycles_param;
	gn_leaf_param_handle_t gn_leaf_ina219_sampling_interval_param;
	gn_leaf_param_handle_t gn_leaf_ina219_sda_param;
	gn_leaf_param_handle_t gn_leaf_ina219_scl_param;
	gn_leaf_param_handle_t gn_leaf_ina219_working_mode_param;
	gn_leaf_param_handle_t gn_leaf_ina219_power_param;
	gn_leaf_param_handle_t gn_leaf_ina219_voltage_param;
	gn_leaf_param_handle_t gn_leaf_ina219_shunt_voltage_param;
	gn_leaf_param_handle_t gn_leaf_ina219_bus_voltage_param;
	gn_leaf_param_handle_t gn_leaf_ina219_current_param;
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
					true }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_boolean);

	data->gn_leaf_ina219_ip_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_IP, GN_VAL_TYPE_STRING, (gn_val_t ) { .s =
							calloc(16, sizeof(char)) },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED, NULL);

	data->gn_leaf_ina219_port_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_PORT, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							8094 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_sampling_cycles_param = gn_leaf_param_create(
			leaf_config, GN_LEAF_INA219_PARAM_SAMPLING_CYCLES,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 1 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED,
			gn_validator_double_positive);

	data->gn_leaf_ina219_sampling_interval_param = gn_leaf_param_create(
			leaf_config, GN_LEAF_INA219_PARAM_SAMPLING_INTERVAL,
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = 1000 },
			GN_LEAF_PARAM_ACCESS_ALL, GN_LEAF_PARAM_STORAGE_PERSISTED,
			gn_validator_double_positive);

	data->gn_leaf_ina219_sda_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_SDA, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 26 }, GN_LEAF_PARAM_ACCESS_NODE_INTERNAL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_scl_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_SCL, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 27 }, GN_LEAF_PARAM_ACCESS_NODE_INTERNAL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_working_mode_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_WORKING_MODE, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 1 }, GN_LEAF_PARAM_ACCESS_ALL,
			GN_LEAF_PARAM_STORAGE_PERSISTED, gn_validator_double_positive);

	data->gn_leaf_ina219_power_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_POWER, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->gn_leaf_ina219_voltage_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_VOLTAGE, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							0 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->gn_leaf_ina219_bus_voltage_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_BUS_VOLTAGE, GN_VAL_TYPE_DOUBLE, (gn_val_t ) {
							.d = 0 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->gn_leaf_ina219_shunt_voltage_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_SHUNT_VOLTAGE, GN_VAL_TYPE_DOUBLE,
			(gn_val_t ) { .d = 0 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	data->gn_leaf_ina219_current_param = gn_leaf_param_create(leaf_config,
			GN_LEAF_INA219_PARAM_CURRENT, GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d =
							0 }, GN_LEAF_PARAM_ACCESS_NETWORK,
			GN_LEAF_PARAM_STORAGE_VOLATILE, NULL);

	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_active_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_ip_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_port_param);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_leaf_ina219_sampling_cycles_param);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_leaf_ina219_sampling_interval_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_sda_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_scl_param);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_leaf_ina219_working_mode_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_voltage_param);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_leaf_ina219_bus_voltage_param);
	gn_leaf_param_add_to_leaf(leaf_config,
			data->gn_leaf_ina219_shunt_voltage_param);
	gn_leaf_param_add_to_leaf(leaf_config, data->gn_leaf_ina219_current_param);

	descriptor->status = GN_LEAF_STATUS_INITIALIZED;
	descriptor->data = data;
	return descriptor;

}

void gn_leaf_ina219_task(gn_leaf_handle_t leaf_config) {

	char leaf_name[GN_LEAF_NAME_SIZE];
	gn_leaf_get_name(leaf_config, leaf_name);

	char node_name[GN_NODE_NAME_SIZE];
	gn_node_get_name(gn_leaf_get_node(leaf_config), node_name);

	ESP_LOGD(TAG, "[%s] - Initializing ..", leaf_name);

	gn_leaf_parameter_event_t evt;

	bool active = true;
	gn_leaf_param_get_bool(leaf_config, GN_LEAF_INA219_PARAM_ACTIVE, &active);

	char ip[IP_STRING_SIZE];
	gn_leaf_param_get_string(leaf_config, GN_LEAF_INA219_PARAM_IP, ip,
	IP_STRING_SIZE);

	double port = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_PORT, &port);

	double sampling_cycles = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_SAMPLING_CYCLES,
			&sampling_cycles);

	double sampling_interval = 0;
	gn_leaf_param_get_double(leaf_config,
			GN_LEAF_INA219_PARAM_SAMPLING_INTERVAL, &sampling_interval);

	double sda = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_SDA, &sda);

	double scl = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_SCL, &scl);

	double working_mode = 0;
	gn_leaf_param_get_double(leaf_config, GN_LEAF_INA219_PARAM_WORKING_MODE,
			&working_mode);

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
			INA219_MODE_CONT_SHUNT_BUS);
	ESP_LOGD(TAG, "ina219_configure: %s", esp_err_to_name(esp_ret));

	esp_ret = ina219_calibrate(&data->dev, 5.0, 0.1); // 5A max current, 0.1 Ohm shunt resistance

	ESP_LOGD(TAG, "ina219_calibrate: %s", esp_err_to_name(esp_ret));

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
	ESP_LOGD(TAG, "Socket created, sending to %s:%d", ip, (int )port);

	float bus_voltage = 0, shunt_voltage = 0, current = 0, power = 0;
	float bus_voltage_instant = 0, shunt_voltage_instant = 0, current_instant =
			0, power_instant = 0;

	char total_msg[255];

//task cycle
	while (true) {

		//ESP_LOGD(TAG, "task cycle..");

		//check for messages
		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				0) == pdPASS) {

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
					//strncpy(ip, evt.data, IP_STRING_SIZE-1);
					memcpy(ip, evt.data, IP_STRING_SIZE - 1);
					ip[15] = 0;

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
						data->gn_leaf_ina219_sampling_cycles_param) == 0) {

					sampling_cycles = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_SAMPLING_CYCLES,
							sampling_cycles);

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_sampling_interval_param) == 0) {

					sampling_interval = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_SAMPLING_INTERVAL,
							sampling_interval);

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

				} else if (gn_leaf_event_mask_param(&evt,
						data->gn_leaf_ina219_working_mode_param) == 0) {

					working_mode = atof(evt.data);
					//execute change
					gn_leaf_param_write_double(leaf_config,
							GN_LEAF_INA219_PARAM_WORKING_MODE, working_mode);

				}

				break;

			default:
				break;

			}

		}

		if (active) {

			bus_voltage = 0;
			shunt_voltage = 0;
			current = 0;
			power = 0;

			//ESP_LOGD(TAG,"start sampling");

			//retrieve sensor info
			//sampling configurable
			for (int i = 0; i < sampling_cycles; i++) {

				ESP_ERROR_CHECK(
						ina219_get_bus_voltage(&data->dev,
								&bus_voltage_instant));
				bus_voltage += bus_voltage_instant;

				ESP_ERROR_CHECK(
						ina219_get_shunt_voltage(&data->dev,
								&shunt_voltage_instant));
				shunt_voltage += shunt_voltage_instant;

				ESP_ERROR_CHECK(
						ina219_get_current(&data->dev, &current_instant));
				current += current_instant;

				ESP_ERROR_CHECK(ina219_get_power(&data->dev, &power_instant));
				power += power_instant;

				vTaskDelay(pdMS_TO_TICKS(sampling_interval-0.5)); //0.6msec is the time needed to perform a 12bit measure 1 sample

			}

			//averaging
			bus_voltage = (bus_voltage / (float) sampling_cycles)  * 1000;
			shunt_voltage = (shunt_voltage / (float) sampling_cycles) * 1000;
			current = (current / (float) sampling_cycles) * 1000;
			power = (power / (float) sampling_cycles) * 1000;

			ESP_LOGD(TAG,
					"VBUS: %.04f mV, VSHUNT: %.04f mV, IBUS: %.04f mA, PBUS: %.04f mW\n",
					bus_voltage, shunt_voltage, current,
					power);

			if (working_mode == 0 || working_mode == 2) {
				int err;

				snprintf(total_msg, 255,
						"grownode,node=%s,leaf=%s power=%f,current=%f,vbus=%f,vshunt=%f,vload=%f",
						node_name, leaf_name, power, current,
						bus_voltage, shunt_voltage,
						(shunt_voltage + bus_voltage));

				err = sendto(sock, total_msg, strlen(total_msg), 0,
						(struct sockaddr*) &dest_addr, sizeof(dest_addr));
				if (err < 0) {
					ESP_LOGE(TAG, "Error occurred during sending: errno %d",
							errno);
				}
				ESP_LOGD(TAG, "Message sent: %s to %s:%d", total_msg, ip,
						(int )port);
			}

			if (working_mode == 1 || working_mode == 2) {

				gn_leaf_param_write_double(leaf_config,
						GN_LEAF_INA219_PARAM_VOLTAGE,
						(shunt_voltage + bus_voltage));

				gn_leaf_param_write_double(leaf_config,
						GN_LEAF_INA219_PARAM_SHUNT_VOLTAGE, shunt_voltage);

				gn_leaf_param_write_double(leaf_config,
						GN_LEAF_INA219_PARAM_BUS_VOLTAGE, bus_voltage);

				gn_leaf_param_write_double(leaf_config,
						GN_LEAF_INA219_PARAM_POWER, power);

				gn_leaf_param_write_double(leaf_config,
						GN_LEAF_INA219_PARAM_CURRENT, current);

			}

		}

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
