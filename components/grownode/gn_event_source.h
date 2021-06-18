#ifndef EVENT_SOURCE_H_
#define EVENT_SOURCE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_event_base.h"
#include "esp_event.h"
#include "esp_timer.h"



// Declare an event base
ESP_EVENT_DECLARE_BASE (GN_BASE_EVENT); // @suppress("Type cannot be resolved")
ESP_EVENT_DECLARE_BASE (GN_LEAF_EVENT); // @suppress("Type cannot be resolved")

#define GN_EVENT_ANY_ID       ESP_EVENT_ANY_ID

typedef enum  {

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
