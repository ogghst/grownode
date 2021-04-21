#include "gn_pump.h"

#ifdef __cplusplus
extern "C" {
#endif

void _gn_pump_task(void *pvParam) {

	while (1) {

		vTaskDelay(2000 / portTICK_PERIOD_MS);

		gn_log_message("pump tick");

	}

}

void gn_pump_init(gn_leaf_config_handle_t config) {

	xTaskCreate(_gn_pump_task, "_gn_pump_task", 2048, NULL, 0, NULL);

}

void gn_pump_callback(gn_event_handle_t event, void *event_data) {

	gn_log_message("gn_pump_callback");

}

#ifdef __cplusplus
}
#endif //__cplusplus
