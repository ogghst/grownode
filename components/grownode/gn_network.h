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

#ifndef MAIN_GN_NETWORK_H_
#define MAIN_GN_NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gn_wifi_init(gn_config_handle_t conf);
void gn_ota_task(void *pvParameter);
esp_err_t gn_wifi_time_sync_init(gn_config_handle_t conf);

gn_err_t gn_wifi_stop(gn_config_handle_t conf);
gn_err_t gn_wifi_start(gn_config_handle_t conf);

int8_t gn_wifi_get_rssi();
void gn_wifi_get_mac(char *mac_string);
void gn_wifi_get_ip(char *ip_string);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MAIN_GN_NETWORK_H_ */
