
# Typical Workflow


## Design your application

### Define the scope of your project
 
The first step into your jorney in a GrowNode implementation is like every other automated farming project: you need to understand what type of system you want to implement. 
 - **what** do you want to grow? from lettuce to strawberries, to herbs, every colture has his own needs and techniques
 - **where** you want to grow? from a greenhouse to a pot into your kitchen, the environment is important and has effects on the design
 - **how** you want to grow? you want a fully automated system? or just a light that is turning some hours per day?

Choose among the several possible techniques, and what you want to measure and control. 
Use some tutorials if you are a newbie, like [this one](https://www.youtube.com/watch?v=N7HNAD4EfkQ) from YouTube.

![some techniques](https://github.com/ogghst/GrowNode/blob/master/docs/resources/images/techniques.jpg?raw=true)

### Sketch your system architecture

Next step is to define the sensor involved in the system you are designing. 
You want to perform automatic watering? then you need a water pump. You want to measure water temperature? then you need a temperature probe. Help yourself with a conceptual schema or visit the [solutions](https://github.com/ogghst/GrowNode/tree/master/docs/resources/solutions/hydroboard1) page already made with GrowNode (now it's just my home project :) ) 

![Example System Architecture](https://github.com/ogghst/GrowNode/blob/master/docs/resources/images/schema3.png?raw=true)

## Create your board
You can now start getting your hands dirty!. But not yet in the soil. Your spades and hoes are now called CAD, PCB and solder.
In this step you will transform your idea in working hardware.

### Wiring diagram
First step is to attach all your sensors and actuators to the ESP32. Follow the datasheets instructions, identify the needed hardware, connect to ESP32 GPIOs, put everything together in a schematic, that could be from a piece of paper to an advanced CAD.

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
This is a process that can be done together with the hardware part described before. this is usually the most complex part, and **this is where GrowNode will help you**!

 - Prepare your favourite development environment (I am personally using [eclipse](https://www.eclipse.org/downloads/) with [ESP-IDF plugin](https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md)
 - install ESP-IDF as per [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
 - install ESP_IDF-LIB as per [ESP-IDF Components library](https://github.com/UncleRus/esp-idf-lib)
 - setup main `CMakeLists.txt` file according to your local paths
 - `git clone` this repository
 - add `lvgl` (release/v8) and `lvgl_esp32_drivers` (master) as components as specified [here](https://github.com/lvgl/lv_port_esp32)


b.	Configure your project
c.	Assemble using prebuilt leaves 
d.	Create your own leaves
4.	Test it in a breadboard (recommended!) 
5.	Wire it up together
6.	Share your project
