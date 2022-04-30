
# Parameters

GrowNode allows users to access Leaves input and outputs through Parameters. A parameter defines its behavior and holds its value.
Depending on their configuration, parameters can be exposed and accessed from inside the code (eg. from an onboard temperature controller) or from the network (eg. to monitor the water level). They can be also updated in both ways. 

Parameters can be stored in the NVS flash (the ESP32 'hard drive') in order to be persisted over board restart, in a transparent way (no code needed). 

## Initialization

Each Leaf has a predetermined set of parameters. Those are initialized in the configuration phase described in the [Leaves](leaves.md) section. However, the initial values can be overridden by the user. For instance, a parameter defining a GPIO pin should be customized depending on the board circuit. To do this, the `gn_leaf_param_init_XXX()` functions are defined. Example: 

```
	gn_leaf_handle_t lights = gn_leaf_create(node, "light switch", gn_gpio_config, 4096);
	gn_leaf_param_init_double(lights, GN_GPIO_PARAM_GPIO, 25);
```

Here, a `lights` leaf is created using the `gn_gpio_config` callback. This leaf (see `gn_gpio` leaf code) has a parameter called `GN_GPIO_PARAM_GPIO` that represents the GPIO to control. This code assigns the value 25 to that parameter at startup.


## Overwrite stored parameters

A leaf parameter can be stored in the board flash depending on its configuration. Standard behavior is that on next startup the init function will be ignored. 

This could be overridden by using the `gn_leaf_param_force_XXX()` instead. this will set the parameter at the desired value ignoring the previously set value.

## Fast creation

Some leaves has convenient functions created to perform creation and initialization in a compact form. Those functions have the suffix `_fastcreate` (see for instance `gn_gpio_fastcreate()` on `gn_gpio.c` leaf)

## Update

A leaf parameter can be updated:

- from the network: see [MQTT Protocol](mqtt.md)
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
