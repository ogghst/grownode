# Get Started

<p align="center">
<img src="img/grownode_logo_full.png">
</p>

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment. It is currently based on the original work of [Nicola Muratori](https://github.com/ogghst). 

It all started merging the need to keep updated the software development skills, the curiosity for the IOT world, and the idea that technology could help in restoring the connection between urban people and the food.

## Architecture

A typical architecture is composed by

 - one or more grownode boards (called nodes)
 - each board is connected to several sensors (called leaves)
 - a messaging server where each board talks using MQTT
 - an automation server - like openhab, home assistant - that orchestrate the node operations
 - several clients to display and manage nodes
 - other systems, like home automation, gardening, that works together with grownode

<p align="center">
<img src="mg/grownode_net.png">
</p>

### Technologies

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

### Functionalities

Grownode provides functionalities that lets you kickstart your projects in minutes:

- SoftAP/Bluetooth Provisioning to join and change your wireless network without any hardcode configuration through a mobile app
- High level device configuration through makefile 
- Firmware update Over The Air
- NTP clock sync
- Persistent storage of parameters
- Sensor and Actuators configuration abstracting the hardware level
- Transparent networking protocol (presentation, keepalive, logging)
- Client/Server and Client/Client parameter retrieval and update through MQTT, having in mind highly distributed environment (mesh networks, cloud communication)
  
# Getting Started

## Setup your environment

### IDE

### ESP-IDF



- install ESP-IDF as per [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)


## Build the environment

- install ESP_IDF-LIB as per [ESP-IDF Components library](https://github.com/UncleRus/esp-idf-lib)
- setup main `CMakeLists.txt` file according to your local paths
- `git clone` this repository
- add `lvgl` (release/v8) and `lvgl_esp32_drivers` (master) as components as specified [here](https://github.com/lvgl/lv_port_esp32)
- connect your ESP32 board to a serial port
- open an ESP-IDF prompt to go to project directory
- type `idf.py menuconfig flash build monitor` according to your needs. this will run the latest test code as per the `main.c` file

## 
