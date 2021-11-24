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

	//leaf management events
	GN_LEAF_INIT_REQUEST_EVENT = 0x001, //used internally to leaf event queue
	GN_LEAF_MESSAGE_RECEIVED_EVENT = 0x002,
	GN_LEAF_ADDED_EVENT = 0x003,

	//leaf parameter events
	//GN_LEAF_PARAM_CHANGE_REQUEST_NETWORK_EVENT = 0x101, //generated when a parameter change is received from network
	GN_LEAF_PARAM_CHANGED_EVENT = 0x102, //generated when a parameter is internally changed
	GN_LEAF_PARAM_INITIALIZED_EVENT = 0x103, //generated when a parameter is internally changed
	GN_LEAF_PARAM_CHANGE_REQUEST_EVENT = 0x104, //generated when a parameter is requested to change

	//system events
	GN_SYSTEM_MESSAGE_RECEIVED_EVENT = 0x201,

	//GUI events
	GN_GUI_LOG_EVENT = 0x301,

	//network events
	GN_NET_CONNECTED_EVENT = 0x401,
	GN_NET_DISCONNECTED_EVENT = 0x402,
	GN_NET_MSG_RECV = 0x403,
	GN_NET_OTA_START = 0x404,
	GN_NET_RST_START = 0x405,
	GN_NET_RBT_START = 0x406,

	//server events
	GN_SRV_CONNECTED_EVENT = 0x501,
	GN_SRV_DISCONNECTED_EVENT = 0x502,
	GN_SRV_KEEPALIVE_START_EVENT = 0x503,

	//node events
	GN_NODE_STARTED_EVENT = 0x601


} gn_event_id_t;

#ifdef __cplusplus
}
#endif

#endif // #ifndef EVENT_SOURCE_H_
