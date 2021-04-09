/*
 * GrowNode.cpp
 *
 *  Created on: 22 mar 2021
 *      Author: muratori.n
 */

#include "GrowNodeController.h"

namespace GrowNode {
namespace Controller {

static const char *TAG = "GrowNodeController";
GrowNodeController *GrowNodeController::instance = nullptr;

GrowNodeController::GrowNodeController() :
		wifi() {
	initialized = false;
	init();
}

GrowNodeController::~GrowNodeController() {

}

GrowNodeController* GrowNodeController::getInstance() {

	if (instance == nullptr) {
		instance = new GrowNodeController();
	}

	return instance;
}

void GrowNodeController::init() {

	if (initialized)
		return;

	ESP_LOGI(TAG, "init - START");

	//gpio_install_isr_service(0);

	ESP_LOGI(TAG, "initFlash");
	initFlash();
	ESP_LOGI(TAG, "initEventLoop");
	initEventLoop();
	ESP_LOGI(TAG, "initSPIFFS");
	initSPIFFS();
	ESP_LOGI(TAG, "initGUI");
	initGUI();
	ESP_LOGI(TAG, "initWifi");
	wifi.init();

	ESP_LOGI(TAG, "init - END");

	initialized = true;
}

void GrowNodeController::initEventLoop() {

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5, .task_name = "loop_task", // task will be created
			.task_priority = uxTaskPriorityGet(NULL), .task_stack_size = 2048, .task_core_id = tskNO_AFFINITY };
	ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &eventLoop));
	//ESP_ERROR_CHECK(esp_event_handler_register_with(eventLoop, TASK_EVENTS, TEXT_EVENT, textEventHandlerCallback, eventLoop));
}

void GrowNodeController::initFlash() {
	/* Initialize NVS partition */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		/* NVS partition was truncated
		 * and needs to be erased */
		ESP_ERROR_CHECK(nvs_flash_erase());
		/* Retry nvs_flash_init */
		ESP_ERROR_CHECK(nvs_flash_init());
	}

#ifdef CONFIG_GROWNODE_RESET_PROVISIONED
	ESP_ERROR_CHECK(nvs_flash_erase());
	/* Retry nvs_flash_init */
	ESP_ERROR_CHECK(nvs_flash_init());
#endif

}

void GrowNodeController::initGUI() {

	xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL, 0, NULL, 1);

}

void GrowNodeController::initSPIFFS() {

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t vfs_conf;
	vfs_conf.base_path = "/spiffs";
	vfs_conf.partition_label = NULL;
	vfs_conf.max_files = 6;
	vfs_conf.format_if_mount_failed = true;

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&vfs_conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",
					esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
				esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	/*
	 DIR *dir = opendir("/spiffs");
	 assert(dir != NULL);
	 while (true) {
	 struct dirent *pe = readdir(dir);
	 if (!pe)
	 break;
	 ESP_LOGI(TAG, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino,
	 pe->d_type);
	 }
	 closedir(dir);
	 */

}

extern "C" {

static SemaphoreHandle_t xGuiSemaphore;

void GrowNodeController::guiTask(void *pvParameter) {

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

void GrowNodeController::create_gui(void) {

	/* use a pretty small demo for monochrome displays */
	/* Get the current screen  */
	lv_obj_t *scr = lv_disp_get_scr_act(NULL);

	/*Create a Label on the currently active screen*/
	lv_obj_t *label1 = lv_label_create(scr, NULL);

	/*Modify the Label's text*/
	lv_label_set_text(label1, "Hello\nworld");

	/* Align the Label to the center
	 * NULL means align on parent (which is the screen now)
	 * 0, 0 at the end means an x, y offset after alignment*/
	lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);

}

void GrowNodeController::lv_tick_task(void *arg) {
	(void) arg;

	lv_tick_inc(LV_TICK_PERIOD_MS);
}

} /* extern "C" */

} /* namespace Controller */
} /* namespace GrowNode */
