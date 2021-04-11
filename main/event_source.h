#ifndef EVENT_SOURCE_H_
#define EVENT_SOURCE_H_

#include "esp_event.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declare an event base
ESP_EVENT_DECLARE_BASE (MESSAGE_EVENT);

enum {
	LOG_SYSTEM
};

#ifdef __cplusplus
}
#endif

#endif // #ifndef EVENT_SOURCE_H_
