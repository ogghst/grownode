---
hide:
  - navigation
---  
  
<p align="center">
<img src="img/grownode_logo_full.png">
</p>

# Abstract

> GrowNode is a vertical framework to build esp32 based devices targeted to growing plants in a controlled environment. It is currently based on the work of [Nicola Muratori](https://github.com/ogghst) and [Adamo Ferro](https://github.com/adamoferro). 

## Target

Grownode is targeted for whoever has the need to build an automatic growing system, from DIY home growing to more advanced farming solutions.

## How GrowNode can help you

This platform will facilitate the hardware and software design and implementation phase, by giving you a ready made set of functionalities, materials and knowledge base. 

## Skills required

In order to startup a basic prebuilt system, you just need to follow the instructions in [boards](boards.md) section.

If you want to play hard and develop your own customized system, a basic knowledge of C programming, ESP-IDF development environment, and electrical engineering is required.

## Examples

### Simple notification system

You can use GrowNode to build a system that warns you when humidity and temperature on a plant is outside determined range, by blinking a LED:

![simple configuration](resources/images/easypot1_simple.png)

### Network connected system

You can then add network connectivity to a server and have your board warns you on your PC or mobile phone, and change the system parameters via a simple interface.

![network configuration](resources/images/easypot1_network.png)

### Environment controlled system

GrowNode has a growing list of supported sensors and actuators, in order to build a more sophisticated system. You can add sensors, motors, lights, pumps, everything needed to build your own solution customized to your needs.

![network controlled configuration](resources/images/easypot1_network_full.png)

### Distributed, orchestrated system

Grownode is born having in mind IoT distributed environments. You can add nodes to your system and those nodes will easily talk between each other, acting as a unique orchestrated platform, with no limits over its scalability.

![orchestration](resources/images/easypot1_orchestration.png)

## Technologies

GrowNode works with most common technologies, relying on minimal custom code in order to obtain the most reliable platform features, benefits from ESP32 core and open source development effort. 

It supports HOMIE protocol for autodiscovery and standard MQTT messaging

<a href="https://homieiot.github.io/">
  <img src="https://homieiot.github.io/img/works-with-homie.png" alt="works with MQTT Homie">
</a>

