

# GrowNode

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment.

[test link](introduction.md)

![GrowNode Main Screen with a Temperature Leaf](/docs/resources/main_screen.png "Main Screen")

# Architecture

GrowNode aims to use most common development tools. Actual release is composed by:

## Hardware

 - ESP32/ESP32S2 microcontrollers
 -  Various displays tested (ILI9341) with touch screen (tested XPT2046) capabilities
 -  Common sensors with any esp-idf compatible libraries

## Software Components

 - ESP-IDF (master branch) programming environment
 - MQTT as a messaging system
 - LVGL as display library

## Others (planning)
 - OpenHab as a controller
 - Fusion360 as Schematic and 3D CAD


## Functionalities

Every node provides some basic services

- High level device configuration
- Hardware and Software abstraction layer - think about nodes as devices and leaves as sensors
- SoftAP/Bluetooth Provisioning to join and change your wireless network without any hardcode configuration through a mobile app
- Firmware update Over The Air
- NTP clock sync
- Client/Server and Client/Client network configuration and high level messaging protocol, having in mind highly distributed environment (mesh networks, cloud communication)

## Testing

Unit testing are working although in early stage (started using after core writing), both SOC and Host tests are implemented

# How to use example

- install ESP-IDF as per [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- install ESP_IDF-LIB as per [ESP-IDF Components library](https://github.com/UncleRus/esp-idf-lib)
- setup main `CMakeLists.txt` file according to your local paths
- `git clone` this repository
- add `lvgl` (release/v8) and `lvgl_esp32_drivers` (master) as components as specified [here](https://github.com/lvgl/lv_port_esp32)
- connect your ESP32 board to a serial port
- open an ESP-IDF prompt to go to project directory
- type `idf.py menuconfig flash build monitor` according to your needs. this will run the latest test code as per the `main.c` file