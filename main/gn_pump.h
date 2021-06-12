/*
 * gn_pump.h
 *
 *  Created on: 19 apr 2021
 *      Author: muratori.n
 */

#ifndef MAIN_GN_PUMP_H_
#define MAIN_GN_PUMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"
//#include "gn_mqtt_protocol.h"

void gn_pump_loop(gn_leaf_config_handle_t leaf_config);

void gn_pump_display_config(gn_leaf_config_handle_t leaf_config);

void gn_pump_display_task(gn_leaf_config_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_PUMP_H_ */
