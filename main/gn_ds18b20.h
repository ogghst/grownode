/*
 * gn_pump.h
 *
 *  Created on: 19 apr 2021
 *      Author: muratori.n
 */

#ifndef MAIN_GN_DS18B20_H_
#define MAIN_GN_DS18B20_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "grownode.h"
//#include "gn_mqtt_protocol.h"

void gn_ds18b20_loop(gn_leaf_config_handle_t leaf_config);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_DS18B20_H_ */
