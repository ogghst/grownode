#ifndef EVENT_SOURCE_H_
#define EVENT_SOURCE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "esp_timer.h"



// Declare an event base
ESP_EVENT_DECLARE_BASE (GN_BASE_EVENT);

typedef enum  {
	GN_LEAF_ADDED,

	GN_DISPLAY_LOG_SYSTEM,

	GN_NET_CONNECTED,
	GN_NET_DISCONNECTED,
	GN_NET_MSG_RECV,

} gn_event_id;

#ifdef __cplusplus
}
#endif

#endif // #ifndef EVENT_SOURCE_H_
