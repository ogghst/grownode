#include "gn_pump.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GN_PUMP_STATE_STOP 0
#define GN_PUMP_STATE_RUNNING 1

int state = GN_PUMP_STATE_STOP;

static const char *TAG = "gn_pump";

void _gn_pump_event_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	switch (id) {
	case GN_NETWORK_CONNECTED_EVENT:
		//lv_label_set_text(network_status_label, "NET OK");
		state = GN_PUMP_STATE_RUNNING;
		break;
	case GN_NETWORK_DISCONNECTED_EVENT:
		//lv_label_set_text(network_status_label, "NET KO");
		state = GN_PUMP_STATE_STOP;
		break;
	case GN_SERVER_CONNECTED_EVENT:
		//lv_label_set_text(server_status_label, "SRV OK");
		state = GN_PUMP_STATE_RUNNING;
		break;
	case GN_SERVER_DISCONNECTED_EVENT:
		//lv_label_set_text(server_status_label, "SRV_KO");
		state = GN_PUMP_STATE_STOP;
		break;
	}

}

void _gn_pump_task(void *pvParam) {

	gn_leaf_config_handle_t config = (gn_leaf_config_handle_t) pvParam;

	while (true) {

		vTaskDelay(5000 / portTICK_PERIOD_MS);

		if (state == GN_PUMP_STATE_RUNNING) {
			char msg[80];
			strncpy(msg, "pump ", 79);
			strncat(msg, config->name, 79);
			strncat(msg, " tick", 79);
			gn_log_message(msg);
		}

	}

}

void gn_pump_callback(gn_event_id_t event, gn_leaf_config_handle_t leaf_config,
		void *event_data) {

	ESP_LOGI(TAG, "gn_pump_callback (%s)", leaf_config->name);

	switch (event) {

	case GN_LEAF_INIT_REQUEST_EVENT: {
		char buf[40];
		sprintf(buf, "%.*s_task", 30, leaf_config->name);
		xTaskCreate(_gn_pump_task, buf, 2048, leaf_config, 0, NULL);
		break;
	}
	case GN_LEAF_MESSAGE_RECEIVED_EVENT: {
		esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
		char buf[30 + event->data_len];
		sprintf(buf, "message received: %.*s", event->data_len, event->data);
		gn_log_message(buf);
		break;
	}

	default:
		break;

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
