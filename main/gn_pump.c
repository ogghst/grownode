#ifdef __cplusplus
extern "C" {
#endif

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#include "gn_pump.h"

#define GN_PUMP_STATE_STOP 0
#define GN_PUMP_STATE_RUNNING 1

#define GPIO_PWM0A_OUT 32

size_t gn_pump_state = GN_PUMP_STATE_STOP;

static const char *TAG = "gn_pump";

//static gn_leaf_config_handle_t _leaf_config;
static char gn_pump_buf[60];

static const int GN_PUMP_EVENT = BIT0;
static EventGroupHandle_t _gn_event_group_pump;

//static lv_obj_t *_cnt;
static SemaphoreHandle_t xGuiSemaphore;
static lv_obj_t *label_status;
static lv_obj_t *label_power;

void gn_pump_callback(void *handler_args, esp_event_base_t base, int32_t id,
		void *event_data) {

	gn_leaf_config_handle_t leaf_config = (gn_leaf_config_handle_t) handler_args;
	gn_leaf_event_handle_t event;

	char* leaf_name = gn_get_leaf_config_name(leaf_config);

	ESP_LOGD(TAG, "gn_pump_callback (%s) event: %d", leaf_name, id);

	switch (id) {

	case GN_LEAF_MESSAGE_RECEIVED_EVENT:
		event = (gn_leaf_event_handle_t) event_data;
		if (strcmp(event->leaf_name, leaf_name) != 0)
			break;
		sprintf(gn_pump_buf, "message received: %.*s",
				(event->data_size > 20 ? 20 : event->data_size),
				(char*) event->data);
		gn_message_display(gn_pump_buf);
		break;

	case GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT: //TODO move to core and use callbacks
		event = (gn_leaf_event_handle_t) event_data;
		if (strcmp(event->leaf_name, leaf_name) != 0)
			break;

		if (strcmp(event->param_name, "status") == 0) {

			if (strncmp(event->data, "true", event->data_size) == 0) {
				//setting to true
				gn_leaf_param_set_bool(leaf_config, "status",
				true);
				xEventGroupSetBits(_gn_event_group_pump, GN_PUMP_EVENT);
				break;
			} else if (strncmp(event->data, "false", event->data_size) == 0) {
				//setting to false
				gn_leaf_param_set_bool(leaf_config, "status",
				false);
				xEventGroupSetBits(_gn_event_group_pump, GN_PUMP_EVENT);
				break;
			}

		}

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

void gn_pump_task(gn_leaf_config_handle_t leaf_config) {

	char* leaf_name = gn_get_leaf_config_name(leaf_config);

	sprintf(gn_pump_buf, "%.*s init", 30, leaf_name);
	gn_message_display(gn_pump_buf);

	//init variables. TODO make status and power stored in flash
	double power = 0;
	bool status = false;

	gn_pump_state = GN_PUMP_STATE_RUNNING;

	_gn_event_group_pump = xEventGroupCreate();

	//setup pwm
	mcpwm_pin_config_t pin_config = { .mcpwm0a_out_num = GPIO_PWM0A_OUT };
	mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
	mcpwm_config_t pwm_config;
	pwm_config.frequency = 3000; //TODO make configurable
	pwm_config.cmpr_a = power;
	pwm_config.cmpr_b = 0.0;
	pwm_config.counter_mode = MCPWM_UP_COUNTER;
	pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings

	esp_event_loop_handle_t event_loop = gn_get_leaf_config_event_loop(leaf_config);

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, gn_pump_callback, leaf_config, NULL));

	gn_leaf_param_handle_t status_param = gn_leaf_param_create("status",
			GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b = false });

	gn_leaf_param_add(leaf_config, status_param);

	gn_leaf_param_handle_t power_param = gn_leaf_param_create("power",
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = power });

	gn_leaf_param_add(leaf_config, power_param);

	while (true) {

		xEventGroupWaitBits(_gn_event_group_pump, GN_PUMP_EVENT, pdTRUE,
		true, portMAX_DELAY);
		ESP_LOGD(TAG, "gn_pump_loop event");

		if (gn_pump_state != GN_PUMP_STATE_RUNNING) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
		}

		status_param = gn_leaf_param_get(leaf_config, "status");

		if (status_param->param_val->v.b != status) {
			status = status_param->param_val->v.b;

			if (status_param->param_val->v.b) {
				mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, power);
			} else {
				mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
			}

			//show status
			if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {

				lv_label_set_text(label_status,
						status_param->param_val->v.b ? "on" : "off");

				xSemaphoreGive(xGuiSemaphore);
			}
		}

		power_param = gn_leaf_param_get(leaf_config, "power");

		if (power_param->param_val->v.d != power) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, power);
			power = power_param->param_val->v.d;

			//show power
			if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {

				char _p[11];
				snprintf(_p, 10, "%f", power_param->param_val->v.d);
				lv_label_set_text(label_power, _p);

				xSemaphoreGive(xGuiSemaphore);
			}

		}

		/*
		 gn_pump_i++;
		 if (gn_pump_state == GN_PUMP_STATE_RUNNING) {
		 if (gn_pump_i > 1000) {


		 gn_leaf_param_handle_t param = gn_leaf_param_get(leaf_config,
		 "status");
		 param->param_val->v.b = !param->param_val->v.b;
		 gn_leaf_param_set_bool(leaf_config, "status",
		 param->param_val->v.b);

		 sprintf(gn_pump_buf, "%s - %d", leaf_config->name,
		 gn_pump_state);
		 gn_message_send_text(leaf_config, gn_pump_buf);
		 gn_message_display(gn_pump_buf);
		 gn_pump_i = 0;

		 }
		 }
		 */

		vTaskDelay(1);
	}

}

void gn_pump_display_config(gn_leaf_config_handle_t leaf_config,
		void *leaf_container, SemaphoreHandle_t _xGuiSemaphore) {

	lv_obj_t* _cnt = (lv_obj_t*) leaf_container;
	xGuiSemaphore = _xGuiSemaphore;
	char* leaf_name = gn_get_leaf_config_name(leaf_config);

	lv_obj_t *label_pump = lv_label_create(_cnt, NULL);
	lv_label_set_text(label_pump, leaf_name);

	lv_obj_align(label_pump, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_fit2(label_pump, LV_FIT_TIGHT, LV_FIT_TIGHT);

	gn_leaf_param_handle_t power_param = gn_leaf_param_get(leaf_config,
			"power");
	gn_leaf_param_handle_t status_param = gn_leaf_param_get(leaf_config,
			"status");

	label_status = lv_label_create(_cnt, NULL);
	lv_label_set_text(label_status,
			status_param->param_val->v.b ? "on" : "off");

	lv_obj_align(label_status, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_fit2(label_status, LV_FIT_TIGHT, LV_FIT_TIGHT);

	label_power = lv_label_create(_cnt, NULL);

	char _p[11];
	snprintf(_p, 10, "%f", power_param->param_val->v.d);
	lv_label_set_text(label_power, _p);

	lv_obj_align(label_power, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_fit2(label_power, LV_FIT_TIGHT, LV_FIT_TIGHT);

}

/*
 void gn_pump_display_task(gn_leaf_config_handle_t leaf_config) {

 }
 */

#ifdef __cplusplus
}
#endif //__cplusplus
