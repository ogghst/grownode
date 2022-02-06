---
hide:
  - navigation
---  

# GrowNode Tutorials

In this section of GrowNode's documentation you will find some tutorials that will make your experience with GrowNode simpler, with step-by-step instructions about different topics.
These are the tutorials now available.

## Getting started

Here you can find step-by-step instructions explaining how to deploy the GrowNode development environment on your workstation, depending on your operating system.

### Windows

- [Windows 10/11 - Easy](windows10_11_easy.md)

	In this tutorial you will install the Eclipse IDE and all the tools required for using GrowNode using the auto-installer provided by the ESP-IDF environment. Super easy!

### Linux

- [Ubuntu 18.04 and 20.04 - Easy](ubuntu1804_2004_easy.md)

	In this tutorial you will install the Eclipse IDE and all the tools required for using GrowNode mostly using a graphical interface.
	> **Note:** this procedure is simpler but installs the very last ESP-IDF version instead of v4.4, which may introduce some incompatibilities with GrowNode in the future. No one has been detected by now.

- [Ubuntu 18.04 and 20.04 - Advanced](ubuntu1804_2004_advanced.md)

	This tutorial explains how to install the basic libraries needed to build GrowNode projects without relying on a particular IDE. It also explains how to install the Eclipse IDE separately, and link it to an existing GrowNode building environment.

## Home automation server

These tutorials show how to install a MQTT broker and the openHAB home automation platform, which can be then used to monitor and control GrowNode remotely (both manually and automatically).

### Linux

- [Ubuntu 18.04 and 20.04 - Easy](openhab_ubuntu1804_2004_easy.md)

	This tutorial explains ho to install Mosquitto and openHAB 3.2 on Ubuntu 18.04 and 20.04 using ready-made packages. Simple as few shell commands!