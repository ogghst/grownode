# Networking

GrowNode uses standard ESP32 provisioning framework to connect your wifi network. It can use SoftAP or BLE provisioning, to be specified from the [build system](build-system).

Once provisioned, GrowNode engine will try to estabilish a connection to the specified WiFi network during the `gn_init()` initialization process. If the network connection cannot be estabilished, the board can be configured to wait forever or to reset its provisioning status (cancelling the wifi credentials and restarting) in order to be ready to join another network.

## Configuration

See [Network Configuration](start.md#network-startup) for a basic startup.

Parameters involved in the WiFi configuration are described in `gn_config_init_param_t` struct to be passed to node creation:

- `bool provisioning_security`: defines if the provisioning protocol should use encrypted communication and proof of possession - default true;
- `char provisioning_password[9]`: define the password an user shall enter to prove being a grownode administrator;
- `int16_t wifi_retries_before_reset_provisioning`: how many times the wifi driver tries to connect to the network before resetting provisioning info - -1 to never lose provisioning (warning: in case of SSID change, no way to reset!
	
Upon connection, a `GN_NET_CONNECTED_EVENT` is triggered, and a `GN_NET_DISCONNECTED_EVENT` is triggered upon disconnection.
