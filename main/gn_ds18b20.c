#include "gn_ds18b20.h"
#include "ds18x20.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GN_DS18B20_STATE_STOP 0
#define GN_DS18B20_STATE_RUNNING 1

static const char *TAG = "gn_ds18b20";

//static const uint32_t LOOP_DELAY_MS = 500;
static const int MAX_SENSORS = 8;
//static const int RESCAN_INTERVAL = 8;
static const gpio_num_t SENSOR_GPIO = 27;
//static const ds18x20_addr_t SENSOR_ADDR = 0x27041685c771ff28;


size_t sensor_count = 0;

size_t gn_ds18b20_i = 0;
char gn_ds18b20_leaf_name[20];
char gn_ds18b20_buf[40];
size_t gn_ds18b20_state = GN_DS18B20_STATE_STOP;

void init() {
	gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);
}

void __gn_ds18b20_loop(gn_leaf_config_handle_t leaf_config) {

	ds18x20_addr_t addrs[MAX_SENSORS];
	float temps[MAX_SENSORS];
	float temperature;
	esp_err_t res;

	if (gn_ds18b20_state == GN_DS18B20_STATE_RUNNING) {

		// Every RESCAN_INTERVAL samples, check to see if the sensors connected
		// to our bus have changed.
		res = ds18x20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS,
				&sensor_count);
		if (res != ESP_OK) {
			ESP_LOGE(TAG, "Sensors scan error %d (%s)", res,
					esp_err_to_name(res));
			goto fail;
		}

		if (!sensor_count) {
			ESP_LOGW(TAG, "No sensors detected!");
			goto fail;
		}

		ESP_LOGI(TAG, "%d sensors detected", sensor_count);

		// If there were more sensors found than we have space to handle,
		// just report the first MAX_SENSORS..
		if (sensor_count > MAX_SENSORS)
			sensor_count = MAX_SENSORS;

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
			ESP_LOGI(TAG, "Sensor %08x%08x (%s) reports %.3f°C (%.3f°F)",
					(uint32_t )(addrs[j] >> 32), (uint32_t )addrs[j],
					(addrs[j] & 0xff) == DS18B20_FAMILY_ID ?
							"DS18B20" : "DS18S20", temp_c, temp_f);
			sprintf(gn_ds18b20_buf, "temp: %.3f°C", temp_c);
			gn_log_message(gn_ds18b20_buf);
		}

		/*
		 if (res != ESP_OK)
		 ESP_LOGE(TAG, "Could not read from sensor %08x%08x: %d (%s)",
		 (uint32_t )(SENSOR_ADDR >> 32), (uint32_t )SENSOR_ADDR, res,
		 esp_err_to_name(res));
		 else
		 ESP_LOGI(TAG, "Sensor %08x%08x: %.2f°C",
		 (uint32_t )(SENSOR_ADDR >> 32), (uint32_t )SENSOR_ADDR,
		 temperature);
		 */
		fail:
		vTaskDelay(pdMS_TO_TICKS(5000));

	}
}


void gn_ds18b20_loop(gn_leaf_config_handle_t leaf_config) {


	//make sure the init event is processed before anything else
	gn_event_handle_t _init_evt = (gn_event_handle_t) malloc(
			sizeof(gn_event_t));
	_init_evt->id = GN_LEAF_INIT_REQUEST_EVENT;
	_init_evt->data = NULL;
	_init_evt->data_size = 0;

	leaf_config->callback(_init_evt, leaf_config);

	free(_init_evt);

	gn_event_t evt;

	while (true) {
		//wait for events, otherwise run execution
		if (xQueueReceive(leaf_config->xLeafTaskEventQueue, &evt,
				(TickType_t) 10) == pdPASS) {
			ESP_LOGD(TAG, "_gn_leaf_task %s event received %d",
					leaf_config->name, evt.id);
			//event received
			leaf_config->callback(&evt, leaf_config);
		} else {
			//run
			//ESP_LOGD(TAG, "_gn_leaf_task %s loop", leaf_config->name);
			__gn_ds18b20_loop(leaf_config);
		}

		vTaskDelay(1);
	}



}

void gn_ds18b20_callback(gn_event_handle_t event,
		gn_leaf_config_handle_t leaf_config) {

	ESP_LOGD(TAG, "callback (%s) event: %d", leaf_config->name, event->id);

	switch (event->id) {

	case GN_LEAF_INIT_REQUEST_EVENT:
		init();
		strncpy(gn_ds18b20_leaf_name, leaf_config->name, 20);
		gn_ds18b20_state = GN_DS18B20_STATE_RUNNING;
		sprintf(gn_ds18b20_buf, "%.*s init", 30, leaf_config->name);
		gn_log_message(gn_ds18b20_buf);
		break;

	case GN_LEAF_MESSAGE_RECEIVED_EVENT:
		sprintf(gn_ds18b20_buf, "message received: %.*s",
				(event->data_size > 20 ? 20 : event->data_size),
				(char*) event->data);
		gn_log_message(gn_ds18b20_buf);
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

#ifdef __cplusplus
}
#endif //__cplusplus
