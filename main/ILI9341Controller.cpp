/*
 * ILI9341Controller.cpp
 *
 *  Created on: 9 mar 2021
 *      Author: muratori.n
 */

#include "ILI9341Controller.h"
#include "GrowNodeController.h"

namespace GrowNode {
namespace Controller {

static constexpr char TAG[] = "ILI9341Controller";

//graphics
static lv_obj_t *statusLabel[10];

static char rawMessages[10][30] { };
static int rawIdx = 0;

static const int GUI_COMPLETED_EVENT = BIT0;
static EventGroupHandle_t gui_event_group;

ILI9341Controller::ILI9341Controller(GrowNodeController *_controller) :
		controller(_controller)
//: logMessages { }
{

	gui_event_group = xEventGroupCreate();
	//renderEvent = xEventGroupCreate();
	//renderMutex = xSemaphoreCreateMutex();
	//msgMutex = xSemaphoreCreateMutex();
	initialized = false;
	//statusLabel = nullptr;

}

ILI9341Controller::~ILI9341Controller() {

}

void ILI9341Controller::init() {

	if (initialized)
		return;

	ESP_LOGI(TAG, "init");

	xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL, 0, NULL, 1);

	ESP_LOGI(TAG, "guiTask created");

	xEventGroupWaitBits(gui_event_group, GUI_COMPLETED_EVENT, pdTRUE, pdTRUE,
	portMAX_DELAY);

	ESP_LOGI(TAG, "init done");

	initialized = true;

}

extern "C" {

#define LV_TICK_PERIOD_MS 1

static SemaphoreHandle_t xGuiSemaphore;

void ILI9341Controller::guiTask(void *pvParameter) {

	(void) pvParameter;
	xGuiSemaphore = xSemaphoreCreateMutex();

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
			&lv_tick_task, .name = "periodic_gui" };
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(
			esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

	/* Create the demo application */
	create_gui();
	ESP_ERROR_CHECK(
			esp_event_handler_instance_register(MESSAGE_EVENT, LOG_SYSTEM, log_system_handler, NULL, NULL));

	xEventGroupSetBits(gui_event_group, GUI_COMPLETED_EVENT);

	ESP_LOGI(TAG, "xEventGroupSetBits gui_event_group");

	while (1) {
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

		/* Try to take the semaphore, call lvgl related function on success */
		if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
			lv_task_handler();
			xSemaphoreGive(xGuiSemaphore);
		}
	}

	/* A task should NEVER return */
	free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	free(buf2);
#endif
	vTaskDelete(NULL);
}

void ILI9341Controller::create_gui(void) {

	lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    //style
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_border_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&style, LV_STATE_DEFAULT, 0);

    //main container
    lv_obj_t *cont = lv_cont_create(scr, NULL);
    lv_obj_add_style(cont, LV_CONT_PART_MAIN, &style);
    lv_obj_align(cont, scr, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_cont_set_fit(cont, LV_FIT_MAX);
    lv_cont_set_layout(cont, LV_LAYOUT_COLUMN_LEFT);

    //button container
    lv_obj_t *btcont = lv_cont_create(scr, NULL);
    lv_obj_add_style(btcont, LV_CONT_PART_MAIN, &style);
    lv_obj_align(btcont, scr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_cont_set_fit(btcont, LV_FIT_TIGHT);
    lv_cont_set_layout(btcont, LV_LAYOUT_COLUMN_LEFT);

    //label title
	lv_obj_t *label1 = lv_label_create(cont, NULL);
	lv_obj_add_style(label1, LV_LABEL_PART_MAIN, &style);
	lv_label_set_text(label1, "GrowNode Board");
	lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

	for (int row = 0; row < 10; row++) {

		//log labels
		statusLabel[row] = lv_label_create(cont, NULL);
		lv_obj_add_style(statusLabel[row], LV_LABEL_PART_MAIN, &style);
		lv_label_set_text(statusLabel[row], "");
		if (row == 0) {
			lv_obj_align(statusLabel[row], NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0,
					-10);
		} else {
			lv_obj_align(statusLabel[row], statusLabel[row - 1],
					LV_ALIGN_IN_TOP_LEFT, 0, -10);
		}

	}

	//buttons
    lv_obj_t * label_ota;

    lv_obj_t * btn_ota = lv_btn_create(btcont, NULL);
    lv_obj_set_event_cb(btn_ota, ota_event_handler);
    lv_obj_align(btn_ota, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_btn_set_checkable(btn_ota, true);
    lv_btn_toggle(btn_ota);
    lv_btn_set_fit2(btn_ota, LV_FIT_NONE, LV_FIT_TIGHT);

    label_ota = lv_label_create(btn_ota, NULL);
    lv_label_set_text(label_ota, "Firmare Update");

	/*
	 statusLabel = lv_label_create(scr, NULL);
	 lv_label_set_text(statusLabel, "Startup....");
	 lv_obj_align(statusLabel, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
	 */

}

void ILI9341Controller::ota_event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
    	ESP_LOGI(TAG, "ota_event_handler - clicked");
        GrowNodeController::updateFirmware();
    }
    else if(event == LV_EVENT_VALUE_CHANGED) {
    	ESP_LOGI(TAG, "ota_event_handler - toggled");

    }
}

void ILI9341Controller::lv_tick_task(void *arg) {
	(void) arg;

	lv_tick_inc(LV_TICK_PERIOD_MS);
}

void ILI9341Controller::log_system_handler(void *handler_args,
		esp_event_base_t base, int32_t id, void *event_data) {

	if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {

		char *message = (char*) event_data;

		//lv_label_set_text(statusLabel, message.c_str());
		//const char *t = text.c_str();

		if (rawIdx >= 9) {

			//scroll messages
			for (int row = 1; row < 10; row++) {
				//ESP_LOGI(TAG, "scrolling %i from %s to %s", row, rawMessages[row], rawMessages[row - 1]);
				strncpy(rawMessages[row - 1], rawMessages[row], 30);
			}
			strncpy(rawMessages[9], message, 30);
		} else {
			//ESP_LOGI(TAG, "setting %i", rawIdx);
			strncpy(rawMessages[rawIdx], message, 30);
			rawIdx++;
		}

		//ESP_LOGI(TAG, "printing %s", message);
		//print
		for (int row = 0; row < 10; row++) {
			//ESP_LOGI(TAG, "label %i to %s", 9 - row, rawMessages[row]);
			lv_label_set_text(statusLabel[9 - row], rawMessages[row]);
		}
		xSemaphoreGive(xGuiSemaphore);
	}
}

} /* extern "C" */

} /* namespace Controller */
} /* namespace GrowNode */
