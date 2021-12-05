
<p align="center">
<img src="img/grownode_logo_full.png">
</p>

GrowNode is a vertical framework to build IoT devices targeted to growing plants in a controlled environment. It is currently based on the work of [Nicola Muratori](https://github.com/ogghst). 

# Get Started

It all started with an idea. 

> We're now in an era where technology could help in restoring the connection between urban people, nature and food.

Everyone can have access to resources that were available only to big industries until just 5 years ago. Microcontrollers and hardware components are now available for few dollars, 3D printers can make complex and specialized parts that were impossible to find on general market or hand craft. You can access online to all the information you need to build almost everything. 

And most of us lives in urban areas, we can buy food but we have no way to produce it. And if you can access to some land, you have no time to manage it.

Therefore I started putting all pieces together. 

> It's now possible to build systems capable of growing and harvesting your food at your home, with an home made system capable of automating growing parameters. And it's possible to build them at your home with tools you can buy easily. 

Lots of examples are available online by users who started their projects from scratch. 

And I'm a software engineer used to work with open source technologies. I know the added value that the community work has. My knowledge, the tools I use every day, and ultimately my salary is obtained thanks to people that decided to share their creative work to everyone. 

So maybe it's my turn to return what the community gave to me. 

## First Steps, First Fails

First steps in automating my plants growth were made using circuits using Arduino environment, custom code, recycled parts found in my house, wiring sensors and actuators by hand

<p align="center">
<img src="img/first_steps.jpg">
</p>

After some attempts, (almost) working projects, (many) dead plants and (some) successful harvesting, I've immediately found the limits in this methodology:

 - Every time I needed to change the behavior of one component, or add a new feature, the entire project had to be revised
 - The system has the need to interact with user, to inform over the his status (eg. temperature, water level), to ask for help (eg. board offline, no water)
 - I needed to integrate the system in my existing home automation, to have an unique place where to check and operate over my plants 
 - Once my farming projects increases in number and quality, and my knowledge of this technology advances, I felt the need to add new features and correct bugs in old works

> In short, I needed a workshop to develop my projects with

I started exploring the existing solution, starting from software tools, and found the most promising:

 - [Espressif Rainmaker](https://rainmaker.espressif.com/) - amazing project from ESP32 vendors to focus on custom application code inheriting a lot of prebuilt features - but too vendor-dependent for me: it requires a cloud connection and it is focused in automating well-defined, industrialized home appliance
 - [ESPHome](https://esphome.io/index.html) - quick and easy way to build boards with almost zero code as an Home Assistant add-on - supereasy for very simple projects, but not scalable to develop complex devices and interactions

> After nights of web searches I realized the ugly truth - I needed something different, time to write it from scratch!

## The Idea 

The goals I wanted to reach were:

### Not just a software library, but a vertical platform

Automating plant growing is not only a matter of writing code. You need to design the system, build it, wire everything together, and keep the environment suitable for your plants. Therefore, the project must contain not only code, but schematics, design documents, 3d prints.. An user will need to access and modify all of them. A potential community has to exchange not only new versions of a software library but also new DYI projects, recipes, sugestions. Lot of skills are involved and GrowNode will have to speak a lot of languages.  

### Focus on system architecture, not functionalities

Whoever has to work with this project, has to start with simple things. Custom code written has to be limited to defining what sensors are present in the system, and how they have to interact between each other. Schematics, 3D prints, instructions should be avaialable to give to users the physical building blocks that matches software components 

### Decoupled, scalable system

The environment where GrowNode systems has to operate will be distributed, scalable, etherogeneous. System boundaries can change as we increase the size of our farm, we add features, or entire new systems. A farming project could interact with his environment, e.g. opening a rollershutter to give the plants light.

### If you want to play hard, you can

Simple systems should be ready in minutes, but you will find your own personal needs soon. Each plant has his own need. Every house or garden has his own climate. Technology advance at the speed of lights and every month there is something new to experiment. There's no one size fits all here. Users shall have the possibility to design their own components and architectures, and the GrowNode platform shall provide the access to the raw code and schematics when it's time to. 

