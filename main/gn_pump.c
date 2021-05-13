#include "gn_pump.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GN_PUMP_STATE_STOP 0
#define GN_PUMP_STATE_RUNNING 1

size_t state = GN_PUMP_STATE_STOP;

static const char *TAG = "gn_pump";

size_t i = 0;
char leaf_name[20];
char buf[40];

void gn_pump_loop(gn_leaf_config_handle_t leaf_config) {
	i++;
	if (state == GN_PUMP_STATE_RUNNING) {
		if (i > 1000) {
			ESP_LOGI(TAG, "running %s, status %d", leaf_name, state);
			i = 0;
		}
	}
}

void gn_pump_callback(gn_event_handle_t event,
		gn_leaf_config_handle_t leaf_config) {

	ESP_LOGD(TAG, "gn_pump_callback (%s) event: %d", leaf_config->name,
			event->id);

	switch (event->id) {

	case GN_LEAF_INIT_REQUEST_EVENT:
		strncpy(leaf_name, leaf_config->name, 20);
		state = GN_PUMP_STATE_RUNNING;
		sprintf(buf, "%.*s init", 30, leaf_config->name);
		gn_log_message(buf);
		break;

	case GN_LEAF_MESSAGE_RECEIVED_EVENT:
		sprintf(buf, "message received: %.*s", (event->data_size > 20? 20: event->data_size), (char*) event->data);
		gn_log_message(buf);
		break;

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

	default:
		break;

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
