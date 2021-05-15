#include "gn_pump.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GN_PUMP_STATE_STOP 0
#define GN_PUMP_STATE_RUNNING 1

size_t gn_pump_state = GN_PUMP_STATE_STOP;

static const char *TAG = "gn_pump";

size_t gn_pump_i = 0;
char gn_pump_leaf_name[20];
char gn_pump_buf[40];

void __gn_pump_loop(gn_leaf_config_handle_t leaf_config) {
	gn_pump_i++;
	if (gn_pump_state == GN_PUMP_STATE_RUNNING) {
		if (gn_pump_i > 1000) {
			ESP_LOGI(TAG, "running %s, status %d", gn_pump_leaf_name, gn_pump_state);
			gn_pump_i = 0;
		}
	}
}

void gn_pump_loop(gn_leaf_config_handle_t leaf_config) {


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
			__gn_pump_loop(leaf_config);
		}

		vTaskDelay(1);
	}



}

void gn_pump_callback(gn_event_handle_t event,
		gn_leaf_config_handle_t leaf_config) {

	ESP_LOGD(TAG, "gn_pump_callback (%s) event: %d", leaf_config->name,
			event->id);

	switch (event->id) {

	case GN_LEAF_INIT_REQUEST_EVENT:
		strncpy(gn_pump_leaf_name, leaf_config->name, 20);
		gn_pump_state = GN_PUMP_STATE_RUNNING;
		sprintf(gn_pump_buf, "%.*s init", 30, leaf_config->name);
		gn_log_message(gn_pump_buf);
		break;

	case GN_LEAF_MESSAGE_RECEIVED_EVENT:
		sprintf(gn_pump_buf, "message received: %.*s", (event->data_size > 20? 20: event->data_size), (char*) event->data);
		gn_log_message(gn_pump_buf);
		break;

	case GN_NETWORK_CONNECTED_EVENT:
		//lv_label_set_text(network_status_label, "NET OK");
		gn_pump_state = GN_PUMP_STATE_RUNNING;
		break;

	case GN_NETWORK_DISCONNECTED_EVENT:
		//lv_label_set_text(network_status_label, "NET KO");
		gn_pump_state = GN_PUMP_STATE_STOP;
		break;

	case GN_SERVER_CONNECTED_EVENT:
		//lv_label_set_text(server_status_label, "SRV OK");
		gn_pump_state = GN_PUMP_STATE_RUNNING;
		break;

	case GN_SERVER_DISCONNECTED_EVENT:
		//lv_label_set_text(server_status_label, "SRV_KO");
		gn_pump_state = GN_PUMP_STATE_STOP;
		break;

	default:
		break;

	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
