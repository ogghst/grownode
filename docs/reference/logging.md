
#Logging

GrowNode uses [ESP-IDF logging library](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html) to log messages.

Every functionality has his own logging TAG, that can be used to specify log level at build or at runtime. eg:

```
	//set default log level
	esp_log_level_set("*", ESP_LOG_INFO);

	//set this file log level
	esp_log_level_set("gn_main", ESP_LOG_INFO);

	//core
	esp_log_level_set("grownode", ESP_LOG_INFO);
	esp_log_level_set("gn_commons", ESP_LOG_INFO);
	esp_log_level_set("gn_nvs", ESP_LOG_INFO);
	esp_log_level_set("gn_mqtt_protocol", ESP_LOG_DEBUG);
	esp_log_level_set("gn_network", ESP_LOG_INFO);
	esp_log_level_set("gn_display", ESP_LOG_INFO);

	//boards
	esp_log_level_set("gn_blink", ESP_LOG_INFO);

```

It is recommended in production code to set log level at appropriate value in the sdkconfig file.

Due to distributed nature of this framework, it is useful to send log messages across the network. This is done by calling the `gn_log()` function:

```
gn_err_t gn_log(char *log_tag, gn_log_level_t level, const char *message, ...);
```

This will mimic the `ESP_LOG()` macro, but will also send the message to the network if the log level is enabled for that TAG. the `gn_log_level_t` struct mimics the ESP log levels, so you can use in the same way:

```
	gn_log(TAG, GN_LOG_INFO, "easypot1 - measuring moisture: %f", moist_act);
```

Structure of the message is described in [MQTT](mqtt.md) section.