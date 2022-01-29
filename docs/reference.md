# Reference Guide

This page aims to describe the GrowNode API and how to use it to develop your own firmware.

GrowNode is developed on top of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) Development Framework, in order to directly access to the ESP32 microprocessor functionalities and the RTOS operating system.

##Basic Concepts

THe highest level structure on a GrowNode system is the board itself. Every solution you want to build is basically a combination of 

- Devices attached to the IO pins (tipycally handled by specific HAL - Hardware Abstraction Layers - like ESP-IDF core libraries or third party devices. *Example: I2C driver, a GPIO pin*

- Several logics to access to those devices and present to the GrowNode framwork [Leaves](#leaves) - several are prebuilt and more can be user defined. *Example: a temperature sensor, a relay, a LED, a motor*

- Several [Parameters](#Parameters) exposed to the Leaf, able to retrieve or command specific functionalities. *Example: the temperature retrieved, a motor switch, a light power*

- One centralized controller that orchestrates and exposes to the Leaves the services needed to work: [Node](#node). This is common on all the GrowNode implementation

- one entry point of the application, where the Node and Leaves are declared and configured [Board](#boards). *Example: a Water Tower Board, a simple Temperature and Humidity pot controller*

##Architecture

In ESP-IDF vacabulary, the Board and it corresponding Node works in the main application RTOS task, and each Leaf works in a separate task. This allows the parallel execution of task logic and, more than this, will avoid that a leaf waiting for things to happen (eg. sensor measure) to have side effects on the whole Node. All messaging across leaves is implemented through RTOS events and messages queues.

###Code reference

Code Documentation is described in [API](../html/index.html) section. The entry point of all GrowNode functionalities resides in `grownode.h` header file. Users just have to reference it in their code. 

##Node

The core element of a GrowNode implementation is the Node. It represents the container and the entry point for the board capabilities.

In order to proper create a Node, a configuration shall be supplied. This is done by creating a `gn_config_handle_t` data structure and then injecting it using the `gn_node_create()` function. A `gn_node_handle_t` pointer will be returned, that is the reference to be passed in the next board configuration step to create the necessary Leaves.

To start the Node execution loop, the `gn_node_start()` function has to be called. This will trigger the `xTaskCreate()` RTOS function per each configured leaf. 

Although a Node is intended to survive for the entire duration of the application, a `gn_node_destroy()` function is provided, to release Node resources. 

###Node statuses

The Node initialization process implies, depending on the configuration, the start of several services like WiFi provisioning, MQTT server connection, that requires time. In order to give the user the possibility to perform operations while the init process continues (like showing a message on the display or handle issues) it is possible to retrieve the Node status and wait until it is in the correct one to proceed. 

A node has a status represented by the `gn_node_status_t` enum. Default, initial status is `GN_NODE_STATUS_NOT_INITIALIZED`. During initialization process, it goes into `GN_NODE_STATUS_INITIALIZING`. If some errors occurs, a specific status is representing it (see [API](../html/index.html)). If everything goes well, the status is moved to `GN_NODE_STATUS_READY_TO_START`. This gives the user the OK to exit from the wait loop and proceed with starting the node operations.

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


##Leaves

Every sensor or actuator is represented by a Leaf. The Leaf is the 'engine' of underlying logic, it is designed to be reusable multiple times in a Node and be configured in a consistent way. Due to the fact it is the bridge between the User and the hardware layer, the Leaf is handled by the Grownode engine as a separated RTOS task, and is therefore accessed in a asyncronous way.

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

The `gn_leaf_descriptor_handle_t` is a reference to the informations configured.

Grownode engine will use later those info to start the leaf. Another callback must be implemented in the leaf:

```
typedef void (*gn_leaf_task_callback)(gn_leaf_handle_t leaf_config);
```

In this callback it is contained the business logic of the leaf, like
 
- reading the Leaf parameter status
- listening for parameter updates from external sources (network or internal)
- updating the user UI
- working with underline hardware resources 
- updating its parameters

### Examples

```
	//creates the moisture sensor
	moisture_leaf = gn_leaf_create(node, "moisture", gn_capacitive_moisture_sensor_config, 4096);
```

##Parameters

GrowNode allow users to access Leaves input and outputs through Parameters. A parameters defines its behavior and hold its value.
Depending on their configuration, parameters can be exposed and accessed from inside the code (e.g. from an onboard temperature controller) or from the network (e.g. to monitor the water level). They can be also updated in both ways. 

###Initialization

Each Leaf has a predetermined set of parameters. Those are initialized in the configuration phase described in [Leaves](#leaves) section. The initial value can be overridden by the user however. For instance, a parameter defining a GPIO pin should be customized depending on the board circuit. To do this, the `gn_leaf_param_init_XXX()` functions are defined. Example: 

```
	gn_leaf_handle_t lights = gn_leaf_create(node, "light switch", gn_gpio_config, 4096);
	gn_leaf_param_init_double(lights, GN_GPIO_PARAM_GPIO, 25);
```

Here, a `lights` leaf is created using the `gn_gpio_config` callback. This leaf (see `gn_gpio` leaf code) has a parameter called `GN_GPIO_PARAM_GPIO` that represents the GPIO to control. This code assigns the value 25 to that parameter at startup.

###Update

A leaf parameter can be updated

- from the network: see see [MQTT Protocol](#MQTT)
- from the code

When updating from user code, the `gn_leaf_param_set_XXX()` functions are used. They inform the leaf that the parameter shall be changed to a new value. This is done via event passing as the leaf resides to another task, so it's an asynchronous call.

###Parameter configuration


####Types

Parameters are strong typed. That means that internally into Grownode engine they are represented using C types. Types are enumerated in `gn_val_type_t`

```
typedef enum {
	GN_VAL_TYPE_STRING, 
	GN_VAL_TYPE_BOOLEAN, 
	GN_VAL_TYPE_DOUBLE,
} gn_val_type_t;
```

- **GN_VAL_TYPE_BOOLEAN** (true/false)
- **GN_VAL_TYPE_DOUBLE** (floating point with signature)
- **GN_VAL_TYPE_STRING** (character array, user defined dimension)


Storage of the value is made by an union called `gn_val_t`:

```
typedef union {
	char *s;
	bool b;
	double d;
} gn_val_t;
```

####Access Type

Parameters can have multiple uses, and therefore its access type can be different:

- 	**GN_LEAF_PARAM_ACCESS_ALL** - can be modified both by the node and network (eg. local configuration settings)
- 	**GN_LEAF_PARAM_ACCESS_NETWORK** - can be modified only by network (eg. configuration settings from environment)
- 	**GN_LEAF_PARAM_ACCESS_NODE** - can be modified only by the node (eg. sensor data)
- 	**GN_LEAF_PARAM_ACCESS_NODE_INTERNAL** - can be modified only by the node (eg. sensor data) and it is not shown externally

`gn_leaf_param_access_type_t`


```
gn_leaf_param_handle_t gn_leaf_param_create(gn_leaf_handle_t leaf_config,
		const char *name, const gn_val_type_t type, const gn_val_t val,
		gn_leaf_param_access_type_t access, gn_leaf_param_storage_t storage,
		gn_validator_callback_t validator);
```


### Responsibilites
todo

### Configuration
todo

### Examples
todo

##Boards
todo

##MQTT

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