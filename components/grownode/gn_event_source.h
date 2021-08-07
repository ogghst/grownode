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

#ifndef EVENT_SOURCE_H_
#define EVENT_SOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/event_groups.h"
 #include "freertos/semphr.h"
 */
#include "esp_event_base.h"
#include "esp_event.h"
//#include "esp_timer.h"

// Declare an event base
extern esp_event_base_t GN_BASE_EVENT; // @suppress("Type cannot be resolved")
extern esp_event_base_t GN_LEAF_EVENT; // @suppress("Type cannot be resolved")

#define GN_EVENT_ANY_ID       ESP_EVENT_ANY_ID

typedef enum {

	GN_LEAF_INIT_REQUEST_EVENT, //used internally to leaf event queue
	GN_LEAF_MESSAGE_RECEIVED_EVENT,
	GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT,
	GN_SYSTEM_MESSAGE_RECEIVED_EVENT,

	GN_LEAF_ADDED_EVENT,

	GN_DISPLAY_LOG_EVENT,
	GN_NETWORK_CONNECTED_EVENT,
	GN_NETWORK_DISCONNECTED_EVENT,
	GN_SERVER_CONNECTED_EVENT,
	GN_SERVER_DISCONNECTED_EVENT,

	GN_NET_MSG_RECV,

	GN_NET_OTA_START,
	GN_NET_RST_START,

	GN_KEEPALIVE_START_EVENT

} gn_event_id_t;

#ifdef __cplusplus
}
#endif

#endif // #ifndef EVENT_SOURCE_H_
