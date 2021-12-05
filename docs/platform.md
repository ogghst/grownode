#A quick look into GrowNode systems

## Architecture

A typical GrowNode system architecture is composed by

 - one or more grownode boards (called nodes)
 - each board is connected to several sensors (called leaves)
 - a messaging server where each board talks using MQTT
 - an automation server - like openhab, home assistant - that orchestrate the node operations
 - several clients to display and manage nodes
 - other systems, like home automation, gardening, that works together with grownode

<p align="center">
<img src="/img/grownode_net.png">
</p>

## Technologies

GrowNode aims to use most common development tools. Actual release is composed by:

### Hardware

 - ESP32 and above microcontrollers
 - Various displays tested (ILI9341) with touch screen (tested XPT2046) capabilities
 - Common sensors and actuators (relay, PWM output, temperature probes, capacitance sensors..) with any esp-idf compatible libraries

### Software Components

 - ESP-IDF (release 4.4) programming environment
 - MQTT as a messaging system
 - LVGL as display library

### Others (optional)

 - OpenHab/Home Assistant as a controller
 - Fusion360 as Schematic and 3D CAD

## Functionalities

Grownode provides functionalities that lets you kickstart your projects in minutes:

- SoftAP/Bluetooth Provisioning to join and change your wireless network without any hardcode configuration through a mobile app
- High level device configuration through makefile 
- Firmware update Over The Air
- NTP clock sync
- Persistent storage of parameters
- Sensor and Actuators configuration abstracting the hardware level
- Transparent networking protocol (presentation, keepalive, logging)
- Client/Server and Client/Client parameter retrieval and update through MQTT, having in mind highly distributed environment (mesh networks, cloud communication)
  
