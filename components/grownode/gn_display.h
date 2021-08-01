/*
 * gn_display.h
 *
 *  Created on: 27 apr 2021
 *      Author: muratori.n
 */

#ifndef MAIN_GN_DISPLAY_H_
#define MAIN_GN_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"

esp_err_t gn_init_display(gn_config_handle_t conf);

void gn_display_leaf_start(gn_leaf_config_handle_t leaf_config);

void gn_display_setup_leaf_display(gn_leaf_config_handle_t leaf_config, gn_display_handler_t display_handler);

BaseType_t gn_display_leaf_refresh_start();

BaseType_t gn_display_leaf_refresh_end();

SemaphoreHandle_t _gn_xGuiSemaphore;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_DISPLAY_H_ */
