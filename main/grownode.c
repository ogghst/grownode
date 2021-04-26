/*
 * grownode.c
 *
 *  Created on: 17 apr 2021
 *      Author: muratori.n
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
//#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
//#include "esp_smartconfig.h"

#include "nvs_flash.h"

#include "driver/gpio.h"

#include "mqtt_client.h"

#include "lwip/apps/sntp.h"

#include "wifi_provisioning/manager.h"

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
#include "wifi_provisioning/scheme_ble.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
#include "wifi_provisioning/scheme_softap.h"
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

#include "gn_event_source.h"
#include "gn_mqtt_protocol.h"

static const char *TAG = "grownode";

esp_event_loop_handle_t gn_event_loop;

void _gn_init_event_loop(gn_config_handle_t conf) {
	//default event loop
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//user event loop
	esp_event_loop_args_t event_loop_args = { .queue_size = 5,
			.task_name = "loop_task", // task will be created
			.task_priority = 0, .task_stack_size = 2048,
			.task_core_id = 1 };
	ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &gn_event_loop));

	conf->event_loop = gn_event_loop;
}

gn_node_config_handle_t _gn_create_node_config() {
	gn_node_config_handle_t _conf = (gn_node_config_handle_t) malloc(
			sizeof(gn_node_config_t));
	_conf->config = NULL;
	_conf->event_loop = NULL;
	_conf->name = NULL;
	return _conf;
}

gn_node_config_handle_t gn_create_node(gn_config_handle_t config,
		const char *name) {

	if (config == NULL || config->mqtt_client == NULL || name == NULL) {
		ESP_LOGE(TAG, "gn_create_node failed. parameters not correct");
		return NULL;
	}

	gn_node_config_handle_t n_c = _gn_create_node_config();

	//n_c->name = bf;
	n_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(n_c->name, name, GN_MEM_NAME_SIZE);
	n_c->event_loop = config->event_loop;
	n_c->config = config;

	return n_c;
}

esp_err_t gn_destroy_node(gn_node_config_handle_t node) {

	free(node->name);
	free(node);

	return ESP_OK;
}

gn_leaf_config_handle_t _gn_create_leaf_config() {
	gn_leaf_config_handle_t _conf = (gn_leaf_config_handle_t) malloc(
			sizeof(gn_leaf_config_t));
	_conf->callback = NULL;
	_conf->name = NULL;
	_conf->node_config = NULL;
	return _conf;
}

gn_leaf_config_handle_t gn_create_leaf(gn_node_config_handle_t node_cfg,
		const char *name, gn_event_callback_t callback) {

	if (node_cfg == NULL || node_cfg->config == NULL
			|| node_cfg->config->mqtt_client == NULL || name == NULL
			|| callback == NULL) {
		ESP_LOGE(TAG, "gn_create_leaf failed. parameters not correct");
		return NULL;
	}

	gn_leaf_config_handle_t l_c = _gn_create_leaf_config();

	l_c->name = (char*) malloc(GN_MEM_NAME_SIZE * sizeof(char));
	strncpy(l_c->name, name, GN_MEM_NAME_SIZE);
	l_c->node_config = node_cfg;
	l_c->callback = callback;

	//mqtt subscribe to topic
	_gn_mqtt_subscribe_leaf(l_c);

	return l_c;
}

esp_err_t gn_destroy_leaf(gn_leaf_config_handle_t leaf) {

	free(leaf->name);
	free(leaf);

	return ESP_OK;

}

void _gn_init_flash(gn_config_handle_t conf) {
	/* Initialize NVS partition */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
			|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
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

void _gn_init_spiffs(gn_config_handle_t conf) {

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t vfs_conf;
	vfs_conf.base_path = "/spiffs";
	vfs_conf.partition_label = NULL;
	vfs_conf.max_files = 6;
	vfs_conf.format_if_mount_failed = true;

	conf->spiffs_conf = vfs_conf;

	// Use settings defined above to initialize and mount SPIFFS filesystem.
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
}

/*
 *
 * DISPLAY
 *
 *
 */

#define LV_TICK_PERIOD_MS 1

//event groups
const int GN_EVT_GROUP_GUI_COMPLETED_EVENT = BIT0;
EventGroupHandle_t _gn_gui_event_group;

SemaphoreHandle_t _gn_xGuiSemaphore;

bool initialized = false;

lv_obj_t *statusLabel[10];

char rawMessages[10][30];
int rawIdx = 0;

ESP_EVENT_DEFINE_BASE( GN_BASE_EVENT);

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

void gn_log_message(const char *message) {

	//void *ptr = const_cast<void*>(reinterpret_cast<void const*>(message));
	ESP_LOGI(TAG, "gn_log_message '%s' (%i)", message, strlen(message));

	//char *ptr = malloc(sizeof(char) * strlen(message) + 1);


	ESP_ERROR_CHECK(
			esp_event_post_to(gn_event_loop, GN_BASE_EVENT, GN_DISPLAY_LOG_SYSTEM, message,
					strlen(message) + 1, portMAX_DELAY));

	//free(ptr);

	ESP_LOGI(TAG, "end gn_log_message");
}

void _gn_ota_task(void *pvParameter) {

	//vTaskDelay(30000 / portTICK_PERIOD_MS);

	//screenPayload x;
	//x.type = screenPayloadType::LOG;
	//strcpy(x.text, "Firmware update start");
	//xQueueSend(screenEventQueue, &x, 0);
	gn_log_message("OTA in progress..");

	esp_wifi_set_ps(WIFI_PS_NONE);

	esp_http_client_config_t config = { };
	config.url = CONFIG_GROWNODE_FIRMWARE_URL;
	//config.event_handler = _http_event_handler;
	config.cert_pem = (char*) server_cert_pem_start;

	esp_err_t ret = esp_https_ota(&config);
	if (ret == ESP_OK) {

		gn_log_message("Firmware updated. Rebooting..");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		esp_restart();

	} else {

		//screen->log("Firmware Not Updated");
		gn_log_message("Firmware upgrade failed.");
		//vTaskDelay(5000 / portTICK_PERIOD_MS);
		//esp_restart();
	}

	vTaskDelete(NULL);

}

void _gn_update_firmware() {
	xTaskCreate(_gn_ota_task, "_gn_ota_task", 8196, NULL, 10, NULL);
}

void _gn_display_lv_tick_task(void *arg) {
	(void) arg;
	lv_tick_inc(LV_TICK_PERIOD_MS);
}

void _gn_display_btn_ota_event_handler(lv_obj_t *obj, lv_event_t event) {
	if (event == LV_EVENT_CLICKED) {
		ESP_LOGI(TAG, "_gn_display_btn_ota_event_handler - clicked");
		_gn_update_firmware();
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		ESP_LOGI(TAG, "_gn_display_btn_ota_event_handler - toggled");

	}
}

void _gn_display_btn_rst_event_handler(lv_obj_t *obj, lv_event_t event) {
	if (event == LV_EVENT_CLICKED) {
		ESP_LOGI(TAG, "_gn_display_btn_rst_event_handler - clicked");
		gn_log_message("resetting flash");
		nvs_flash_erase();
		gn_log_message("reboot in 3 sec");

		vTaskDelay(3000 / portTICK_PERIOD_MS);

		esp_restart();

	} else if (event == LV_EVENT_VALUE_CHANGED) {
		ESP_LOGI(TAG, "_gn_display_btn_rst_event_handler - toggled");

	}
}

void _gn_display_log_system_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	char *message = (char*) event_data;

	//lv_label_set_text(statusLabel, message.c_str());
	//const char *t = text.c_str();

	if (rawIdx > 9) {

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

	//if (pdTRUE == xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY)) {

		//ESP_LOGI(TAG, "printing %s", message);
		//print
		for (int row = 0; row < 10; row++) {
			//ESP_LOGI(TAG, "label %i to %s", 9 - row, rawMessages[row]);
			lv_label_set_text(statusLabel[row], rawMessages[row]);
		}
	//	xSemaphoreGive(_gn_xGuiSemaphore);
	//}

}

void _gn_display_create_gui() {

	lv_obj_t *scr = lv_disp_get_scr_act(NULL);

	//style
	static lv_style_t style;
	lv_style_init(&style);
	//lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_text_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	lv_style_set_bg_color(&style, LV_STATE_PRESSED, LV_COLOR_RED);
	lv_style_set_text_color(&style, LV_STATE_PRESSED, LV_COLOR_WHITE);
	//lv_style_set_border_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_border_width(&style, LV_STATE_DEFAULT, 0);
	lv_style_set_margin_all(&style, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_all(&style, LV_STATE_DEFAULT, 2);
	//lv_style_set_radius(&style, LV_STATE_DEFAULT, 0);

	static lv_style_t style_log;
	lv_style_init(&style_log);
	//lv_style_set_bg_opa(&style_log, LV_STATE_DEFAULT, LV_OPA_COVER);
	lv_style_set_bg_color(&style_log, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_text_color(&style_log, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	//lv_style_set_border_color(&style_log, LV_STATE_DEFAULT, LV_COLOR_WHITE);
	//lv_style_set_radius(&style_log, LV_STATE_DEFAULT, 0);

	//main container
	lv_obj_t *cont = lv_cont_create(scr, NULL);
	lv_obj_add_style(cont, LV_CONT_PART_MAIN, &style);
	lv_obj_set_style_local_radius(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
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
	//lv_obj_set_height(log_cont, 260);
	lv_cont_set_layout(log_cont, LV_LAYOUT_COLUMN_LEFT);

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

	for (int row = 0; row < 10; row++) {

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

	//bottom container

	//buttons
	lv_obj_t *label_ota;

	lv_obj_t *btn_ota = lv_btn_create(bottom_cont, NULL);
	lv_obj_add_style(btn_ota, LV_CONT_PART_MAIN, &style);
	lv_obj_set_event_cb(btn_ota, _gn_display_btn_ota_event_handler);
	lv_obj_align(btn_ota, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_checkable(btn_ota, false);
	//lv_btn_toggle(btn_ota);
	lv_btn_set_fit2(btn_ota, LV_FIT_TIGHT, LV_FIT_TIGHT);

	label_ota = lv_label_create(btn_ota, NULL);
	lv_label_set_text(label_ota, "OTA");

	lv_obj_t *label_rst;

	lv_obj_t *btn_rst = lv_btn_create(bottom_cont, NULL);
	lv_obj_add_style(btn_rst, LV_CONT_PART_MAIN, &style);
	lv_obj_set_event_cb(btn_rst, _gn_display_btn_rst_event_handler);
	lv_obj_align(btn_rst, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_btn_set_checkable(btn_rst, false);
	//lv_btn_toggle(btn_ota);
	lv_btn_set_fit2(btn_rst, LV_FIT_TIGHT, LV_FIT_TIGHT);

	label_rst = lv_label_create(btn_rst, NULL);
	lv_label_set_text(label_rst, "RST");

	//connection status
	lv_obj_t *connection_label = lv_label_create(bottom_cont, NULL);
	lv_obj_add_style(connection_label, LV_LABEL_PART_MAIN, &style_log);
	lv_obj_align(connection_label, bottom_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
	lv_obj_set_width_fit(connection_label, LV_FIT_MAX);
	lv_label_set_text(connection_label, "Disconnected");

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
		if (pdTRUE == xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY)) {
			lv_task_handler();
			xSemaphoreGive(_gn_xGuiSemaphore);
		}
	}

	/* A task should NEVER return */
	free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
	free(buf2);
#endif
	vTaskDelete(NULL);
}

void _gn_init_display(gn_config_handle_t conf) {

	_gn_gui_event_group = xEventGroupCreate();

	ESP_LOGI(TAG, "_gn_init_display");

	xTaskCreatePinnedToCore(_gn_display_gui_task, "_gn_display_gui_task",
			4096 * 2, NULL, 10, NULL, 1);

	ESP_LOGI(TAG, "_gn_display_gui_task created");

	xEventGroupWaitBits(_gn_gui_event_group, GN_EVT_GROUP_GUI_COMPLETED_EVENT,
			pdTRUE, pdTRUE, portMAX_DELAY);


	ESP_ERROR_CHECK (esp_event_handler_instance_register_with(gn_event_loop, GN_BASE_EVENT,
			GN_DISPLAY_LOG_SYSTEM, _gn_display_log_system_handler, NULL, NULL));


	ESP_LOGI(TAG, "_gn_init_display done");

}

/* Signal Wi-Fi events on this event-group */
const int GN_WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t _gn_event_group_wifi;
int _gn_wifi_connect_retries = 0;

void _gn_wifi_event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) {

	ESP_LOGI(TAG, "_gn_wifi_event_handler");

	if (event_base == WIFI_PROV_EVENT) {
		switch (event_id) {
		case WIFI_PROV_START:
			gn_log_message("Provisioning Started");
			break;
		case WIFI_PROV_CRED_RECV: {
			wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t*) event_data;
			ESP_LOGI(TAG,
					"Received Wi-Fi credentials" "\n\tSSID     : %s\n\tPassword : %s",
					(const char*) wifi_sta_cfg->ssid,
					(const char*) wifi_sta_cfg->password);
			break;
		}
		case WIFI_PROV_CRED_FAIL: {
			wifi_prov_sta_fail_reason_t *reason =
					(wifi_prov_sta_fail_reason_t*) event_data;
			_gn_wifi_connect_retries++;
			if (_gn_wifi_connect_retries > 5) {
				nvs_flash_erase();
				esp_restart();
			}

			gn_log_message("Provisioning Failed");

			ESP_LOGE(TAG,
					"Provisioning failed!\n\tReason : %s" "\n\tRetrying %d of 5 times and then reset to factory",
					((*reason == WIFI_PROV_STA_AUTH_ERROR) ?
							"Wi-Fi station authentication failed" :
							"Wi-Fi access-point not found"),
					_gn_wifi_connect_retries);
			break;
		}
		case WIFI_PROV_CRED_SUCCESS:
			_gn_wifi_connect_retries = 0;
			gn_log_message("Provisioning OK");
			break;
		case WIFI_PROV_END:
			ESP_LOGI(TAG, "WIFI_PROV_END");
			/* De-initialize manager once provisioning is finished */
			wifi_prov_mgr_deinit();
			break;
		default:
			break;
		}
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
		esp_wifi_connect();
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		_gn_wifi_connect_retries = 0;
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		char *log = (char*) malloc(sizeof(char) * 20);
		sprintf(log, "IP: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
		gn_log_message(log);
		//ESP_LOGI(TAG, "IP : " IPSTR, IP2STR(&event->ip_info.ip));
		free(log);
		/* Signal main application to continue execution */
		xEventGroupSetBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT);
	} else if (event_base == WIFI_EVENT
			&& event_id == WIFI_EVENT_STA_DISCONNECTED) {

		_gn_wifi_connect_retries++;
		if (_gn_wifi_connect_retries > 5) {
			nvs_flash_erase();
			esp_restart();
		}

		ESP_LOGI(TAG,
				"Disconnected. Connecting to the AP again. Trying %d out of 5..",
				_gn_wifi_connect_retries);
		gn_log_message("Disconnected");
		esp_wifi_connect();
	}
}

void _gn_wifi_init_sta(void) {
	/* Start Wi-Fi in station mode */
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void _gn_wifi_get_device_service_name(char *service_name, size_t max) {
	uint8_t eth_mac[6];
	const char *ssid_prefix = "PROV_";
	esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
	snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
			eth_mac[4], eth_mac[5]);
}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
esp_err_t _gn_wifi_custom_prov_data_handler(uint32_t session_id,
		const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen,
		void *priv_data) {
	if (inbuf) {
		ESP_LOGI(TAG, "Received data: %.*s", inlen, (char*) inbuf);
	}
	char response[] = "SUCCESS";
	*outbuf = (uint8_t*) strdup(response);
	if (*outbuf == NULL) {
		ESP_LOGE(TAG, "System out of memory");
		return ESP_ERR_NO_MEM;
	}
	*outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

	return ESP_OK;
}

void _gn_init_wifi(gn_config_handle_t conf) {

	gn_log_message("Wifi Init");

	ESP_ERROR_CHECK(esp_netif_init());
	_gn_event_group_wifi = xEventGroupCreate();

	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
					&_gn_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(
			esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
					&_gn_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(
			esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
					&_gn_wifi_event_handler, NULL));

	esp_netif_create_default_wifi_sta();

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
	esp_netif_create_default_wifi_ap();
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	conf->wifi_config = cfg;

	/* Configuration for the provisioning manager */
	wifi_prov_mgr_config_t config = {
	/* What is the Provisioning Scheme that we want ?
	 * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
        .scheme = wifi_prov_scheme_ble,
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
			.scheme = wifi_prov_scheme_softap,
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */

			/* Any default scheme specific event handler that you would
			 * like to choose. Since our example application requires
			 * neither BT nor BLE, we can choose to release the associated
			 * memory once provisioning is complete, or not needed
			 * (in case when device is already provisioned). Choosing
			 * appropriate scheme specific event handler allows the manager
			 * to take care of this automatically. This can be set to
			 * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */
#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP
			.scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_SOFTAP */
			};

	ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

	conf->prov_config = config;

	bool provisioned = false;

#ifdef CONFIG_GROWNODE_RESET_PROVISIONED
    wifi_prov_mgr_reset_provisioning();
#endif

	/* Let's find out if the device is provisioned */
	ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

	/* If device is not yet provisioned start provisioning service */
	if (!provisioned) {
		ESP_LOGI(TAG, "Starting provisioning");

		/* What is the Device Service Name that we want
		 * This translates to :
		 *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
		 *     - device name when scheme is wifi_prov_scheme_ble
		 */
		char service_name[12];
		_gn_wifi_get_device_service_name(service_name, sizeof(service_name));

		ESP_LOGI(TAG, "service_name: %s", service_name);

		/* What is the security level that we want (0 or 1):
		 *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
		 *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
		 *          using X25519 key exchange and proof of possession (pop) and AES-CTR
		 *          for encryption/decryption of messages.
		 */
		wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

		/* Do we want a proof-of-possession (ignored if Security 0 is selected):
		 *      - this should be a string with length > 0
		 *      - NULL if not used
		 */
		const char *pop = "abcd1234";

		/* What is the service key (could be NULL)
		 * This translates to :
		 *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
		 *     - simply ignored when scheme is wifi_prov_scheme_ble
		 */
		const char *service_key = NULL;

#ifdef CONFIG_GROWNODE_PROV_TRANSPORT_BLE
        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
#endif /* CONFIG_GROWNODE_PROV_TRANSPORT_BLE */

		/* An optional endpoint that applications can create if they expect to
		 * get some additional custom data during provisioning workflow.
		 * The endpoint name can be anything of your choice.
		 * This call must be made before starting the provisioning.
		 */
		wifi_prov_mgr_endpoint_create("custom-data");

		/* Start provisioning service */
		ESP_ERROR_CHECK(
				wifi_prov_mgr_start_provisioning(security, pop, service_name,
						service_key));

		/* The handler for the optional endpoint created above.
		 * This call must be made after starting the provisioning, and only if the endpoint
		 * has already been created above.
		 */
		wifi_prov_mgr_endpoint_register("custom-data",
				_gn_wifi_custom_prov_data_handler, NULL);

		/* Uncomment the following to wait for the provisioning to finish and then release
		 * the resources of the manager. Since in this case de-initialization is triggered
		 * by the default event loop handler, we don't need to call the following */
		// wifi_prov_mgr_wait();
		// wifi_prov_mgr_deinit();
	} else {
		ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

		/* We don't need the manager as device is already provisioned,
		 * so let's release it's resources */
		wifi_prov_mgr_deinit();

		/* Start Wi-Fi station */
		_gn_wifi_init_sta();
	}

	ESP_LOGI(TAG, "Wait for Wi-Fi connection");
	gn_log_message("Connecting...");
	/* Wait for Wi-Fi connection */
	xEventGroupWaitBits(_gn_event_group_wifi, GN_WIFI_CONNECTED_EVENT, false,
			true, portMAX_DELAY);

}

static bool time_sync_init_done = false;

esp_err_t _gn_init_time_sync(gn_config_handle_t conf) {

	if (sntp_enabled()) {
		ESP_LOGI(TAG, "SNTP already initialized.");
		time_sync_init_done = true;
		return ESP_OK;
	}
	char *sntp_server_name = CONFIG_GROWNODE_SNTP_SERVER_NAME;
	ESP_LOGI(TAG, "Initializing SNTP. Using the SNTP server: %s",
			sntp_server_name);
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, sntp_server_name);
	sntp_init();
	gn_log_message("Time sync");
	time_sync_init_done = true;
	return ESP_OK;

}

gn_config_handle_t _gn_default_conf;

gn_config_handle_t _gn_create_config() {
	gn_config_handle_t _conf = (gn_config_handle_t) malloc(sizeof(gn_config_t));
	_conf->event_loop = NULL;
	_conf->mqtt_client = NULL;
	//_conf->prov_config = NULL;
	//_conf->spiffs_conf = NULL;
	//_conf->wifi_config = NULL;
	return _conf;
}

gn_config_handle_t gn_init() {

	if (initialized)
		return _gn_default_conf;

	_gn_default_conf = _gn_create_config();

	//init flash
	_gn_init_flash(_gn_default_conf);
	//init spiffs
	_gn_init_spiffs(_gn_default_conf);
	//init event loop
	_gn_init_event_loop(_gn_default_conf);
	//init display
	_gn_init_display(_gn_default_conf);
	//vTaskDelay(1000 / portTICK_PERIOD_MS);

	//init wifi
	_gn_init_wifi(_gn_default_conf);
	//init time sync
	_gn_init_time_sync(_gn_default_conf);
	//init mqtt system
	_gn_init_mqtt(_gn_default_conf);

	initialized = true;

	return _gn_default_conf;

}

#ifdef __cplusplus
}
#endif //__cplusplus
