# Reference Guide

This page aims to describe the GrowNode API and how to use it to develop your own firmware. 

> Disclaimer: This is NOT intended for ready-made solution users. Don't tell me I've not warned you! :)

GrowNode is based on the ESP32 microprocessor, and is developed on top of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) Development Framework. This allows one to directly access to the ESP32 functionalities and the real-time operating system (RTOS).

In order to mantain a coherent code style and ease the access of all ESP-IDF functionalities, GrowNode is written in pure C, although a C++ version could be done in next future. 

## Basic Concepts

The highest level structure on a GrowNode system is the board itself. Every solution you want to build is basically a combination of:

- Devices attached to the IO pins (typically handled by specific HAL - Hardware Abstraction Layers - like ESP-IDF core libraries or third party devices). *Examples: I2C driver, a GPIO pin*

- Several logics to access to those devices, which are called [Leaves](#leaves) in the GrowNode framework - several are prebuilt and more can be user defined. *Examples: a temperature sensor, a relay, a LED, a motor*

- Several [Parameters](#parameters) exposed by/to the Leaf, able to retrieve/command specific functionalities. *Examples: the temperature retrieved, a motor switch, a light power*

- One centralized controller that orchestrates and exposes to the Leaves the services needed to work, called [Node](#node). This is common on all the GrowNode implementations

- one entry point of the application, where the Node and Leaves are declared and configured, called [Board](#boards). *Examples: a Water Tower Board, a simple Temperature and Humidity pot controller*

## Architecture

In ESP-IDF vocabulary, the Board and its corresponding Node works in the main application RTOS task, and each Leaf works in a separate task. This allows the parallel execution of task logic and, moreover, avoids that a leaf in waiting state (eg. waiting for a sensor measure) affects the running of the whole Node. All messaging across leaves is implemented through RTOS events and message queues.

Putting all together, here's an overview of GrowNode platform:

<p align="center">
<img src="../img/platform.png" width="80%">
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

##User defined Leaves

This section aims to give you necessary knowledge over the Leaves management framework. Is intended for users that want to use preconfigured leaves or to build new leaves. It's a rather simple task and many working examples are present in the `leaves` folder.

A Leaf is tipycally used to bridge the hardware layer, supported by a collection of Parameters that represents the inputs and outputs of its operations.

A Leaf can also be used to coordinate other leaves by sending messages and modifying their parameters.

In order to approach to this topic, you must understand the parameter API features.

### Parameter declaration

The declaration of a parameter inside a Leaf is done by calling `gn_leaf_param_create()`. Signature:

```
gn_leaf_param_handle_t gn_leaf_param_create(gn_leaf_handle_t leaf_config,
		const char *name, const gn_val_type_t type, const gn_val_t val,
		gn_leaf_param_access_type_t access, gn_leaf_param_storage_t storage,
		gn_validator_callback_t validator);
```

The return type of this function is a reference to the parameter, that will be stored into the leaf for future use.

### Parameter Types

Parameters are strong typed. That means that internally into Grownode engine they are represented using C types. Types are enumerated in `gn_val_type_t`.

```
typedef enum {
	GN_VAL_TYPE_STRING,			/*!< character array, user defined dimension */
	GN_VAL_TYPE_BOOLEAN,		/*!< true/false */
	GN_VAL_TYPE_DOUBLE,			/*!< floating point with sign */
} gn_val_type_t;
```

Storage of the value is made by an union called `gn_val_t`:

```
typedef union {
	char *s;
	bool b;
	double d;
} gn_val_t;
```

As you can see from this definition, it is user responsibility to allocate memory in case of a char array parameter. This has to be done inside the Leaf code.

### Access Type

Parameters can have multiple uses, and therefore its access type can be different:

```
typedef enum {
	GN_LEAF_PARAM_ACCESS_ALL = 0x01, 			/*!< param can be modified both by the node and network (eg. local configuration settings)*/
	GN_LEAF_PARAM_ACCESS_NETWORK = 0x02,		/*!< param can be modified only by network (eg. configuration settings from environment)*/
	GN_LEAF_PARAM_ACCESS_NODE = 0x03, 			/*!< param can be modified only by the node (eg. sensor data)*/
	GN_LEAF_PARAM_ACCESS_NODE_INTERNAL = 0x04 	/*!< param can be modified only by the node (eg. sensor data) and it is not shown externally*/
} gn_leaf_param_access_type_t;
```

The access type is evaluated upon parameter change. If the request is not compatible with the access type (eg. a network request against a GN_LEAF_PARAM_ACCESS_NODE access type) the request won't have any effect. 

### Storage

Some parameters holds the board hardware configuration, like the GPIO pin a sensor is attached to, or board status information that neeed to survive over board restarts or power failures (like the standard power a pump shall be activated). Those parameters can be stored on each update in the ESP32 Non Volatile Storage (NVS). The current implementation stores in key-value pairs, before hashing the key using leaf name and parameter name.

```
typedef enum {
	GN_LEAF_PARAM_STORAGE_PERSISTED, 	/*!< param is stored in NVS flash every time it changes*/
	GN_LEAF_PARAM_STORAGE_VOLATILE 		/*< param is never stored in NVS flash*/
} gn_leaf_param_storage_t;
```

> Pay attention to not persist parameters that have continuous updates, like temperature. It can cause a fast degradation of the board memory!

### Validators

Making sure the parameter update arriving from the network makes sense can be a boring task for a leaf developer. And risk of forgetting a check and allow unsafe values can break the code or even make the system dangerous (think of turning on a pump at exceeding speed or without a time limit).

For this reason, the Grownode platform exposes a reusable mechanism to make the code safer: parameter validators.

Validators are functions compliant to the `gn_validator_callback_t` callback:

```
typedef gn_leaf_param_validator_result_t (*gn_validator_callback_t)(
		gn_leaf_param_handle_t param, void **value);
```

The intended behavior is to check the value agains predetermined values, and return the result code on its `gn_leaf_param_validator_result_t` variable:

```
typedef enum {
	GN_LEAF_PARAM_VALIDATOR_PASSED = 0x000,					/*!< value is compliant */
	GN_LEAF_PARAM_VALIDATOR_ERROR_ABOVE_MAX = 0x001,		/*!< value is over the maximum limit */
	GN_LEAF_PARAM_VALIDATOR_ERROR_BELOW_MIN = 0x002,		/*!< value is below the minimum limit */
	GN_LEAF_PARAM_VALIDATOR_ERROR_NOT_ALLOWED = 0x100,		/*!< value is not allowed for other reasons */
	GN_LEAF_PARAM_VALIDATOR_ERROR_GENERIC = 0x101,			/*!< algorithm has returned an error */
	GN_LEAF_PARAM_VALIDATOR_PASSED_CHANGED = 0x200			/*!< value was not allowed but has been modified by the validator to be compliant*/
} gn_leaf_param_validator_result_t;
```

As you can see, there is a possibility that the validator changes the value of the parameter, like if the value to be checked is below zero then set the value to zero. 

Standard validators are present for standard value types:

```
gn_leaf_param_validator_result_t gn_validator_double_positive(
		gn_leaf_param_handle_t param, void **param_value);

gn_leaf_param_validator_result_t gn_validator_double(
		gn_leaf_param_handle_t param, void **param_value);

gn_leaf_param_validator_result_t gn_validator_boolean(
		gn_leaf_param_handle_t param, void **param_value);
```

But it's easy to add new ones. Examples can be found in `gn_hydroboard2_watering_control.c` code:


```
gn_leaf_param_validator_result_t _gn_hb2_watering_time_validator(
		gn_leaf_param_handle_t param, void **param_value) {

	double val;
	if (gn_leaf_param_get_value(param, &val) != GN_RET_OK)
		return GN_LEAF_PARAM_VALIDATOR_ERROR;

	double _p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_time_validator - param: %d", (int )_p1);

	if (GN_HYDROBOARD2_MIN_WATERING_TIME > **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MIN_WATERING_TIME,
				sizeof(GN_HYDROBOARD2_MIN_WATERING_TIME));
		return GN_LEAF_PARAM_VALIDATOR_PASSED_CHANGED;
	} else if (GN_HYDROBOARD2_MAX_WATERING_TIME < **(double**) param_value) {
		memcpy(param_value, &GN_HYDROBOARD2_MAX_WATERING_TIME,
				sizeof(GN_HYDROBOARD2_MAX_WATERING_TIME));
		return GN_LEAF_PARAM_VALIDATOR_PASSED_CHANGED;
	}

	_p1 = **(double**) param_value;
	ESP_LOGD(TAG, "_watering_time_validator - param: %d", (int )_p1);

	return GN_LEAF_PARAM_VALIDATOR_PASSED;
}
```

##Standard Leaves




## Boards

Boards are a collection of preconfigured Leaves to have a ready made solution, described in [Boards](boards.md) section. Goal is to give you a working solution without the need of develop your own code.

In order to include a board in your code, you just need to modify your `main.c` file including the appropriate header file in `components/grownode/boards library` folder and call the appropriate board configuration function:

```
	...
	//header include the board you want to start here
	#include "gn_blink.h"
	...
	//creates a new node
	gn_node_handle_t node = gn_node_create(config, "node");

	//the board to start
	gn_configure_blink(node);

	//finally, start node
	gn_node_start(node);

```

## Event subsystem

Main application works in a RTOS task. Leaves works in dedicated tasks. Networking and other ESP-IDF services has their own tasks as well. This means that all communication through those components must be done using the RTOS task messaging features and higher ESP-IDF abstractions.

GrowNode uses [Event Loop library](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html) from ESP-IDF to list, subscribe and publish events. It declares one base event `GN_BASE_EVENT` and a `gn_event_id_t` enumeration where all event types are listed.

### Subscribing events

In order to grab a specific event you can rely on ESP-IDF event loop functions. In order to recall the proper event loop from a Leaf or Node, `gn_leaf_get_event_loop()` and `gn_node_get_event_loop()` are provided. See example:

```
//register for events
esp_event_handler_instance_register_with(gn_leaf_get_event_loop(leaf_config), GN_BASE_EVENT,  
	GN_EVENT_ANY_ID, gn_leaf_led_status_event_handler, leaf_config, NULL);

```

This code creates a subscription for all events (`GN_EVENT_ANY_ID`), calling `gn_leaf_led_status_event_handler` callback once an event is triggered, and pass the `leaf_config` pointer in the context.

Those functions returns currently the same event loop, different implementations are made for future needs.

### Publishing events

Event are published using straight esp event loop functionalities: 

```
esp_event_post_to(event_loop, GN_BASE_EVENT,
		GN_NET_CONNECTED_EVENT, NULL, 0,
		portMAX_DELAY);
```

### Listening for event

Event callbacks shall implement `esp_event_handler_t` syntax. Payload is dependent on the event type triggered: 

```
void gn_pump_control_task_event_handler(void *handler_args,
		esp_event_base_t base, int32_t event_id, void *event_data) {

	gn_leaf_parameter_event_t *evt = (gn_leaf_parameter_event_t*) event_data;
	switch (event_id) {
	case GN_LEAF_PARAM_CHANGED_EVENT:
...
```

### Acquiring events in leaf code

Events addressed to leaves from other leaves and from network has a special, direct way to be processed, that improves GrowNode engine performances, using RTOS queues. Leaves that has just to wait for an event can implement an infinite loop where an `xQueueReceive()` function waits for events for a specific time window. If no events are presents in the queue, the queue releases the control to the leaf: 

```
gn_leaf_parameter_event_t evt;
	
if (xQueueReceive(gn_leaf_get_event_queue(leaf_config), &evt, 
	pdMS_TO_TICKS(100)) == pdPASS) {
	
	//event arrived for this node
	switch (evt.id) {

		//parameter change
		case GN_LEAF_PARAM_CHANGE_REQUEST_EVENT:

		ESP_LOGD(TAG, "request to update param %s, data = '%s'",
			evt.param_name, evt.data);
	...
```

## Networking

GrowNode uses standard ESP32 provisioning framework to connect your wifi network. It can use SoftAP or BLE provisioning, to be specified from the [build system](#build-system).

Once provisioned, GrowNode engine will try to estabilish a connection to the specified WiFi network during the `gn_init()` initialization process. If the network connection cannot be estabilished, the board can be configured to wait forever or to reset its provisioning status (cancelling the wifi credentials and restarting) in order to be ready to join another network.

### Configuration

See [Network Configuration](start.md#network-startup) for a basic startup.

Parameters involved in the WiFi configuration are described in `gn_config_init_param_t` struct to be passed to node creation:

- `bool provisioning_security`: defines if the provisioning protocol should use encrypted communication and proof of possession - default true;
- `char provisioning_password[9]`: define the password an user shall enter to prove being a grownode administrator;
- `int16_t wifi_retries_before_reset_provisioning`: how many times the wifi driver tries to connect to the network before resetting provisioning info - -1 to never lose provisioning (warning: in case of SSID change, no way to reset!
	
Upon connection, a `GN_NET_CONNECTED_EVENT` is triggered, and a `GN_NET_DISCONNECTED_EVENT` is triggered upon disconnection.

## MQTT

GrowNode uses MQTT as messaging protocol. It has been choosen due to wide use across IoT community, becoming a de facto standard for those applications. Users can deploy their own MQTT server or use a public server in the network. 

GrowNode will connect to the MQTT server just after receiving wifi credentials.

### Configuration

See [Network Configuration](start.md#network-startup) for a basic startup.

Parameters involved in the MQTT server configuration are described in `gn_config_init_param_t` struct to be passed to node creation:

- `bool server_board_id_topic`: whether grownode engine shall add the MAC address of the board in the topic prefix - default false;
- `char server_base_topic[80]`: the base topic where to publish messages, format shall include all slashes - example: `/grownode/test`;
- `char server_url[255]`: URL of the server, specified with protocol and port - example: `mqtt://192.168.1.170:1883`;
- `uint32_t server_keepalive_timer_sec`: GrowNode engine will send a keepalive message to MQTT server. This indicates the seconds between two messages. if not found or 0, keepalive messages will not be triggered;

### MQTT Protocol

> All messages are sent using QoS 0, retain = false. This to improve efficiency and performances on the node side. But this also means that if no listeners are subscribed when the message is lost. 

GrowNode uses JSON formatting to produce complex payloads. 

#### Startup messages

Upon Server connection, the Node will send a startup message.

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/STS  |
| QoS         | 0 			|
| Payload     | { "msgtype": "online" }       |

#### LWT messages

LWT message (last will testament) is a particular message the MQTT server will publish if no messages are received from a certain time. 

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/STS  |
| QoS         | 1 			|
| **retain**  | 1 			|
| Payload     | { "msgtype": "offline" }  |

#### Node Config message

This is a message showing the node configuration to the network. Currently, it is sent as keepalive.

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/STS  |
| QoS         | 0 			|
| Payload     | { "msgtype": "config" }       | 

todo

### Leaf parameter change notify

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/<leaf>/<parameter>/STS  |
| QoS         | 0 			|
| Payload     | parameter-dependent    | 

### Leaf parameter change request

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/<leaf>/<parameter>/CMD  |
| QoS         | 0 			|
| Payload     | parameter-dependent    | 

#### Discovery messages

> This feature is not yet completed. Please consider it experimental.

Discovery messages are particular messages that allows servers like openHAB or HomeAssistant to automatically detect the node capabilities and expose them in their system. With this functionality users does not have to manually add sensors to their home automation servers.

Parameters involved in the configuration are described in `gn_config_init_param_t` struct to be passed to node creation:

- `bool server_discovery`: whether the functionality shall be enabled - default false
- `char server_discovery_prefix[80]`: topic prefix

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <server_discovery_prefix>  |
| QoS         | 0 			|
| Payload     | todo    | 


### OTA message

This message informs the node to upload the firmware from the config specified URL.

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/CMD  |
| QoS         | 0 			|
| Payload     | OTA    | 

This gives the confirmation the OTA message has been processed and the OTA is in progress

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/STS  |
| QoS         | 0 			|
| Payload     | OTA    | 

### Reboot message

This message informs the node to reboot 

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/CMD  |
| QoS         | 0 			|
| Payload     | RBT    | 

This gives the confirmation the reboot message has been processed and the board is rebooting

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/STS  |
| QoS         | 0 			|
| Payload     | RBT    | 

### Reset  message

This message informs the node to reset the NVS. This will bring to the initial configuration, including provisioning.

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/CMD  |
| QoS         | 0 			|
| Payload     | RST    | 

This gives the confirmation the reset message has been processed and the reset is in progress

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/STS  |
| QoS         | 0 			|
| Payload     | RST    | 

### Leaf parameter change request

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | <base>/<leaf>/<parameter>/CMD  |
| QoS         | 0 			|
| Payload     | parameter-dependent    | 


##Display

todo

##Logging

todo

##Error handling

todo

##Utilities

todo

## Build System

Grownode relies on ESP-IDF build system. It is designed to be a component, and you can configure the build options via standard ESP-IDF command line or from your IDE. See [Configuration](../workflow/#configure-your-project) section.
