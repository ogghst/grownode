# MQTT

GrowNode uses MQTT as messaging protocol. It has been choosen due to wide use across IoT community, becoming a de facto standard for those applications. Users can deploy their own MQTT server or use a public server in the network. 

GrowNode will connect to the MQTT server just after receiving wifi credentials.

## Configuration

See [Network Configuration](../start.md#network-startup) for a basic startup.

Parameters involved in the MQTT server configuration are described in `gn_config_init_param_t` struct to be passed to node creation:

- `bool server_board_id_topic`: whether grownode engine shall add the MAC address of the board in the topic prefix - default false;
- `char server_base_topic[80]`: the base topic where to publish messages, format shall include all slashes - example: `/grownode/test`;
- `char server_url[255]`: URL of the server, specified with protocol and port - example: `mqtt://192.168.1.170:1883`;
- `uint32_t server_keepalive_timer_sec`: GrowNode engine will send a keepalive message to MQTT server. This indicates the seconds between two messages. if not found or 0, keepalive messages will not be triggered;

## MQTT Protocol

> All messages are sent using QoS 0, retain = false. This to improve efficiency and performances on the node side. But this also means that if no listeners are subscribed when the message is lost. 

GrowNode uses JSON formatting to produce complex payloads. Parts in *italic* have to be replaced with your configuration.

### Startup messages

Upon Server connection, the Node will send a startup message.

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 0 			|
| Payload     | { "msgtype": "online" }       |

### LWT messages

LWT message (last will testament) is a particular message the MQTT server will publish if no messages are received from a certain time. 

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 1 			|
| **retain**  | 1 			|
| Payload     | { "msgtype": "offline" }  |

### Node Config message

This is a message showing the node configuration to the network. Currently, it is sent as keepalive.

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 0 			|
| Payload     | { "msgtype": "config"... (see example) }       | 

Example message:

```
{
  "msgtype": "config",
  "name": "node",
  "leaves": [
    {
      "name": "blink",
      "leaf_type": "gpio",
      "params": [
        {
          "name": "status",
          "type": "bool",
          "val": true
        },
        {
          "name": "inverted",
          "type": "bool",
          "val": false
        },
        {
          "name": "gpio",
          "type": "number",
          "val": 2
        }
      ]
    }
  ]
}
```

### Leaf parameter change request


To request a parameter to change, user shall send a mqtt message indicating the leaf and the parameter to change. This will be evaluated against the permissions for this parameter:

| From        | To          |
| ----------- | ----------- |
| Server      | Board       |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/*leaf*/*parameter*/CMD  |
| QoS         | 0 			|
| Payload     | parameter-dependent    |


### Leaf parameter change notify

When a parameter changes its status, if configured the board will send a parameter status change:

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/*leaf*/*parameter*/STS  |
| QoS         | 0 			|
| Payload     | parameter-dependent    |  

### Discovery messages

> This feature is not yet completed. Please consider it experimental.

Discovery messages are particular messages that allows servers like openHAB or HomeAssistant to automatically detect the node capabilities and expose them in their system. With this functionality users does not have to manually add sensors to their home automation servers.

Parameters involved in the configuration are described in `gn_config_init_param_t` struct to be passed to node creation:

- `bool server_discovery`: whether the functionality shall be enabled - default false
- `char server_discovery_prefix[80]`: topic prefix

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *server_discovery_prefix*  |
| QoS         | 0 			|
| Payload     | todo    | 


### OTA message

This message informs the node to upload the firmware from the config specified URL.

| From        | To          |
| ----------- | ----------- |
| Server      | Board       |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/CMD  |
| QoS         | 0 			|
| Payload     | OTA    | 

This gives the confirmation the OTA message has been processed and the OTA is in progress

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 0 			|
| Payload     | OTA         | 

### Reboot message

This message tells the node to reboot 

| From        | To          |
| ----------- | ----------- |
| Server      | Board       |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/CMD  |
| QoS         | 0 			|
| Payload     | RBT    | 

This gives the confirmation the reboot message has been processed and the board is rebooting

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 0 			|
| Payload     | RBT    | 

### Reset  message

This message tells the node to reset the NVS. This will bring to the initial configuration, including provisioning.

| From        | To          |
| ----------- | ----------- |
| Server      | Board       |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/CMD  |
| QoS         | 0 			|
| Payload     | RST    | 

This gives the confirmation the reset message has been processed and the reset is in progress

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 0 			|
| Payload     | RST    | 

### Log messages

This is sent by the [logging API](logging.mw)

| From        | To          |
| ----------- | ----------- |
| Board       | Server      |

| Parameter   | Description |
| ----------- | ----------- |
| Topic       | *base*/STS  |
| QoS         | 0 			|
| Payload     | { "msgtype": "log"; "tag": "_tag_"; "lev": "_lev_"; "msg": "_payload_" }   	    | 

