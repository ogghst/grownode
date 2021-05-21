#include "gn_ds18b20.h"
#include "ds18x20.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GN_DS18B20_STATE_STOP 0
#define GN_DS18B20_STATE_RUNNING 1

static const char *TAG = "gn_ds18b20";

//static const uint32_t LOOP_DELAY_MS = 500;
#define MAX_SENSORS 8
//static const int RESCAN_INTERVAL = 8;
static const gpio_num_t SENSOR_GPIO = 27;
//static const ds18x20_addr_t SENSOR_ADDR = 0x27041685c771ff28;

static size_t gn_ds18b20_state = GN_DS18B20_STATE_STOP;

static size_t sensor_count = 0;
static ds18x20_addr_t addrs[MAX_SENSORS];
static float temps[MAX_SENSORS];
static esp_err_t res;
static bool scanned = false;

void gn_ds18b20_callback(void *handler_args, esp_event_base_t base, int32_t id,
		void *event_data) {

	gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_args;
	gn_leaf_event_handle_t event;
	char gn_ds18b20_buf[40];

	ESP_LOGD(TAG, "callback (%s) event: %d", leaf_config->name, id);

	switch (id) {

	case GN_LEAF_MESSAGE_RECEIVED_EVENT: //TODO better use specific callbacks per events
		event = (gn_leaf_event_handle_t) event_data;
		if (strcmp(event->leaf_name, leaf_config->name) != 0)
			break;
		sprintf(gn_ds18b20_buf, "message received: %.*s",
				(event->data_size > 20 ? 20 : event->data_size),
				(char*) event->data);
		gn_message_display(gn_ds18b20_buf);
		break;

	case GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT:
		event = (gn_leaf_event_handle_t) event_data;
		if (strcmp(event->leaf_name, leaf_config->name) != 0)
			break;

		gn_leaf_param_handle_t _param = leaf_config->params;
		while(_param) {
			if (strcmp(event->param_name, _param->name) == 0) {

				sprintf(gn_ds18b20_buf, "message received: %.*s",
						(event->data_size > 20 ? 20 : event->data_size),
						(char*) event->data);
				gn_leaf_param_set_string(leaf_config, "temp", (char*) event->data);
				gn_message_display(gn_ds18b20_buf);
				break;
			} else _param = _param->next;
		}

		break;

	case GN_NETWORK_CONNECTED_EVENT:
		//lv_label_set_text(network_status_label, "NET OK");
		gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
		break;

	case GN_NETWORK_DISCONNECTED_EVENT:
		//lv_label_set_text(network_status_label, "NET KO");
		gn_ds18b20_state = GN_DS18B20_STATE_STOP;
		break;

	case GN_SERVER_CONNECTED_EVENT:
		//lv_label_set_text(server_status_label, "SRV OK");
		gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
		break;

	case GN_SERVER_DISCONNECTED_EVENT:
		//lv_label_set_text(server_status_label, "SRV_KO");
		gn_ds18b20_state = GN_DS18B20_STATE_STOP;
		break;

	default:
		break;

	}

}

void _scan_sensors() {

	if (scanned)
		return;

	res = ds18x20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS, &sensor_count);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Sensors scan error %d (%s)", res, esp_err_to_name(res));
		return;
	}

	if (!sensor_count) {
		ESP_LOGW(TAG, "No sensors detected!");
		return;
	}

	ESP_LOGD(TAG, "%d sensors detected", sensor_count);

	// If there were more sensors found than we have space to handle,
	// just report the first MAX_SENSORS..
	if (sensor_count > MAX_SENSORS)
		sensor_count = MAX_SENSORS;

	scanned = true;
}

void __gn_ds18b20_loop(gn_leaf_config_handle_t leaf_config) {

	char gn_ds18b20_buf[40];

	if (gn_ds18b20_state == GN_DS18B20_STATE_RUNNING) {

		_scan_sensors();

		res = ds18x20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count,
				temps);
		if (res != ESP_OK) {
			ESP_LOGE(TAG, "Sensors read error %d (%s)", res,
					esp_err_to_name(res));
			goto fail;
		}

		for (int j = 0; j < sensor_count; j++) {
			float temp_c = temps[j];
			float temp_f = (temp_c * 1.8) + 32;
			// Float is used in printf(). You need non-default configuration in
			// sdkconfig for ESP8266, which is enabled by default for this
			// example. See sdkconfig.defaults.esp8266
			ESP_LOGD(TAG, "Sensor %08x%08x (%s) reports %.3f °C (%.3f °F)",
					(uint32_t )(addrs[j] >> 32), (uint32_t )addrs[j],
					(addrs[j] & 0xff) == DS18B20_FAMILY_ID ? "DS18B20" : "DS18S20",
					temp_c, temp_f);
			sprintf(gn_ds18b20_buf, "%.3f", temp_c);

			gn_leaf_param_set_string(leaf_config, "temp", gn_ds18b20_buf);

			//gn_param_handle_t temp = gn_leaf_param_get(leaf_config, "temp");
			//temp->param_val->val.s = (char*) realloc(temp->param_val->val.s, sizeof(gn_ds18b20_buf));

			gn_message_display(gn_ds18b20_buf);
		}

		/*
		 if (res != ESP_OK)
		 ESP_LOGE(TAG, "Could not read from sensor %08x%08x: %d (%s)",
		 (uint32_t )(SENSOR_ADDR >> 32), (uint32_t )SENSOR_ADDR, res,
		 esp_err_to_name(res));
		 else
		 ESP_LOGI(TAG, "Sensor %08x%08x: %.2fÂ°C",
		 (uint32_t )(SENSOR_ADDR >> 32), (uint32_t )SENSOR_ADDR,
		 temperature);
		 */
		fail: return;

	}
}

void gn_ds18b20_loop(gn_leaf_config_handle_t leaf_config) {

	gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);
	gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
	char gn_ds18b20_buf[GN_LEAF_NAME_SIZE + 10];
	sprintf(gn_ds18b20_buf, "%.*s init", GN_LEAF_NAME_SIZE, leaf_config->name);
	gn_message_display(gn_ds18b20_buf);

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(leaf_config->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, gn_ds18b20_callback, leaf_config, NULL));

	gn_leaf_param_handle_t param = gn_leaf_param_create("temp", GN_VAL_TYPE_STRING,
			(gn_val_t) { .s = "" });
	gn_leaf_param_add(leaf_config, param);

	while (true) {

		__gn_ds18b20_loop(leaf_config);

		vTaskDelay(pdMS_TO_TICKS(5000));
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
