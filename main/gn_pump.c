#include "gn_pump.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GN_PUMP_STATE_STOP 0
#define GN_PUMP_STATE_RUNNING 1

size_t gn_pump_state = GN_PUMP_STATE_STOP;

static const char *TAG = "gn_pump";

static size_t gn_pump_i = 0;
static gn_leaf_config_handle_t _leaf_config;
static char gn_pump_buf[60];

void gn_pump_callback(void *handler_args, esp_event_base_t base, int32_t id,
		void *event_data) {

	gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_args;
	gn_leaf_event_handle_t event;

	ESP_LOGD(TAG, "gn_pump_callback (%s) event: %d", leaf_config->name, id);

	switch (id) {

	case GN_LEAF_MESSAGE_RECEIVED_EVENT:
		event = (gn_leaf_event_handle_t) event_data;
		if (strcmp(event->leaf_name, leaf_config->name) != 0)
			break;
		sprintf(gn_pump_buf, "message received: %.*s",
				(event->data_size > 20 ? 20 : event->data_size),
				(char*) event->data);
		gn_message_display(gn_pump_buf);
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

void gn_pump_loop(gn_leaf_config_handle_t leaf_config) {

	_leaf_config = leaf_config;
	gn_pump_state = GN_PUMP_STATE_RUNNING;
	sprintf(gn_pump_buf, "%.*s init", 30, leaf_config->name);
	gn_message_display(gn_pump_buf);

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(leaf_config->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, gn_pump_callback, leaf_config, NULL));

	gn_param_handle_t param = gn_leaf_param_create("status", GN_VAL_TYPE_BOOLEAN,
			(gn_val_t){ false });
	gn_leaf_param_add(leaf_config, param);

	while (true) {

		gn_pump_i++;
		if (gn_pump_state == GN_PUMP_STATE_RUNNING) {
			if (gn_pump_i > 1000) {

				gn_param_handle_t param = gn_leaf_param_get(leaf_config, "status");
				param->param_val->v.b = !param->param_val->v.b;
				gn_leaf_param_set_bool(leaf_config, "status", param->param_val->v.b);

				sprintf(gn_pump_buf,  "%s - %d", leaf_config->name,
						gn_pump_state);
				gn_message_send_text(leaf_config, gn_pump_buf);
				gn_message_display(gn_pump_buf);
				gn_pump_i = 0;

			}
		}

		vTaskDelay(1);
	}

}

#ifdef __cplusplus
}
#endif //__cplusplus
