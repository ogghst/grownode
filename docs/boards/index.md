# Boards

Here you can find resources to make your own board starting from existing works. Code is in 'components\grownode\boards' folder 

##Blink

This is just a test board. Scope is to have your development board LED blinking to show you've done a great job :)

##Easypot1

A good starting point to make sure your pot won't suffer due to lack of attention :)
Goal of this board is to introduce you to GrowNode platform, using super common components you can buy online.

[Board Detail](boards_easypot1.md)

##Hydroboard1

This is the first project done using GrowNode platform. It is basically a Water Tower with water reservoir temperature control (heating/cooling) done through a Peltier cell, and a water level measurement device done by a capacitance sensor.

[Board Detail](boards_hb1.md)

##Hydroboard2

Second attempt to release a Water Tower system. this time I've added some MOSFETs to drive:

- 4 12VDC PWM output (I personally used for water pump, peltier pump, peltier fan, environmental fan
- 4 12VDC Relay output (Lights 1, Lights2, Peltier cell Hot and Cold mode)
- 2 temperature sensors (using DB18B20)
- 1 temperature/humidity/pressure sensor (using BME280)
- 1 capacitive water level sensor

It has an onboard logic to keep the reservoir at desired temperature prior to water it (thus keeping the system on an acceptable temperature range through all the year)

the board is powered with 220V - 12V 10A power adaptor so it can manage quite a lot of power consumption

[Board Detail](boards_hb2.md)