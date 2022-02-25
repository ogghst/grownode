
# Sleep Modes

GrowNode implements Deep and Light sleep functionalities. These are useful to save energy when no active actions are needed and let the board be battery powered. 

In order to understand basic sleep mode functionalities, please refer to [ESP32 reference guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html)

## Configuration

Sleep management is configured in init parameters:

```
	gn_config_init_param_t config_init = {
		...
		.wakeup_time_millisec = 5000LL,
		.sleep_delay_millisec = 50LL,
		.sleep_time_millisec = 120000LL,
		.sleep_mode = GN_SLEEP_MODE_DEEP
	};

```

Main parameters are:
 - 	`gn_sleep_mode_t sleep_mode` - define if and how the board must sleep. possible values are GN_SLEEP_MODE_NONE (never sleeps), GN_SLEEP_MODE_LIGHT (light sleep), GN_SLEEP_MODE_DEEP (deep sleep)
 - 	`uint64_t wakeup_time_millisec` - if sleep mode is GN_SLEEP_MODE_LIGHT or GN_SLEEP_MODE_DEEP, sets for how long the board must stay on (counted from boot)
 - 	`uint64_t sleep_time_millisec` - if sleep mode is GN_SLEEP_MODE_LIGHT or GN_SLEEP_MODE_DEEP, sets for how long the board must sleep
 - 	`uint64_t sleep_delay_millisec` - if sleep mode is GN_SLEEP_MODE_LIGHT or GN_SLEEP_MODE_DEEP, sets for how long the board must stay on waiting for leaves to complete its job before sleeping

## Sleep start cycle

In light and deep sleep mode, a timer sets until when the board must stay on. Upon timeout, the board sends to its leaves a GN_NODE_LIGHT_SLEEP_START_EVENT o GN_NODE_DEEP_SLEEP_START_EVENT. This allows the leaves to perform housekeeping work in preparation to sleep. Then, waits for `sleep_delay_millisec`. After this time, its starts checking whether leaves are still working. Once all leaves are in blocking status if releases the MQTT and WIFI connection and start the sleep cycle for `sleep_time_millisec`.
