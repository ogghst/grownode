
# From Ideas to Project

This page aims to give you the basics on the necessary steps to define and design a GrowNode system. If you are already an advanced user.. You're welcome to add your experience! :)

## Design your application

### Define the scope of your project
 
The first step into your journey in a GrowNode implementation is like every other automated farming project: you need to understand what type of system you want to implement. 
 - **what** do you want to grow? from lettuce to strawberries, to herbs, each situation has his own needs and techniques
 - **where** you want to grow? from a greenhouse to a pot into your kitchen, the environment is important and has effects on the design
 - **how** you want to grow? You want a fully automated system? Or just a light that is turning some hours per day?

Choose among the several possible techniques, and what you want to measure and control. 
Use some tutorials if you are a newbie, like [this one](https://www.youtube.com/watch?v=N7HNAD4EfkQ) from YouTube.

![some techniques](https://github.com/ogghst/GrowNode/blob/master/docs/resources/images/techniques.jpg?raw=true)

### Sketch your system architecture

Next step is to define the sensor involved in the system you are designing. 
You want to perform automatic watering? Then you need a water pump. You want to measure water temperature? Then you need a temperature probe. Help yourself with a conceptual schema or visit the [solutions](https://github.com/ogghst/GrowNode/tree/master/docs/resources/solutions/hydroboard1) page already made with GrowNode (now it's just my home project :) ) 

![Example System Architecture](https://github.com/ogghst/GrowNode/blob/master/docs/resources/images/schema3.png?raw=true)

## Create your board
You can now start getting your hands dirty!. But not yet in the soil. Your spades and hoes are now called CAD, PCB and solder.
In this step you will transform your idea in working hardware.

### Wiring diagram
First step is to attach all your sensors and actuators to the ESP32. Follow the datasheets, identify the needed hardware, connect to ESP32 GPIOs, put everything together in a schematic, that could be from a piece of paper to an advanced CAD.

![another schematic](https://github.com/ogghst/GrowNode/blob/master/docs/resources/images/schema5.png?raw=true)

Depending on the type of outcome you want, you need to choose carefully your tool.
 - Prototyping - your target is probably a breadboard, therefore you should focus only on material needed, then you will wire up all together on the fly
 - Pre Production - you will probably create a PCB using online shops or directly at home and solder components by yourself
 - Production - you will design everything with a CAD, identify SMT components, and send everything to a PCB manufacturer like JLCPCB

If you are unfamiliar with this process, visit the [solutions](https://github.com/ogghst/GrowNode/tree/master/docs/resources/solutions/hydroboard1) page with some prebuilt ideas.


### Bill of Material
Now you're ready to shop! List your devices you've included in the schematic and wait for the courier. 
![electrical components](https://github.com/ogghst/GrowNode/blob/master/docs/resources/images/electrical_components.jpg?raw=true)

## Design your Software
This is a process that can be done together with the hardware part described before. This is usually the most complex part, and **this is where GrowNode will help you**!

### Configure your project
 Follow the instructions on [Install](start.md) page in order to have a working environment on your PC.
 After it, you have a preconfigured environment, that has to be personalized for your needs.

 You have two files to start:

`sdkconfig`,  in the main project directory. This is where all the instructions to the compiler resides. This is a standard place to put your settings that will enable specific functionalities. 
You can edit this file using the IDF command `idf.py menuconfig`in your IDF command shell. browse into `component config`, you will find a section named `grownode`:
![sdkconfig](../img/sdkconfig1.png)

Some basic configuration parameters:

 - `reset flash at every startup`: every time the board is started up, all information stored are wiped out. This is useful when you are testing the board and you want to restart every time with a clean situation. please note that this removes also your provisioning status (wifi SSID and password)
 - `enable networking`: in order to have all the network related functionalities. in case of local boards or issues with firmware size this will reduce a lot the firware footprint and enhance performances. Dependant parameters are:
 - `provisioning transport`: how the board will receive wifi credentials at startup. SoftAP means by becoming a local access point, Bluetooth is the other option. Depending on the choice, you will have to use one of the Espressif provisioning apps (at this page)[https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html]
- `keepalive message`: how often the board should publish a status message [see API](html/index.html)
- `provisioning security`: whether provisioning data shall be encrypted and if yes the use of a proof of possession (password) is required
- `SoftAP Prefix`: the name of the temporary network the board will create to ask for wifi credentials
- `firmare URL`: where to search for an updated firmware when the OTA process will start
- `MQTT URL`: where to find the messaging server
- `Base MQTT Topic`: the 'address' of the MQTT messages created by the board and the server
- `Enable MQTT Discovery`: if enabled, once the board is started up it will publish a special message that can be used by controllers like HomeAssistant to autoconfigure baord parameters
- `SNTP Server name`: the address of the time server the board will use to sync his clock
- `Enable Display`: if set, it will start the display driver. the configuration of the display will be taken from `LVGL` component configuration settings 


##Work in Progress

c.	Assemble using prebuilt leaves 
d.	Create your own leaves
4.	Test it in a breadboard (recommended!) 
5.	Wire it up together
6.	Share your project
