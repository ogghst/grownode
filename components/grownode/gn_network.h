/*
 * gn_network.h
 *
 *  Created on: 2 mag 2021
 *      Author: muratori.n
 */

#ifndef MAIN_GN_NETWORK_H_
#define MAIN_GN_NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t _gn_init_wifi(gn_config_handle_t conf);
void _gn_ota_task(void *pvParameter);
esp_err_t _gn_init_time_sync(gn_config_handle_t conf);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_NETWORK_H_ */
