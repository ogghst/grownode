// Copyright 2021 Nicola Muratori (nicola.muratori@gmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "gn_display.h"
#include "gn_commons.h"
#include "grownode_intl.h"
#include "gn_event_source.h"

#define LV_TICK_PERIOD_MS 1

#define TAG "gn_display"

gn_config_handle_intl_t _config;

//event groups
const int GN_EVT_GROUP_GUI_COMPLETED_EVENT = BIT0;
EventGroupHandle_t _gn_gui_event_group;

//TODO revise log structure with structs

#define LOG_MESSAGES 14

#define GN_DISPLAY_HOR_RES 240
#define GN_DISPLAY_VER_RES 320

lv_obj_t *statusLabel[LOG_MESSAGES];
char rawMessages[LOG_MESSAGES][30];
size_t rawIdx = 0;
gn_leaf_config_handle_intl_t _current_leaf = NULL;

//lv_obj_t *network_status_label, *server_status_label;
//lv_obj_t *network_led, *server_led;

bool _initialized = false;

////////////////////////////////////////////////////////////////////

static lv_style_t gn_style_base, gn_style_leaf_panel,
		gn_style_leaf_panel_nav_buttons, gn_style_leaf, gn_style_bottom,
		gn_style_bottom_element, gn_style_action_button_element;
static lv_obj_t *grownode_scr, *main_scr;
static lv_obj_t *status_bar, *bottom_panel, *log_panel, *action_panel,
		*leaf_panel;

static lv_obj_t *net_label, *srv_label;

static void scroll_left_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "scroll_left_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	if (!_config || !_config->node_config || !_current_leaf
			|| _config->node_config->leaves.last < 2)
		return;

	ESP_LOGD(TAG, "current leaf: %s", _current_leaf->name);

	size_t found = 0;

	for (size_t i = 0; i < _config->node_config->leaves.last; i++) {

		gn_leaf_config_handle_intl_t _p =
				(gn_leaf_config_handle_intl_t) _config->node_config->leaves.at[i];
		ESP_LOGD(TAG, "checking %s", _p->name);
		if (strcmp(_p->name, _current_leaf->name) == 0) {
			ESP_LOGD(TAG, "found leaf at %d", i);
			found = 1;
			lv_obj_add_flag(_current_leaf->display_container,
					LV_OBJ_FLAG_HIDDEN);
			//show the previous panel
			if (i == 0)
				_current_leaf =
						_config->node_config->leaves.at[_config->node_config->leaves.last
								- 1];
			else if (i > 0)
				_current_leaf = _config->node_config->leaves.at[i - 1];

			ESP_LOGD(TAG, "new leaf %s", _current_leaf->name);

			break;
		}

	}
	//make it visible if found
	if (found == 1 && _current_leaf->display_container != NULL)
		lv_obj_clear_flag(_current_leaf->display_container, LV_OBJ_FLAG_HIDDEN);

}

static void scroll_right_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "scroll_right_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	if (!_config || !_config->node_config || !_current_leaf
			|| _config->node_config->leaves.last < 2)
		return;

	ESP_LOGD(TAG, "current leaf: %s", _current_leaf->name);

	size_t found = 0;

	for (size_t i = 0; i < _config->node_config->leaves.last; i++) {

		gn_leaf_config_handle_intl_t _p =
				(gn_leaf_config_handle_intl_t) _config->node_config->leaves.at[i];
		ESP_LOGD(TAG, "checking %s", _p->name);
		if (strcmp(_p->name, _current_leaf->name) == 0) {
			ESP_LOGD(TAG, "found leaf at %d", i);
			found = 1;
			lv_obj_add_flag(_current_leaf->display_container,
					LV_OBJ_FLAG_HIDDEN);
			//show the next panel
			if (i == _config->node_config->leaves.last - 1)
				_current_leaf = _config->node_config->leaves.at[0];
			else if (i < _config->node_config->leaves.last)
				_current_leaf = _config->node_config->leaves.at[i + 1];

			ESP_LOGD(TAG, "new leaf %s", _current_leaf->name);

			break;
		}

	}
	//make it visible if found
	if (found == 1 && _current_leaf->display_container != NULL)
		lv_obj_clear_flag(_current_leaf->display_container, LV_OBJ_FLAG_HIDDEN);

}

static void log_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "log_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	//lv_obj_t *button = lv_event_get_target(e);
	lv_obj_add_flag(leaf_panel, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(action_panel, LV_OBJ_FLAG_HIDDEN);

	lv_obj_clear_flag(log_panel, LV_OBJ_FLAG_HIDDEN);

}

static void leaf_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "leaf_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	//lv_obj_t *button = lv_event_get_target(e);
	lv_obj_add_flag(log_panel, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(action_panel, LV_OBJ_FLAG_HIDDEN);

	lv_obj_clear_flag(leaf_panel, LV_OBJ_FLAG_HIDDEN);

}

static void action_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "action_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	//lv_obj_t *button = lv_event_get_target(e);
	lv_obj_add_flag(leaf_panel, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(log_panel, LV_OBJ_FLAG_HIDDEN);

	lv_obj_clear_flag(action_panel, LV_OBJ_FLAG_HIDDEN);

}

static void ota_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "ota_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	gn_firmware_update();

}

static void reset_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "reset_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	gn_reset();

}

static void reboot_button_event_handler(lv_event_t *e) {

	ESP_LOGD(TAG, "reboot_button_event_handler");

	if (e->code != LV_EVENT_CLICKED)
		return;

	gn_reboot();

}

void style_init() {
	lv_style_init(&gn_style_base);
	lv_style_set_text_color(&gn_style_base, lv_color_white());
	lv_style_set_bg_color(&gn_style_base, lv_color_black());
	lv_style_set_border_width(&gn_style_base, 0);
	lv_style_set_border_color(&gn_style_base, lv_palette_main(LV_PALETTE_NONE));
	lv_style_set_pad_all(&gn_style_base, 0);
	lv_style_set_radius(&gn_style_base, 0);

	lv_style_init(&gn_style_leaf_panel);
	lv_style_set_bg_color(&gn_style_leaf_panel,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_align(&gn_style_leaf_panel, LV_ALIGN_CENTER);
	lv_style_set_pad_all(&gn_style_leaf_panel, 5);
	lv_style_set_bg_opa(&gn_style_leaf_panel, LV_OPA_COVER);
	lv_style_set_bg_color(&gn_style_leaf_panel,
			lv_palette_darken(LV_PALETTE_BLUE_GREY, 5));
	lv_style_set_bg_grad_color(&gn_style_leaf_panel,
			lv_palette_darken(LV_PALETTE_BLUE_GREY, 2));
	lv_style_set_bg_grad_dir(&gn_style_leaf_panel, LV_GRAD_DIR_VER);
	lv_style_set_bg_main_stop(&gn_style_leaf_panel, 00);
	lv_style_set_bg_grad_stop(&gn_style_leaf_panel, 255);

	lv_style_init(&gn_style_leaf_panel_nav_buttons);
	lv_style_set_bg_opa(&gn_style_leaf_panel_nav_buttons, LV_OPA_0);
	lv_style_set_border_color(&gn_style_leaf_panel_nav_buttons,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_border_width(&gn_style_leaf_panel_nav_buttons, 1);
	lv_style_set_shadow_width(&gn_style_leaf_panel_nav_buttons, 0);
	lv_style_set_radius(&gn_style_leaf_panel_nav_buttons, 30);

	lv_style_init(&gn_style_leaf);
	lv_style_set_bg_color(&gn_style_leaf,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_border_width(&gn_style_leaf, 1);
	lv_style_set_radius(&gn_style_leaf, 10);
	lv_style_set_border_color(&gn_style_leaf, lv_palette_main(LV_PALETTE_BLUE));
	lv_style_set_height(&gn_style_leaf, lv_pct(90));
	lv_style_set_width(&gn_style_leaf, lv_pct(90));
	lv_style_set_shadow_width(&gn_style_leaf, 15);
	lv_style_set_shadow_color(&gn_style_leaf,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_shadow_ofs_x(&gn_style_leaf, 5);
	lv_style_set_shadow_ofs_y(&gn_style_leaf, 5);

	lv_style_init(&gn_style_bottom);
	lv_style_set_bg_color(&gn_style_bottom,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_border_width(&gn_style_bottom, 1);
	lv_style_set_border_color(&gn_style_bottom,
			lv_palette_main(LV_PALETTE_BLUE));
	lv_style_set_pad_all(&gn_style_bottom, 5);
	lv_style_set_radius(&gn_style_bottom, 0);
	lv_style_set_align(&gn_style_bottom, LV_ALIGN_CENTER);

	lv_style_init(&gn_style_bottom_element);
	lv_style_set_bg_color(&gn_style_bottom_element,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_border_width(&gn_style_bottom_element, 1);
	lv_style_set_border_color(&gn_style_bottom_element,
			lv_palette_main(LV_PALETTE_CYAN));
	lv_style_set_pad_all(&gn_style_bottom_element, 5);
	lv_style_set_radius(&gn_style_bottom_element, 10);
//lv_style_set_height(&gn_style_bottom_element,lv_pct(100));
//lv_style_set_width(&gn_style_bottom_element, lv_pct(30));
	lv_style_set_align(&gn_style_bottom_element, LV_ALIGN_CENTER);

	lv_style_init(&gn_style_action_button_element);
	lv_style_set_bg_color(&gn_style_action_button_element,
			lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_border_width(&gn_style_action_button_element, 1);
	lv_style_set_border_color(&gn_style_action_button_element,
			lv_palette_main(LV_PALETTE_CYAN));
	lv_style_set_pad_all(&gn_style_action_button_element, 5);
	lv_style_set_radius(&gn_style_action_button_element, 10);
	//lv_style_set_height(&gn_style_bottom_element,lv_pct(100));
	//lv_style_set_width(&gn_style_bottom_element, lv_pct(30));
	lv_style_set_align(&gn_style_action_button_element, LV_ALIGN_CENTER);

}

void grownode_scr_init(lv_obj_t *scr) {

	lv_obj_t *cont = lv_obj_create(scr);
	lv_obj_set_size(cont, GN_DISPLAY_HOR_RES, GN_DISPLAY_VER_RES);
	lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_style(cont, &gn_style_base, 0);

	/*Create the main label*/
	lv_obj_t *main_label = lv_label_create(cont);
	lv_label_set_text(main_label, "GrowNode");

	/*Position the main label*/
	lv_obj_align(main_label, LV_ALIGN_CENTER, 0, 0);

}

lv_obj_t* build_status_bar(lv_obj_t *parent, lv_obj_t *align_to) {

//status bar
	lv_obj_t *status_bar = lv_obj_create(parent);
	lv_obj_set_size(status_bar, GN_DISPLAY_HOR_RES, GN_DISPLAY_VER_RES * 0.05);
	lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_EVENLY,
			LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START);
	lv_obj_add_style(status_bar, &gn_style_base, 0);

//name label
	lv_obj_t *name_label = lv_label_create(status_bar);
	lv_label_set_text(name_label, "-");
//lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_add_style(name_label, &gn_style_base, 0);

//network label
	net_label = lv_label_create(status_bar);
	lv_label_set_text(net_label, "-");
//lv_obj_align_to(net_label, status_bar, LV_ALIGN_RIGHT_MID, 40, 0);
	lv_obj_add_style(net_label, &gn_style_base, 0);

//server label
	srv_label = lv_label_create(status_bar);
	lv_label_set_text(srv_label, "-");
//lv_obj_align_to(srv_label, status_bar, LV_ALIGN_LEFT_MID, 10, 0);
	lv_obj_add_style(srv_label, &gn_style_base, 0);

	return status_bar;

}

lv_obj_t* build_leaf_panel(lv_obj_t *parent, lv_obj_t *align_to) {

//leaf panel
	ESP_LOGD(TAG, "build_leaf_panel - lv_obj_create");
	lv_obj_t *leaf_panel = lv_obj_create(parent);
	lv_obj_align_to(leaf_panel, align_to, LV_ALIGN_TOP_LEFT, 0,
	GN_DISPLAY_VER_RES * 0.05);
	lv_obj_add_style(leaf_panel, &gn_style_base, 0);
	lv_obj_add_style(leaf_panel, &gn_style_leaf_panel, 0);
	lv_obj_set_height(leaf_panel, lv_pct(80));
	lv_obj_set_width(leaf_panel, GN_DISPLAY_HOR_RES);

	ESP_LOGD(TAG, "build_leaf_panel - scroll_left_button");
	lv_obj_t *scroll_left_button = lv_btn_create(parent);
	lv_obj_add_event_cb(scroll_left_button, scroll_left_button_event_handler,
			LV_EVENT_CLICKED, NULL);
	lv_obj_add_style(scroll_left_button, &gn_style_base, 0);
	lv_obj_add_style(scroll_left_button, &gn_style_leaf_panel, 0);
	lv_obj_add_style(scroll_left_button, &gn_style_leaf_panel_nav_buttons, 0);
	lv_obj_set_height(scroll_left_button, lv_pct(20));
	lv_obj_align(scroll_left_button, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_t *scroll_left_button_label = lv_label_create(scroll_left_button);
	lv_label_set_text(scroll_left_button_label, "<");
	lv_obj_center(scroll_left_button_label);

	ESP_LOGD(TAG, "build_leaf_panel - scroll_right_button");
	lv_obj_t *scroll_right_button = lv_btn_create(parent);
	lv_obj_add_event_cb(scroll_right_button, scroll_right_button_event_handler,
			LV_EVENT_CLICKED, NULL);
	lv_obj_add_style(scroll_right_button, &gn_style_base, 0);
	lv_obj_add_style(scroll_right_button, &gn_style_leaf_panel, 0);
	lv_obj_add_style(scroll_right_button, &gn_style_leaf_panel_nav_buttons, 0);
	lv_obj_set_height(scroll_right_button, lv_pct(20));
	lv_obj_align(scroll_right_button, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_t *scroll_right_button_label = lv_label_create(scroll_right_button);
	lv_label_set_text(scroll_right_button_label, ">");
	lv_obj_center(scroll_right_button_label);

	ESP_LOGD(TAG, "build_leaf_panel - end");

	return leaf_panel;

}

lv_obj_t* build_log_panel(lv_obj_t *parent, lv_obj_t *align_to) {

	//log panel
	lv_obj_t *log_panel = lv_obj_create(parent);
	lv_obj_align_to(log_panel, align_to, LV_ALIGN_TOP_LEFT, 0, lv_pct(5));
	lv_obj_add_style(log_panel, &gn_style_base, 0);
	lv_obj_add_style(log_panel, &gn_style_leaf_panel, 0);
	lv_obj_set_height(log_panel, lv_pct(80));
	lv_obj_set_width(log_panel, GN_DISPLAY_HOR_RES);

	for (int row = 0; row < LOG_MESSAGES; row++) {

		//log labels
		statusLabel[row] = lv_label_create(log_panel);
		lv_obj_add_style(statusLabel[row], &gn_style_base, 0);
		lv_label_set_text(statusLabel[row], "");

		if (row == 0) {
			lv_obj_align_to(statusLabel[row], log_panel, LV_ALIGN_TOP_LEFT, 0,
					0);

		} else {
			lv_obj_align_to(statusLabel[row], statusLabel[row - 1],
					LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
		}

	}

	return log_panel;

}

lv_obj_t* build_action_panel(lv_obj_t *parent, lv_obj_t *align_to) {

	lv_obj_t *action_panel = lv_obj_create(parent);
	lv_obj_align_to(action_panel, align_to, LV_ALIGN_TOP_LEFT, 0, lv_pct(5));
	lv_obj_add_style(action_panel, &gn_style_base, 0);
	lv_obj_set_height(action_panel, lv_pct(80));
	lv_obj_set_width(action_panel, GN_DISPLAY_HOR_RES);

	lv_obj_t *ota_button = lv_obj_create(action_panel);
	lv_obj_add_style(ota_button, &gn_style_base, 0);
	lv_obj_add_style(ota_button, &gn_style_action_button_element, 0);
	lv_obj_add_event_cb(ota_button, ota_button_event_handler, LV_EVENT_CLICKED,
	NULL);
	lv_obj_align_to(ota_button, action_panel, LV_ALIGN_TOP_LEFT, 5, 15);
	lv_obj_set_size(ota_button, LV_PCT(90), LV_PCT(11));

	lv_obj_t *ota_button_label = lv_label_create(ota_button);
	lv_label_set_text(ota_button_label, "Over The Air Update");
	lv_obj_center(ota_button_label);

	lv_obj_t *reboot_button = lv_obj_create(action_panel);
	lv_obj_add_style(reboot_button, &gn_style_base, 0);
	lv_obj_add_style(reboot_button, &gn_style_action_button_element, 0);
	lv_obj_add_event_cb(reboot_button, reboot_button_event_handler,
			LV_EVENT_CLICKED, NULL);
	lv_obj_align_to(reboot_button, ota_button, LV_ALIGN_TOP_LEFT, 0, 35);
	lv_obj_set_size(reboot_button, LV_PCT(90), LV_PCT(11));

	lv_obj_t *reboot_button_label = lv_label_create(reboot_button);
	lv_label_set_text(reboot_button_label, "Reboot");
	lv_obj_center(reboot_button_label);

	lv_obj_t *reset_button = lv_obj_create(action_panel);
	lv_obj_add_style(reset_button, &gn_style_base, 0);
	lv_obj_add_style(reset_button, &gn_style_action_button_element, 0);
	lv_obj_add_event_cb(reset_button, reset_button_event_handler,
			LV_EVENT_CLICKED, NULL);
	lv_obj_align_to(reset_button, reboot_button, LV_ALIGN_TOP_LEFT, 0, 35);
	lv_obj_set_size(reset_button, LV_PCT(90), LV_PCT(11));

	lv_obj_t *reset_button_label = lv_label_create(reset_button);
	lv_label_set_text(reset_button_label, "Reset Configuration");
	lv_obj_center(reset_button_label);

	return action_panel;

}

lv_obj_t* build_bottom_panel(lv_obj_t *parent, lv_obj_t *align_to) {
//bottom scrolling panel
	lv_obj_t *bottom_panel = lv_obj_create(parent);
	lv_obj_set_size(bottom_panel, GN_DISPLAY_HOR_RES,
	GN_DISPLAY_VER_RES * 0.15);
	lv_obj_align_to(bottom_panel, align_to, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	lv_obj_add_style(bottom_panel, &gn_style_base, 0);
	lv_obj_add_style(bottom_panel, &gn_style_bottom, 0);
	lv_obj_set_flex_flow(bottom_panel, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bottom_panel, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	lv_obj_t *leaf_button = lv_obj_create(bottom_panel);
	lv_obj_add_style(leaf_button, &gn_style_base, 0);
	lv_obj_add_style(leaf_button, &gn_style_bottom, 0);
	lv_obj_add_style(leaf_button, &gn_style_bottom_element, 0);
	lv_obj_add_event_cb(leaf_button, leaf_button_event_handler,
			LV_EVENT_CLICKED,
			NULL);
	lv_obj_set_size(leaf_button, LV_PCT(30), LV_PCT(90));

	lv_obj_t *leaf_button_label = lv_label_create(leaf_button);
	lv_label_set_text(leaf_button_label, "LEAVES");
	lv_obj_center(leaf_button_label);
	lv_obj_add_event_cb(leaf_button_label, leaf_button_event_handler,
			LV_EVENT_CLICKED,
			NULL);

	lv_obj_t *log_button = lv_obj_create(bottom_panel);
	lv_obj_add_style(log_button, &gn_style_base, 0);
	lv_obj_add_style(log_button, &gn_style_bottom, 0);
	lv_obj_add_style(log_button, &gn_style_bottom_element, 0);
	lv_obj_add_event_cb(log_button, log_button_event_handler, LV_EVENT_CLICKED,
	NULL);
	lv_obj_set_size(log_button, LV_PCT(30), LV_PCT(90));

	lv_obj_t *log_button_label = lv_label_create(log_button);
	lv_label_set_text(log_button_label, "LOG");
	lv_obj_center(log_button_label);

	lv_obj_t *action_button = lv_obj_create(bottom_panel);
	lv_obj_add_style(action_button, &gn_style_base, 0);
	lv_obj_add_style(action_button, &gn_style_bottom, 0);
	lv_obj_add_style(action_button, &gn_style_bottom_element, 0);
	lv_obj_add_event_cb(action_button, action_button_event_handler,
			LV_EVENT_CLICKED, NULL);
	lv_obj_set_size(action_button, LV_PCT(30), LV_PCT(90));

	lv_obj_t *action_button_label = lv_label_create(action_button);
	lv_label_set_text(action_button_label, "SETTINGS");
	lv_obj_center(action_button_label);

	return bottom_panel;
}

lv_obj_t* main_scr_init(lv_obj_t *scr) {

	lv_obj_t *cont = lv_obj_create(scr);
	lv_obj_set_size(cont, GN_DISPLAY_HOR_RES, GN_DISPLAY_VER_RES);
	lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_style(cont, &gn_style_base, 0);
	lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

//lv_obj_align_to(log_button, bottom_panel, LV_ALIGN_CENTER, 0, 0);

	return cont;

}

void init_grownode_elements() {

//sample leaf
	lv_obj_t *sample_leaf = lv_obj_create(leaf_panel);
	lv_obj_add_style(sample_leaf, &gn_style_base, 0);
	lv_obj_add_style(sample_leaf, &gn_style_leaf_panel, 0);
	lv_obj_add_style(sample_leaf, &gn_style_leaf, 0);

	lv_obj_t *sample_leaf_temp_title = lv_label_create(sample_leaf);
	lv_label_set_text(sample_leaf_temp_title, "Temp");
	lv_obj_align_to(sample_leaf_temp_title, sample_leaf, LV_ALIGN_TOP_LEFT, 0,
			LV_PCT(10));

	lv_obj_t *sample_leaf_temp_value = lv_label_create(sample_leaf);
	lv_label_set_text(sample_leaf_temp_value, "20 Celsius");
	lv_obj_align_to(sample_leaf_temp_value, sample_leaf_temp_title,
			LV_ALIGN_TOP_LEFT, 0, LV_PCT(10));

}

void main_panel_open(void *arg) {

	if (pdTRUE == gn_display_leaf_refresh_start()) {
		ESP_LOGD(TAG, "Opening Main Screen");
		lv_scr_load(main_scr);
		gn_display_leaf_refresh_end();
	}
}

/////////////////////////////////////////////////////////////////////////

void _gn_display_lv_tick_task(void *arg) {
	(void) arg;
	lv_tick_inc(LV_TICK_PERIOD_MS);
}

/*
 void _gn_display_btn_ota_event_handler(lv_obj_t *obj, lv_event_t event) {
 if (event == LV_EVENT_CLICKED) {
 ESP_LOGD(TAG, "_gn_display_btn_ota_event_handler - clicked");

 ESP_ERROR_CHECK(
 esp_event_post_to(gn_get_config_event_loop(_config), GN_BASE_EVENT, GN_NET_OTA_START, NULL, 0, portMAX_DELAY));

 }
 }

 void _gn_display_btn_rst_event_handler(lv_obj_t *obj, lv_event_t event) {
 if (event == LV_EVENT_CLICKED) {

 ESP_LOGD(TAG, "_gn_display_btn_rst_event_handler - clicked");

 ESP_ERROR_CHECK(
 esp_event_post_to(gn_get_config_event_loop(_config), GN_BASE_EVENT, GN_NET_RST_START, NULL, 0, portMAX_DELAY));

 ESP_LOGD(TAG, "_gn_display_btn_rst_event_handler - sent event");

 }
 }
 */
void _gn_display_log_system_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		char *message = (char*) event_data;

		//lv_label_set_text(statusLabel, message.c_str());
		//const char *t = text.c_str();

		if (rawIdx > LOG_MESSAGES - 1) {

			//scroll messages
			for (int row = 1; row < LOG_MESSAGES; row++) {
				//ESP_LOGI(TAG, "scrolling %i from %s to %s", row, rawMessages[row], rawMessages[row - 1]);
				strncpy(rawMessages[row - 1], rawMessages[row], 30);
			}
			strncpy(rawMessages[LOG_MESSAGES - 1], message, 30);
		} else {
			//ESP_LOGI(TAG, "setting %i", rawIdx);
			strncpy(rawMessages[rawIdx], message, 30);
			rawIdx++;
		}

		//ESP_LOGI(TAG, "printing %s", message);
		//print
		for (int row = 0; row < LOG_MESSAGES; row++) {
			//ESP_LOGI(TAG, "label %i to %s", 9 - row, rawMessages[row]);
			lv_label_set_text(statusLabel[row], rawMessages[row]);
		}

		gn_display_leaf_refresh_end();
	}

}

void _gn_display_net_mqtt_handler(void *handler_args, esp_event_base_t base,
		int32_t id, void *event_data) {

//TODO pass wifi info to the event
//char *message = (char*) event_data;

	if (pdTRUE == gn_display_leaf_refresh_start()) {

		switch (id) {
		case GN_NET_CONNECTED_EVENT:
			lv_label_set_text(net_label, "NETOK");
			break;
		case GN_NET_DISCONNECTED_EVENT:
			lv_label_set_text(net_label, "NETKO");
			break;
		case GN_SRV_CONNECTED_EVENT:
			lv_label_set_text(srv_label, "SRVOK");
			break;
		case GN_SRV_DISCONNECTED_EVENT:
			lv_label_set_text(srv_label, "SRVKO");
			break;
		}

		gn_display_leaf_refresh_end();
	}

}

/**
 * 	@brief start of code guard to ensure the block will be executed outside display refresh task
 *
 * 	this is implemented with xSemaphoreTake
 *
 * 	@return result of the xSemaphoreTake function
 */
BaseType_t gn_display_leaf_refresh_start() {
	ESP_LOGD(TAG, "gn_display_leaf_refresh_start");
	return xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY);
}

/**
 *	@brief end of code guard to ensure the block will be executed outside display refresh task
 *
 * this is implemented with xSemaphoreGive
 *
 *	@return result of the xSemaphoreTake function
 */
BaseType_t gn_display_leaf_refresh_end() {
	ESP_LOGD(TAG, "gn_display_leaf_refresh_end");
	return xSemaphoreGive(_gn_xGuiSemaphore);
}

/*
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

 }
 */

/**
 * @brief prepare a container where to draw the display components of the leaf
 *
 * this has to be called into the display refresh task
 * @see gn_display_leaf_refresh_start()
 *
 * @return a pointer to lv_obj_t (to be casted)
 */
gn_display_container_t gn_display_setup_leaf_display(
		gn_leaf_config_handle_t leaf_config) {

#ifdef CONFIG_GROWNODE_DISPLAY_ENABLED

	ESP_LOGD(TAG, "gn_display_setup_leaf_display - %s",
			((gn_leaf_config_handle_intl_t )leaf_config)->name);

	gn_leaf_config_handle_intl_t _leaf_config =
			(gn_leaf_config_handle_intl_t) leaf_config;

	if (_leaf_config->display_container != NULL)
		return _leaf_config->display_container;

	lv_obj_t *_a_leaf_cont = NULL;

	//create a leaf container
	_a_leaf_cont = lv_obj_create(leaf_panel);
	lv_obj_add_style(_a_leaf_cont, &gn_style_base, 0);
	lv_obj_add_style(_a_leaf_cont, &gn_style_leaf_panel, 0);
	lv_obj_add_style(_a_leaf_cont, &gn_style_leaf, 0);
	lv_obj_align_to(_a_leaf_cont, leaf_panel, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_flex_flow(_a_leaf_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(_a_leaf_cont, LV_FLEX_ALIGN_START,
			LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	_leaf_config->display_container = _a_leaf_cont;
	if (_current_leaf && strcmp(_current_leaf->name, _leaf_config->name) != 0) {
		//hide the previous panel
		lv_obj_add_flag(_current_leaf->display_container, LV_OBJ_FLAG_HIDDEN);
	}
	_current_leaf = _leaf_config;

	return _a_leaf_cont;
#else
return NULL;
#endif
}

/*
 static void lv_tick_task(void *arg) {
 (void) arg;

 lv_tick_inc(LV_TICK_PERIOD_MS);
 }
 */

void _gn_display_gui_task(void *pvParameter) {

	(void) pvParameter;
	_gn_xGuiSemaphore = xSemaphoreCreateMutex();

	lv_init();

	lvgl_driver_init();

	 lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t),
	MALLOC_CAP_DMA);
	assert(buf1 != NULL);

	 lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t),
			 MALLOC_CAP_DMA);
	assert(buf2 != NULL);

	static lv_disp_draw_buf_t disp_buf;

	uint32_t size_in_px = DISP_BUF_SIZE;

	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;
	disp_drv.hor_res = 240;
	disp_drv.ver_res = 320;

	disp_drv.draw_buf = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);

	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	const esp_timer_create_args_t periodic_timer_args = { .callback =
			&_gn_display_lv_tick_task, .name = "_gn_display_lv_tick_task" };
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(
			esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

	//if (pdTRUE == gn_display_leaf_refresh_start()) {
		//_gn_display_create_gui();

///////////////////////////////////////////////////////////////

		ESP_LOGD(TAG, "style init");
		style_init();

		ESP_LOGD(TAG, "create screen");
		grownode_scr = lv_obj_create(NULL);
		main_scr = lv_obj_create(NULL);

		ESP_LOGD(TAG, "init screen");
		grownode_scr_init(grownode_scr);
		lv_obj_t *cont = main_scr_init(main_scr);

		ESP_LOGD(TAG, "building status bar");
		status_bar = build_status_bar(cont, cont);
		ESP_LOGD(TAG, "building bottom panel");
		bottom_panel = build_bottom_panel(cont, cont);
		ESP_LOGD(TAG, "building leaf panel");
		leaf_panel = build_leaf_panel(cont, status_bar);
		//lv_obj_add_flag(leaf_panel, LV_OBJ_FLAG_HIDDEN);
		ESP_LOGD(TAG, "building action panel");
		action_panel = build_action_panel(cont, status_bar);
		lv_obj_add_flag(action_panel, LV_OBJ_FLAG_HIDDEN);
		ESP_LOGD(TAG, "building log panel");
		log_panel = build_log_panel(cont, status_bar);
		lv_obj_add_flag(log_panel, LV_OBJ_FLAG_HIDDEN);

		ESP_LOGD(TAG, "load screen");
		//init_grownode_elements();

		lv_scr_load(grownode_scr);

///////////////////////////////////////////////////////////////
		//gn_display_leaf_refresh_end();

		const esp_timer_create_args_t main_panel_timer_args = { .callback =
				&main_panel_open, .name = "_gn_display_main_panel_open" };
		esp_timer_handle_t main_panel_timer;
		ESP_ERROR_CHECK(
				esp_timer_create(&main_panel_timer_args, &main_panel_timer));
		ESP_ERROR_CHECK(esp_timer_start_once(main_panel_timer, 4000 * 1000));

	//}

	xEventGroupSetBits(_gn_gui_event_group, GN_EVT_GROUP_GUI_COMPLETED_EVENT);

	while (1) {
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(10));

		/* Try to take the semaphore, call lvgl related function on success */
		if (pdTRUE == xSemaphoreTake(_gn_xGuiSemaphore, portMAX_DELAY)) {
			//ESP_LOGE(TAG, "LVGL handle");
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

/**
 * starts the display resources:
 * - initialize all objects
 * - listen to events in order to
 *
 * this should be called only into the node config init.
 * a second inizialization request has no effects. *
 *
 * @param config the node configuration
 *
 * @return the status of the initialization. ESP_OK if everything went well.
 *
 */
esp_err_t gn_init_display(gn_config_handle_t config) {

	esp_err_t ret = ESP_OK;

	if (_initialized)
		return ret;

//TODO initialization guards

#ifndef CONFIG_GROWNODE_DISPLAY_ENABLED
	return ESP_OK;
#endif

//TODO dangerous, better update through events
	_config = (gn_config_handle_intl_t) config;

	_gn_gui_event_group = xEventGroupCreate();

	ESP_LOGI(TAG, "gn_init_display");

	xTaskCreatePinnedToCore(_gn_display_gui_task, "_gn_display_gui_task",
			4096 * 2,
			NULL, 1, NULL, 1);

	ESP_LOGD(TAG, "_gn_display_gui_task created");

	xEventGroupWaitBits(_gn_gui_event_group, GN_EVT_GROUP_GUI_COMPLETED_EVENT,
	pdTRUE, pdTRUE, portMAX_DELAY);

//add event handlers
	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(_config->event_loop, GN_BASE_EVENT, GN_LOG_EVENT, _gn_display_log_system_handler, NULL, NULL));

	ESP_ERROR_CHECK(
			esp_event_handler_instance_register_with(_config->event_loop, GN_BASE_EVENT, GN_EVENT_ANY_ID, _gn_display_net_mqtt_handler, NULL, NULL));

	ESP_LOGD(TAG, "gn_init_display done");

	_initialized = true;

	return ESP_OK;

}

#ifdef __cplusplus
}
#endif //__cplusplus
