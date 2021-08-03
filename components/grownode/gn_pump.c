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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_pump.h"

#define GN_PUMP_STATE_STOP 0
#define GN_PUMP_STATE_RUNNING 1

#define GPIO_PWM0A_OUT 32

static const char *TAG = "gn_pump";

void gn_pump_task(gn_leaf_config_handle_t leaf_config) {

	//sprintf(gn_pump_buf, "%.*s init", 30, leaf_name);
	//gn_message_display(gn_pump_buf);

	size_t gn_pump_state = GN_PUMP_STATE_RUNNING;
	//bool change = true;
	gn_leaf_event_t evt;

	//init variables. TODO make status and power stored in flash
	double power = 0;
	bool status = false;

	gn_leaf_param_handle_t status_param = gn_leaf_param_create(leaf_config, "status",
			GN_VAL_TYPE_BOOLEAN, (gn_val_t ) { .b = status });
	gn_leaf_param_add(leaf_config, status_param);
	//update param if stored
	status = status_param->param_val->v.b;

	gn_leaf_param_handle_t power_param = gn_leaf_param_create(leaf_config, "power",
			GN_VAL_TYPE_DOUBLE, (gn_val_t ) { .d = power });
	gn_leaf_param_add(leaf_config, power_param);
	//update param if stored
	power = power_param->param_val->v.d;

	//gn_leaf_context_add_to_leaf(leaf_config, "status", status);
	//gn_leaf_context_add_to_leaf(leaf_config, "power", power);

	//_gn_event_group_pump = xEventGroupCreate();

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


	//setup screen

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	static lv_obj_t *label_status;
	static lv_obj_t *label_power;
	static lv_obj_t *label_pump;

	lv_obj_t *_cnt = (lv_obj_t*) gn_display_setup_leaf_display(leaf_config);

	if (_cnt) {
		//char *leaf_name = gn_get_leaf_config_name(leaf_config);

		//style
		lv_style_t *style = lv_style_list_get_local_style(&_cnt->style_list);

		label_pump = lv_label_create(_cnt, NULL);
		lv_label_set_text(label_pump, "PUMP");
		lv_obj_add_style(label_pump, LV_LABEL_PART_MAIN, style);

		gn_leaf_param_handle_t power_param = gn_leaf_param_get(leaf_config,
				"power");
		gn_leaf_param_handle_t status_param = gn_leaf_param_get(leaf_config,
				"status");

		label_status = lv_label_create(_cnt, NULL);
		lv_obj_add_style(label_status, LV_LABEL_PART_MAIN, style);
		lv_label_set_text(label_status,
				status_param->param_val->v.b ? "status: on" : "status: off");

		label_power = lv_label_create(_cnt, NULL);
		lv_obj_add_style(label_power, LV_LABEL_PART_MAIN, style);

		char _p[21];
		snprintf(_p, 20, "power: %f", power_param->param_val->v.d);
		lv_label_set_text(label_power, _p);
	}

#endif

	while (true) {

		/*
		 //test code to see if display works
		 if (pdTRUE == gn_display_leaf_refresh_start()) {

		 if (cnt == 0) {
		 lv_label_set_text(label_pump, "+");
		 cnt = 1;
		 } else {
		 lv_label_set_text(label_pump, "X");
		 cnt = 0;
		 }

		 gn_display_leaf_refresh_end();

		 }
		 */

		/*
		 xEventGroupWaitBits(_gn_event_group_pump, GN_PUMP_EVENT, pdTRUE,
		 true, pdMS_TO_TICKS(100)); //portMAX_DELAY
		 */

		if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt,
				pdMS_TO_TICKS(100)) == pdPASS) {

			switch (evt.id) {

			case GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT:

				status_param = gn_leaf_param_get(leaf_config, "status");

				if (status_param->param_val->v.b != status) {
					status = status_param->param_val->v.b;

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {

						lv_label_set_text(label_status,
								status_param->param_val->v.b ?
										"status: on" : "status: off");

						gn_display_leaf_refresh_end();
					}
				}
#endif

				power_param = gn_leaf_param_get(leaf_config, "power");

				if (power_param->param_val->v.d >= 0 && power_param->param_val->v.d <= 1024) {

					power = power_param->param_val->v.d;

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
					if (pdTRUE == gn_display_leaf_refresh_start()) {
						char _p[21];
						snprintf(_p, 20, "power: %f",
								power_param->param_val->v.d);
						lv_label_set_text(label_power, _p);

						gn_display_leaf_refresh_end();
					}
#endif
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

		if (gn_pump_state != GN_PUMP_STATE_RUNNING) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
		} else if (!status_param->param_val->v.b) {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, 0);
			//change = false;
		} else {
			mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, power);
			//change = false;
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

/*
 void gn_pump_display_config(gn_leaf_config_handle_t leaf_config,
 gn_display_handler_t leaf_container) {

 lv_obj_t *_cnt = (lv_obj_t*) leaf_container;
 char *leaf_name = gn_get_leaf_config_name(leaf_config);

 //style
 lv_style_t *style = lv_style_list_get_local_style(&_cnt->style_list);

 label_pump = lv_label_create(_cnt, NULL);
 lv_label_set_text(label_pump, "PUMP");
 lv_obj_add_style(label_pump, LV_LABEL_PART_MAIN, style);

 gn_leaf_param_handle_t power_param = gn_leaf_param_get(leaf_config,
 "power");
 gn_leaf_param_handle_t status_param = gn_leaf_param_get(leaf_config,
 "status");

 label_status = lv_label_create(_cnt, NULL);
 lv_obj_add_style(label_status, LV_LABEL_PART_MAIN, style);
 lv_label_set_text(label_status,
 status_param->param_val->v.b ? "status: on" : "status: off");

 label_power = lv_label_create(_cnt, NULL);
 lv_obj_add_style(label_power, LV_LABEL_PART_MAIN, style);

 char _p[21];
 snprintf(_p, 20, "power: %f", power_param->param_val->v.d);
 lv_label_set_text(label_power, _p);

 }
 */

/*
 void gn_pump_display_task(gn_leaf_config_handle_t leaf_config) {

 }
 */

#ifdef __cplusplus
}
#endif //__cplusplus
