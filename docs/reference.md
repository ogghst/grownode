# Reference Guide

This page aims to describe the GrowNode API and how to use it to develop your own firmware. 

> Disclaimer: This is NOT intended for ready-made solution users. Don't tell me I've not warned you! :)

GrowNode is based on the ESP32 microprocessor, and is developed on top of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) Development Framework. This allows one to directly access to the ESP32 functionalities and the real-time operating system (RTOS).

## Basic Concepts

The highest level structure on a GrowNode system is the board itself. Every solution you want to build is basically a combination of:

- Devices attached to the IO pins (typically handled by specific HAL - Hardware Abstraction Layers - like ESP-IDF core libraries or third party devices). *Examples: I2C driver, a GPIO pin*

- Several logics to access to those devices, which are called [Leaves](#leaves) in the GrowNode framework - several are prebuilt and more can be user defined. *Examples: a temperature sensor, a relay, a LED, a motor*

- Several [Parameters](#parameters) exposed by/to the Leaf, able to retrieve/command specific functionalities. *Examples: the temperature retrieved, a motor switch, a light power*

- One centralized controller that orchestrates and exposes to the Leaves the services needed to work, called [Node](#node). This is common on all the GrowNode implementations

- one entry point of the application, where the Node and Leaves are declared and configured, called [Board](#boards). *Examples: a Water Tower Board, a simple Temperature and Humidity pot controller*

## Architecture

In ESP-IDF vocabulary, the Board and its corresponding Node works in the main application RTOS task, and each Leaf works in a separate task. This allows the parallel execution of task logic and, moreover, avoids that a leaf in waiting state (eg. waiting for a sensor measure) affects the running of the whole Node. All messaging across leaves is implemented through RTOS events and message queues.

Putting all together, here's an overview of GrowNode platform.

<p align="center">
<img src="img/platform.png" width="80%">
</p>

### Code reference

Code Documentation is described in [API](../html/index.html) section. The entry point of all GrowNode functionalities resides in the `grownode.h` header file. Users just have to reference it in their code. 

## Node

The core element of a GrowNode implementation is the Node. It represents the container and the entry point for the board capabilities.

In order to properly create a Node, a configuration shall be supplied. This is done by creating a `gn_config_handle_t` data structure and then injecting it using the `gn_node_create()` function. A `gn_node_handle_t` pointer will be returned, that is the reference to be passed in the next board configuration steps to create the necessary Leaves.

To start the Node execution loop, the `gn_node_start()` function has to be called. This will trigger the `xTaskCreate()` RTOS function per each configured leaf. 

Although a Node is intended to survive for the entire duration of the application, a `gn_node_destroy()` function is provided, to release Node resources. 

### Node statuses

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


## Leaves

Every sensor or actuator is represented by a Leaf. The Leaf is the 'engine' of the underlying logic, it is designed to be reusable multiple times in a Node and to be configured in a consistent way. A Leaf represents the bridge between the User and the hardware layer, therefore it is handled by the GrowNode engine as a separated RTOS task, and is accessed in an asyncronous way.

Every leaf shall expose a `gn_leaf_config_callback` callback function that initializes its resources. 

In order to add a leaf to a node, the `gn_leaf_create` function is called first:

```
gn_leaf_handle_t gn_leaf_create(gn_node_handle_t node,
		const char *name, gn_leaf_config_callback leaf_config, size_t task_size)
```

This binds the Leaf to the parent Node, and tells the Node to use the `gn_leaf_config_callback` callback to initialize the resources at the appropriate moment. Typical job of a config callback function is to load and initialize its parameters and allocate memory for the side structures. Callback definition is:

```
typedef gn_leaf_descriptor_handle_t (*gn_leaf_config_callback)(
		gn_leaf_handle_t leaf_config);
```

The `gn_leaf_descriptor_handle_t` is a reference to the information configured.

GrowNode engine will use later those info to start the leaf. Another callback must be implemented in the leaf:

```
typedef void (*gn_leaf_task_callback)(gn_leaf_handle_t leaf_config);
```

In this callback it is contained the business logic of the leaf, like:
 
- reading the Leaf parameter status
- listening for parameter updates from external sources (network or internal)
- updating the user UI
- working with underlying hardware resources 
- updating its parameters

### Examples

```
	//creates the moisture sensor
	moisture_leaf = gn_leaf_create(node, "moisture", gn_capacitive_moisture_sensor_config, 4096);
```

## Parameters

GrowNode allows users to access Leaves input and outputs through Parameters. A parameter defines its behavior and holds its value.
Depending on their configuration, parameters can be exposed and accessed from inside the code (eg. from an onboard temperature controller) or from the network (eg. to monitor the water level). They can be also updated in both ways. 

Parameters can be stored in the NVS flash (the ESP32 'hard drive') in order to be persisted over board restart, in a transparent way (no code needed). 

### Initialization

Each Leaf has a predetermined set of parameters. Those are initialized in the configuration phase described in the [Leaves](#leaves) section. However, the initial values can be overridden by the user. For instance, a parameter defining a GPIO pin should be customized depending on the board circuit. To do this, the `gn_leaf_param_init_XXX()` functions are defined. Example: 

```
	gn_leaf_handle_t lights = gn_leaf_create(node, "light switch", gn_gpio_config, 4096);
	gn_leaf_param_init_double(lights, GN_GPIO_PARAM_GPIO, 25);
```

Here, a `lights` leaf is created using the `gn_gpio_config` callback. This leaf (see `gn_gpio` leaf code) has a parameter called `GN_GPIO_PARAM_GPIO` that represents the GPIO to control. This code assigns the value 25 to that parameter at startup.

### Fast creation

Some leaves has convenient functions created to perform creation and initialization in a compact form. Those functions have the suffix `_fastcreate` (see for instance `gn_gpio_fastcreate()` on `gn_gpio.c` leaf)

### Update

A leaf parameter can be updated:

- from the network: see [MQTT Protocol](#mqtt)
- from the code

When updating from user code, the `gn_leaf_param_set_XXX()` functions are used. They inform the leaf that the parameter shall be changed to a new value. This is done via event passing as the leaf resides to another task, so it's an asynchronous call.

### Code Sample: Leaf declaration and parameters initialization

This is the complete code to create and configure a BME280 Leaf sensor, a temperature + humidity + pressure sensor (for complete description of this sensor, see [leaves](leaves.md)

```
	gn_leaf_handle_t env_thp = gn_leaf_create(node, "bme280", gn_bme280_config, 8192);
	gn_leaf_param_init_double(env_thp, GN_BME280_PARAM_SDA, 21);
	gn_leaf_param_init_double(env_thp, GN_BME280_PARAM_SCL, 22);
	gn_leaf_param_init_bool(env_thp, GN_BME280_PARAM_ACTIVE, true);
	gn_leaf_param_init_double(env_thp, GN_BME280_PARAM_UPDATE_TIME_SEC, 10);
```

###Next steps: Build your own leaves

Once mastered basic concepts of leaves creation and parametrization, you can start developing your own: see [Leaves](leaves.md) page


## Boards
todo

## MQTT

todo 

### Leaf - Server messaging

todo

### Leaf - leaf messaging

todo


##Display

todo

##Logging

todo

##Error handling

todo

##Utilities

todo
