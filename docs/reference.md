# Reference Guide

This page aims to describe the GrowNode API and how to use it to develop your own firmware.

GrowNode is developed on top of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) Development Framework, in order to directly access to the ESP32 microprocessor functionalities and the RTOS operating system.

## Basic Concepts

THe highest level structure on a GrowNode system is the board itself. Every solution you want to build is basically a combination of 

- Devices attached to the IO pins (tipycally handled by specific HAL - Hardware Abstraction Layers - like ESP-IDF core libraries or third party devices. 

> Example: I2C driver, a GPIO pin

- several logics to access to those devices and present to the GrowNode framwork [Leaves](#leaves) - several are prebuilt and more can be user defined. 

> Example: a temperature sensor, a relay, a LED, a motor

- each leaf exposes several [Parameters](#Parameters) able to retrieve or command specific functionalities of the leaf. 

> Example: the temperature retrieved, a motor switch, a light power

- one centralized controller that orchestrates and exposes to the Leaves the services needed to work: [Node](#node). This is common on all the GrowNode implementation

- one entry point of the application, where the Node and Leaves are declared and configured [Board](#boards). 

> Example: a Water Tower Board, a simple Temperature and Humidity pot controller

###Code reference

Code Documentation is described in [API](../html/index.html) section. The entry point of all GrowNode functionalities resides in `grownode.h` header file. Users just have to reference it in their code. 



##Node

The core element of a GrowNode implementation is the Node. 

### Responsibilites
todo

### Configuration
todo

### Examples
todo


##Leaves

### Responsibilites
todo

### Configuration
todo

### Examples
todo

##Parameters

### Responsibilites
todo

### Configuration
todo

### Examples
todo

##Boards
todo

##MQTT Protocol

todo 

### Leaf - Server messaging

todo

### Leaf - leaf messaging

todo


## Display Configuration

todo

