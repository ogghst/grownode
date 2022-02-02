
# Leaves

Every sensor or actuator is represented by a Leaf. The Leaf is the 'engine' of the underlying logic, it is designed to be reusable multiple times in a Node and to be configured in a consistent way. A Leaf represents the bridge between the User and the hardware layer, therefore it is handled by the GrowNode engine as a separated RTOS task, and is accessed in an asyncronous way.

## Using Leaves

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



