
<p align="center">
<img src="docs/img/grownode_logo_full.png">
</p>

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment.

See [Doc Website](https://ogghst.github.io/grownode/)

# Architecture

GrowNode aims to use most common development tools. Actual release is composed by:

## Hardware

 - ESP32 and above microcontrollers
 - Various displays tested (ILI9341) with touch screen (tested XPT2046) capabilities
 - Common sensors and actuators (relay, PWM output, temperature probes, capacitance sensors..) with any esp-idf compatible libraries

## Software Components

 - ESP-IDF (release 4.4) programming environment
 - MQTT as a messaging system
 - LVGL as display library

## Others (optional)

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
  
## Testing

Unit testing are working although in early stage (started using after core writing), both SOC and Host tests are implemented

# How to use example

## Setup your environment

### ESP-IDF

 - install ESP-IDF as per [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
 - install ESP_IDF-LIB as per [ESP-IDF Components library](https://github.com/UncleRus/esp-idf-lib) in the same root folder of ESP-IDF
 - you should have then a folder structure like:
>     root folder
>     +--esp-idf-master
>     +--esp-idf-lib-master

### IDE
 - Latest ESP-IDF installation are including an Eclipse development environment that is already configured to run an ESP-IDF project. 
 - If you want to install your own IDE, prepare your favorite development environment (Anyway, I am personally using [eclipse](https://www.eclipse.org/downloads/) with [ESP-IDF plugin](https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md)

## Obtain GrowNode

 - Open an IDF shell (you should find in in your windows start menu - it is called ESP-IDF 4.4 CMD), change dir where you want to install grownode
 - Clone GrowNode repository using command `git clone --recurse-submodules https://github.com/ogghst/grownode.git`

## Check everything works

### Via ESP-IDF command prompt

 - Open an IDF shell (you should find in in your windows start menu - it is called ESP-IDF 4.4 CMD), change dir where you have downloaded grownode git folder, run `idf.py build`. It will compile for few minutes. 
 - If everything runs well, you should see something like `Project build complete`
 - Plug your ESP32 into your USB port, take note of the COM port you are attached, and run `idf.py -p (PORT) flash`

### Via Eclipse IDE

- Open Eclipse IDE
- Import the project you have downloaded using `Project -> Import -> Existing IDF Project`
- Press Ctrl+B to build the project
- If everything runs well, you should see something like `Project build complete`
- Plug your ESP32 into your USB port, create a new launch target with the COM port of the board, and run using Ctrl + F11



