---
hide:
  - navigation
---  

# Node

The core element of a GrowNode implementation is the Node. It represents the container and the entry point for the board capabilities.

In order to properly create a Node, a configuration shall be supplied. This is done by creating a `gn_config_handle_t` data structure and then injecting it using the `gn_node_create()` function. A `gn_node_handle_t` pointer will be returned, that is the reference to be passed in the next board configuration steps to create the necessary Leaves.

To start the Node execution loop, the `gn_node_start()` function has to be called. This will trigger the `xTaskCreate()` RTOS function per each configured leaf. 

Although a Node is intended to survive for the entire duration of the application, a `gn_node_destroy()` function is provided, to release Node resources. 

## Node statuses

The Node initialization process implies, depending on the configuration, the start of several services like WiFi provisioning, MQTT server connection, that requires time. In order to give the user the possibility to perform operations while the init process continues (like showing a message on the display or handle issues) it is possible to use a loop to retrieve the Node status and wait until init ends, and then proceed. 

A node has a status represented by the `gn_node_status_t` enum. 

The default initial status is `GN_NODE_STATUS_NOT_INITIALIZED`. During initialization process, it goes into `GN_NODE_STATUS_INITIALIZING`. If some errors occur, a specific status is associated (see [API](../html/index.html)). If everything goes well, the status is moved to `GN_NODE_STATUS_READY_TO_START`. This gives the user the OK to exit from the wait loop and proceed with starting the node operations.

After a successful call of `gn_node_start()` the node goes into `GN_NODE_STATUS_STARTED`. A good `main()` infinite loop could check if the status of the node changes and react accordingly. Note: you won't find it in the code as per today :)

### Code Sample: Node creation and startup

```
	gn_config_init_param_t config_init = {
		.provisioning_password = "grownode",
		.server_base_topic = "/grownode/mqttroottopic",
		...
	};

	//creates the config handle
	gn_config_handle_t config = gn_init(&config_init);
	...
	
	//creates a new node
	gn_node_config_handle_t node = gn_node_create(config, "my root node");
	...
	
	//waits until the config process ends
	while (gn_get_status(config) != GN_NODE_STATUS_READY_TO_START) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "grownode startup status: %s",
				gn_get_status_description(config));
	}
	...

	//finally, start node
	gn_node_start(node);
```
