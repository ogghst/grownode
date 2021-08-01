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

#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_display.h"
#include "gn_commons.h"
#include "grownode_intl.h"
#include "gn_event_source.h"

#define LV_TICK_PERIOD_MS 1

static const char *TAG = "grownode";

gn_config_handle_t _config;

//event groups
const int GN_EVT_GROUP_GUI_COMPLETED_EVENT = BIT0;
EventGroupHandle_t _gn_gui_event_group;

//TODO revise log structure with structs

#define SLSIZE 4

lv_obj_t *statusLabel[SLSIZE];
char rawMessages[SLSIZE][30];
int rawIdx = 0;

//lv_obj_t *network_status_label, *server_status_label;
lv_obj_t *network_led, *server_led;

static lv_obj_t *leaf_cont;
static lv_style_t style;

bool _initialized = false;

void _gn_display_lv_tick_task(void *arg) {
	(void) arg;
	lv_tick_inc(LV_TICK_PERIOD_MS);
}

void _gn_display_btn_ota_event_handler(lv_obj_t *obj, lv_event_t event) {
	if (event == LV_EVENT_CLICKED) {
		ESP_LOGD(TAG, "_gn_display_btn_ota_event_handler - clicked");

		ESP_ERROR_CHECK(
				esp_event_post_to(gn_get_config_event_loop(_config), GN_BASE_EVENT, GN_NET_OTA_START, NULL, 0, portMAX_DELAY));

	} else if (event == LV_EVENT_VALUE_CHANGED) {
		ESP_LOGD(TAG, "_gn_display_btn_ota_event_handler - toggled");

	}
}

void _gn_display_btn_rst_event_handler(lv_obj_t *obj, lv_event_t event) {
	if (event == LV_EVENT_CLICKED) {

		ESP_LOGD(TAG, "_gn_display_btn_rst_event_handler - clicked");

		ESP_ERROR_CHECK(
				esp_event_post_to(gn_get_config_event_loop(_config), GN_BASE_EVENT, GN_NET_RST_START, NULL, 0, portMAX_DELAY));

		ESP_LOGD(TAG, "_gn_display_btn_rst_event_handler - sent event");

	} else if (event == LV_EVENT_VALUE_CHANGED) {
		ESP_LOGD(TAG, "_gn_display_btn_rst_event_handler - toggled");

	}
}

void _gn_display_log_system_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	if (pdTRUE == xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY)) {

		char *message = (char*) event_data;

		//lv_label_set_text(statusLabel, message.c_str());
		//const char *t = text.c_str();

		if (rawIdx > SLSIZE-1) {

			//scroll messages
			for (int row = 1; row < SLSIZE; row++) {
				//ESP_LOGI(TAG, "scrolling %i from %s to %s", row, rawMessages[row], rawMessages[row - 1]);
				strncpy(rawMessages[row - 1], rawMessages[row], 30);
			}
			strncpy(rawMessages[SLSIZE-1], message, 30);
		} else {
			//ESP_LOGI(TAG, "setting %i", rawIdx);
			strncpy(rawMessages[rawIdx], message, 30);
			rawIdx++;
		}

		//ESP_LOGI(TAG, "printing %s", message);
		//print
		for (int row = 0; row < SLSIZE; row++) {
			//ESP_LOGI(TAG, "label %i to %s", 9 - row, rawMessages[row]);
			lv_label_set_text(statusLabel[row], rawMessages[row]);
		}

		xSemaphoreGive(_gn_xGuiSemaphore);
	}

}

void _gn_display_net_mqtt_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	//TODO pass wifi info to the event
	//char *message = (char*) event_data;

	if (pdTRUE == xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY)) {

		switch (id) {
		case GN_NETWORK_CONNECTED_EVENT:
			//lv_label_set_text(network_status_label, "NET OK");
			lv_led_on(network_led);
			break;
		case GN_NETWORK_DISCONNECTED_EVENT:
			//lv_label_set_text(network_status_label, "NET KO");
			lv_led_off(network_led);
			break;
		case GN_SERVER_CONNECTED_EVENT:
			//lv_label_set_text(server_status_label, "SRV OK");
			lv_led_on(server_led);
			break;
		case GN_SERVER_DISCONNECTED_EVENT:
			//lv_label_set_text(server_status_label, "SRV_KO");
			lv_led_off(server_led);
			break;
		}

		xSemaphoreGive(_gn_xGuiSemaphore);
	}

}

BaseType_t gn_display_leaf_refresh_start() {
	return xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY);
}

BaseType_t gn_display_leaf_refresh_end() {
	return xSemaphoreGive(_gn_xGuiSemaphore);
}

void _gn_display_create_gui() {

	lv_obj_t *scr = lv_disp_get_scr_act(NULL);

	//style
	lv_style_init(&style);
	//lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_text_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_bg_color(&style, LV_STATE_PRESSED, LV_COLOR_BLACK);
	lv_style_set_text_color(&style, LV_STATE_PRESSED, LV_COLOR_WHITE);
	lv_style_set_border_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_border_width(&style, LV_STATE_DEFAULT, 0);
	lv_style_set_margin_all(&style, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_all(&style, LV_STATE_DEFAULT, 1);
	lv_style_set_radius(&style, LV_STATE_DEFAULT, 0);

	static lv_style_t style_log;
	lv_style_copy(&style_log, &style);
	//lv_style_init(&style_log);
	//lv_style_set_bg_opa(&style_log, LV_STATE_DEFAULT, LV_OPA_COVER);
	//lv_style_set_bg_color(&style_log, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	//lv_style_set_text_color(&style_log, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	//lv_style_set_border_color(&style_log, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	//lv_style_set_radius(&style_log, LV_STATE_DEFAULT, 0);

	static lv_style_t style_btn;
	//lv_style_init(&style_btn);
	lv_style_copy(&style_btn, &style);
	lv_style_set_bg_opa(&style_btn, LV_STATE_DEFAULT, LV_OPA_COVER);

	lv_style_set_bg_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_text_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_bg_color(&style_btn, LV_STATE_PRESSED, LV_COLOR_GREEN);
	lv_style_set_text_color(&style_btn, LV_STATE_PRESSED, LV_COLOR_BLACK);

	lv_style_set_border_color(&style_btn, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_border_width(&style_btn, LV_STATE_DEFAULT, 1);
	lv_style_set_pad_all(&style_btn, LV_STATE_DEFAULT, 5);
	lv_style_set_radius(&style_btn, LV_STATE_DEFAULT, 5);

	static lv_style_t style_led;
	//lv_style_init(&style_led);
	lv_style_copy(&style_led, &style);
	lv_style_set_bg_opa(&style_led, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_bg_color(&style_led, LV_STATE_DEFAULT, LV_COLOR_GREEN);
	lv_style_set_border_color(&style_led, LV_STATE_DEFAULT, LV_COLOR_GREEN);
	lv_style_set_border_width(&style_led, LV_STATE_DEFAULT, 1);

	//main container
	lv_obj_t *cont = lv_cont_create(scr, NULL);
	lv_obj_add_style(cont, LV_CONT_PART_MAIN, &style);
	//lv_obj_set_style_local_radius(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
	lv_obj_align(cont, scr, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_cont_set_fit(cont, LV_FIT_MAX);
	lv_cont_set_layout(cont, LV_LAYOUT_COLUMN_MID);

	//title container
	lv_obj_t *title_cont = lv_cont_create(cont, NULL);
	lv_obj_add_style(title_cont, LV_CONT_PART_MAIN, &style);
	lv_obj_align(title_cont, cont, LV_ALIGN_IN_TOP_MID, 0, 0);
	//lv_cont_set_fit(title_cont, LV_FIT_TIGHT);
	lv_cont_set_fit2(title_cont, LV_FIT_MAX, LV_FIT_TIGHT);
	//lv_obj_set_width(title_cont, 240);
	//lv_obj_set_height(title_cont, 20);
	lv_cont_set_layout(title_cont, LV_LAYOUT_COLUMN_MID);

	//log container
	lv_obj_t *log_cont = lv_cont_create(cont, NULL);
	lv_obj_add_style(log_cont, LV_CONT_PART_MAIN, &style);
	lv_obj_align(log_cont, title_cont, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_cont_set_fit2(log_cont, LV_FIT_MAX, LV_FIT_TIGHT);
	//lv_obj_set_width(log_cont, 240);
	lv_obj_set_height(log_cont, 260);
	lv_cont_set_layout(log_cont, LV_LAYOUT_COLUMN_LEFT);

	//leaf container
	leaf_cont = lv_cont_create(cont, NULL);
	lv_obj_add_style(leaf_cont, LV_CONT_PART_MAIN, &style);
	lv_obj_align(leaf_cont, title_cont, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_cont_set_fit2(leaf_cont, LV_FIT_MAX, LV_FIT_TIGHT);
	//lv_obj_set_width(log_cont, 240);
	//lv_obj_set_height(log_cont, 260);
	lv_cont_set_layout(leaf_cont, LV_LAYOUT_COLUMN_LEFT);

	//label leave
	lv_obj_t *label_leaf_cont = lv_label_create(leaf_cont, NULL);
	lv_obj_add_style(label_leaf_cont, LV_LABEL_PART_MAIN, &style_log);
	lv_label_set_text(label_leaf_cont, "Leaves");



	//bottom container
	lv_obj_t *bottom_cont = lv_cont_create(cont, NULL);
	lv_obj_add_style(bottom_cont, LV_CONT_PART_MAIN, &style);
	lv_obj_align(bottom_cont, cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
	lv_cont_set_fit2(bottom_cont, LV_FIT_MAX, LV_FIT_TIGHT);
	//lv_obj_set_width(button_cont, 240);
	//lv_obj_set_height(button_cont, 40);
	lv_cont_set_layout(bottom_cont, LV_LAYOUT_ROW_MID);

	//label title
	lv_obj_t *label_title = lv_label_create(title_cont, NULL);
	lv_obj_add_style(label_title, LV_LABEL_PART_MAIN, &style_log);
	lv_label_set_text(label_title, "GrowNode Board");
	//lv_obj_align(label_title, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

	//log labels

	for (int row = 0; row < SLSIZE; row++) {

		//log labels
		statusLabel[row] = lv_label_create(log_cont, NULL);
		lv_obj_add_style(statusLabel[row], LV_LABEL_PART_MAIN, &style_log);
		lv_label_set_text(statusLabel[row], "");
		lv_obj_set_width_fit(statusLabel[row], LV_FIT_MAX);

		if (row == 0) {
			lv_obj_align(statusLabel[row], NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

		} else {
			lv_obj_align(statusLabel[row], statusLabel[row - 1],
					LV_ALIGN_IN_TOP_LEFT, 0, 0);
		}

	}

	//TODO put into a specific configuration panel
	//buttons
	lv_obj_t *label_ota;

	lv_obj_t *btn_ota = lv_btn_create(bottom_cont, NULL);
	lv_obj_add_style(btn_ota, LV_CONT_PART_MAIN, &style_btn);
	lv_obj_set_event_cb(btn_ota, _gn_display_btn_ota_event_handler);
	lv_obj_align(btn_ota, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_checkable(btn_ota, false);
	//lv_btn_toggle(btn_ota);
	lv_btn_set_fit2(btn_ota, LV_FIT_TIGHT, LV_FIT_TIGHT);

	label_ota = lv_label_create(btn_ota, NULL);
	lv_label_set_text(label_ota, "OTA");

	lv_obj_t *label_rst;

	lv_obj_t *btn_rst = lv_btn_create(bottom_cont, NULL);
	lv_obj_add_style(btn_rst, LV_CONT_PART_MAIN, &style_btn);
	lv_obj_set_event_cb(btn_rst, _gn_display_btn_rst_event_handler);
	lv_obj_align(btn_rst, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_checkable(btn_rst, false);
	//lv_btn_toggle(btn_ota);
	lv_btn_set_fit2(btn_rst, LV_FIT_TIGHT, LV_FIT_TIGHT);

	label_rst = lv_label_create(btn_rst, NULL);
	lv_label_set_text(label_rst, "RST");

	network_led = lv_led_create(bottom_cont, NULL);
	lv_obj_add_style(network_led, LV_LED_PART_MAIN, &style_led);
	lv_obj_align(network_led, bottom_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
	lv_obj_set_size(network_led, 10, 10);
	lv_led_set_bright(network_led, 255);
	lv_led_off(network_led);

	server_led = lv_led_create(bottom_cont, NULL);
	lv_obj_add_style(server_led, LV_LED_PART_MAIN, &style_led);
	lv_obj_align(server_led, bottom_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
	lv_obj_set_size(server_led, 10, 10);
	lv_led_set_bright(server_led, 255);
	lv_led_on(server_led);

	/*
	 //network status
	 network_status_label = lv_label_create(bottom_cont, NULL);
	 lv_obj_add_style(network_status_label, LV_LABEL_PART_MAIN, &style);
	 lv_obj_align(network_status_label, bottom_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0,
	 0);
	 lv_obj_set_width_fit(network_status_label, LV_FIT_MAX);
	 lv_label_set_text(network_status_label, "NET KO");

	 //server status
	 server_status_label = lv_label_create(bottom_cont, NULL);
	 lv_obj_add_style(server_status_label, LV_LABEL_PART_MAIN, &style);
	 lv_obj_align(server_status_label, bottom_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0,
	 0);
	 lv_obj_set_width_fit(server_status_label, LV_FIT_MAX);
	 lv_label_set_text(server_status_label, "SRV KO");
	 */
}

/*
void gn_display_leaf_start(gn_leaf_config_handle_t leaf_config) {

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	//create a leaf container
	//leaf container
	lv_obj_t *_a_leaf_cont = lv_cont_create(leaf_cont, NULL);
	lv_obj_add_style(_a_leaf_cont, LV_CONT_PART_MAIN, &style);
	lv_obj_align(_a_leaf_cont, leaf_cont, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_cont_set_fit2(_a_leaf_cont, LV_FIT_MAX, LV_FIT_TIGHT);
	//lv_obj_set_width(log_cont, 240);
	//lv_obj_set_height(log_cont, 260);
	lv_cont_set_layout(_a_leaf_cont, LV_LAYOUT_COLUMN_LEFT);

	//((gn_leaf_config_handle_intl_t)leaf_config)->display_config_cb(leaf_config, _a_leaf_cont);
	((gn_leaf_config_handle_intl_t)leaf_config)->display_handler = _a_leaf_cont;

#endif

}
*/

void gn_display_setup_leaf_display(gn_leaf_config_handle_t leaf_config, gn_display_handler_t display_handler) {

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED
	//create a leaf container
	//leaf container
	lv_obj_t *_a_leaf_cont = lv_cont_create(leaf_cont, NULL);
	lv_obj_add_style(_a_leaf_cont, LV_CONT_PART_MAIN, &style);
	lv_obj_align(_a_leaf_cont, leaf_cont, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_cont_set_fit2(_a_leaf_cont, LV_FIT_MAX, LV_FIT_TIGHT);
	//lv_obj_set_width(log_cont, 240);
	//lv_obj_set_height(log_cont, 260);
	lv_cont_set_layout(_a_leaf_cont, LV_LAYOUT_COLUMN_LEFT);

	((gn_leaf_config_handle_intl_t)leaf_config)->display_handler = _a_leaf_cont;
	((gn_leaf_config_handle_intl_t)leaf_config)->display_config_cb(leaf_config, _a_leaf_cont);
#endif
}

void _gn_display_gui_task(void *pvParameter) {

	(void) pvParameter;
	_gn_xGuiSemaphore = xSemaphoreCreateMutex();

	lv_init();

	/* Initialize SPI or I2C bus used by the drivers */
	lvgl_driver_init();

	lv_color_t *buf1 = (lv_color_t*) heap_caps_malloc(
	DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf1 != NULL);

	/* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	lv_color_t *buf2 = (lv_color_t*) heap_caps_malloc(
	DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

	static lv_disp_buf_t disp_buf;

	uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

    /* Actual size in pixels, not bytes. */
    size_in_px *= 8;
#endif

	/* Initialize the working buffer depending on the selected display.
	 * NOTE: buf2 == NULL when using monochrome displays. */
	lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;

	/* When using a monochrome display we need to register the callbacks:
	 * - rounder_cb
	 * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	/* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);

#endif

	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	const esp_timer_create_args_t periodic_timer_args = { .callback =
			&_gn_display_lv_tick_task, .name = "_gn_display_lv_tick_task" };
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(
			esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

	_gn_display_create_gui();

	xEventGroupSetBits(_gn_gui_event_group, GN_EVT_GROUP_GUI_COMPLETED_EVENT);

	while (1) {
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

		/* Try to take the semaphore, call lvgl related function on success */
		if (pdTRUE == gn_display_leaf_refresh_start()) {
			//ESP_LOGE(TAG, "LVGL handle");
			lv_task_handler();

			gn_display_leaf_refresh_end();
		}
	}

	/* A task should NEVER return */
	free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	free(buf2);
#endif
	vTaskDelete(NULL);
}

esp_err_t gn_init_display(gn_config_handle_t config) {

	gn_config_handle_intl_t conf = (gn_config_handle_intl_t)config;

	esp_err_t ret = ESP_OK;

	if (_initialized)
		return ret;

	//TODO initialization guards

#ifndef CONFIG_GROWNODE_DISPLAY_ENABLED
	return ESP_OK;
#endif

	_gn_gui_event_group = xEventGroupCreate();

	ESP_LOGI(TAG, "gn_init_display");

	xTaskCreatePinnedToCore(_gn_display_gui_task, "_gn_display_gui_task",
			4096 * 2, NULL, 1, NULL, 1);

	ESP_LOGD(TAG, "_gn_display_gui_task created");

	xEventGroupWaitBits(_gn_gui_event_group, GN_EVT_GROUP_GUI_COMPLETED_EVENT,
	pdTRUE, pdTRUE, portMAX_DELAY);

	//add event handlers
	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_DISPLAY_LOG_EVENT, _gn_display_log_system_handler, NULL, NULL));

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(conf->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, _gn_display_net_mqtt_handler, NULL, NULL));

	ESP_LOGD(TAG, "gn_init_display done");

	//TODO dangerous, better update through events
	_config = conf;

	_initialized = true;

	return ESP_OK;

}

#ifdef __cplusplus
}
#endif //__cplusplus
