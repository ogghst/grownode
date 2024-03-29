# How to get started with GrowNode on Ubuntu 18.04 and 20.04 - Advanced version

> Updated on January 18, 2022

## Introduction

With this step-by-step tutorial you will be able to quickly deploy the GrowNode development environment on your Ubuntu 18.04 or 20.04 workstation.

At the end of the tutorial you will have:

- installed the main packages required to GrowNode
- installed the ESP-IDF environment v4.4
- compiled and run your first GrowNode test board
 
In addition, the following optional steps are also explained:

- installing the Eclipse IDE
- installing the ESP-IDF plugin for Eclipse
- compiling and running your first GrowNode test board via Eclipse

> *Notes:*
>
> - *The same steps should also apply for Ubuntu 18.10 and 20.10, but we haven't tried yet. Let us know if you did!*
> - *Older versions of Ubuntu do not satisfy the minimal requirements out-of-the-box, so they are not officially supported by GrowNode.*
> - *This procedure could be also used to create an Ubuntu virtual machine to play with GrowNode without affecting your system. Read more details on this in the Appendix below.*

### Prerequisites

- at least 2.5 Gb free space on hard drive (additional 3 Gb required for Eclipse and related sofrware)
- at least 4 Gb RAM
- an ESP32 board with USB cable
 
The ESP32 board could be a ready-to-use one (like the Wemos D1 R32), or other more compact ones that need some soldering (like the ESP32-DevKitC V4 and D1 Mini ESP32).

## Installing the GrowNode software environment

The following steps are mandatory in order to correctly deploy all the software libraries and basic tools for developing a GrowNode board. Just follow the instructions: it will be easy!

### Step 1: Package installation and system preparation

Open a terminal and use the following command to install the main packages required by ESP-IDF:

```
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

Then, you can make your serial devices always accessible with:

```
sudo usermod -a -G dialout $USER 
```

The latter command becomes effective only once you re-login to your workstation. So, it's better if do a logout/login procedure (or a restart) now to avoid forgetting it :)

### Step 2: Install the ESP-IDF environment

GrowNode needs the environment called ESP-IDF v4.4 to compile the firmware board. Type the following commands to download it in your home and install it in your system:

```
mkdir -p ~/esp
cd ~/esp
git clone -b release/v4.4 --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
./install.sh esp32
echo -e "\nalias get_idf='. $HOME/esp/esp-idf/export.sh'" >> ~/.bashrc
source ~/.bashrc
```

Congratulations! You have just installed the base environment that will allow you to build your GrowNode boards!
Now you are ready to download and build your first GrowNode project.

### Step 3: Download the GrowNode source and build the firmware of your first board

The following commands allow you to download the current version of GrowNode in your home ESP directory:

```
cd ~/esp
git clone --recurse-submodules https://github.com/ogghst/grownode.git
```

It's time for your first build! The first step is setting up the ESP-IDF environment variables.
The command is:

```
get_idf
```

> **IMPORTANT:** this operation must be done every time you open a new terminal/console to use ESP-IDF commands, and applies only to that particular console.

Now move to GrowNode's folder and build the default project:

```
cd ~/esp/grownode
idf.py build
```

The compiler works for a while, and at the end of the process there will be the message `Project build complete`.
If you see a different message... something went wrong. Be sure you followed all the steps above!

### Step 4: Upload (flash) the firmware on the board

After building, plug the ESP32 board to your workstation with an USB cable and detect the name of the new serial port associated to the board. Most probably it is called `/dev/ttyUSB0`.

> *If you don't know how to get the name of the new serial port, have a look at the Appendix below*

Finally, flash your board with the command:

```
cd ~/esp/grownode
idf.py -p /dev/ttyUSB0 flash
```
*(replace the port name *`/dev/ttyUSB0`* with the one used on your system, if different)*

Your flash should be fine if at the end of the procedure you see something similar to this:

```
Writing at 0x0000d000... (100 %)
Wrote 8192 bytes (31 compressed) at 0x0000d000 in 0.1 seconds (effective 729.6 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
Done
```

Did you? Great! Have a look at your board: one led is blinking! Congratulations!

If you didn't see the above messages and no led is blinking, something went wrong. Assuming you are using the right serial port name, in the [official ESP-IDF guide](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/get-started/index.html#step-9-flash-onto-the-device) you can find some solutions.

A very common problem is serial baud rate, especially if you are working in a virtual machine using VirtualBox. Try to flash using the following command to slow down serial communication:

```
idf.py -p /dev/ttyUSB0 -b 115200 flash
```
*(again, replace the port name with yours)*

### Step 5: Monitor the serial output of your board

It is possible to read the debug messages sent by the board through the serial port using the following command:

```
idf.py -p /dev/ttyUSB0 monitor
```
*(again, replace the port name with yours)*

If build and flashing were ok, you should see the board continuosly saying something like:

```
I (5602) gn_blink: blinking - 0
I (10602) gn_blink: blinking - 1
```

Type `CTRL+]` to exit from the monitor mode.



### Appendix

#### A. How to get the name of the serial device associated to the ESP32 board

The procedure differes depending on if you are working or not inside a virtual machine. Let's see the two cases.

##### If you are NOT working inside a virtual machine

In this case, the host serial device associated to the ESP32 board is usually called `/dev/ttyUSB0`.
If you want to check the actual name, follow these steps:

1. open a terminal
2. plug the USB cable
3. type the command `dmesg`
4. a line with a text similar to the following should appear at the end of the log, indicating the name of your new serial device in place of `your-serial-device-id`:

`usb 1-1: ch341-uart converter now attached to`**`your-serial-device-id`**

5. use the name `/dev/your-serial-device-id` when flashing your board or when looking at debug messages through a serial monitor (i.e., in steps 4 and 5 replace `/dev/ttyUSB0` with the `/dev/your-serial-device-id` you discovered here)

##### If you are working inside a virtual machine

In this case the serial device depends on the way you set up the virtual machine.
If you are using VirtualBox and you followed the instructions below, your serial device will depend on the COM you selected as "Host Device":

- `COM1` corresponds to `/dev/ttyS0`
- `COM2` corresponds to `/dev/ttyS1`
- ...and so on.

#### B. Installing GrowNode into an Ubuntu virtual machine using VirtualBox

If you wish to create a new Ubuntu virtual machine (VM) from scratch using VirtualBox just to have a try of GrowNode without affecting your system, you can still install the software environment inside the VM following the instructions of this page.

> How to create an Ubuntu VM using VirtualBox is out of scope here. You can find many guides about this topic on the web.

When working inside a VM you have to be aware of the following issues/requirements in order to be able to flash and properly work with your board:

- you must link the host serial device to a VM serial device
- you must plug the board via USB **BEFORE** starting the VM
- you should never unplug the board while the VM is running
- you will probably need to manually set the flashing baud rate to 115200 baud (as explained at the end of Step 4 above)

##### Properly link the host and guest serial ports

In VirtualBox, open the "Serial Ports" tab of the VM settings, shown in figure.

<p align="center"><img alt="Serial Ports tab in VirtualBox" src="../img/appendix_b_virtualbox_serial.png"></p>

To properly set the serial communication, you have to:

- flag "Enable Serial Port"
- select the "Port Number" from the list (`COM1` will be ok in most cases)
- select "Host Device" in "Port Mode"
- write the name of the host serial port into the "Path/Address" field
	> Follow the instructions of the previous section *"If you are NOT working inside a virtual machine"* to identify it

## Installing the Eclipse IDE with the ESP-IDF plugin

Eclipse and the ESP-IDF plugin have several requirements which allow one to use only certain versions of softwares and libraries. You have not to worry about this, as this tutorial already makes you install the right things.

### Step 1: Installing Eclipse IDE for C/C++ Developers 2021-09

This is the latest version officially supported by the plugin, and you can get it following these steps:

- download it from [this page](https://www.eclipse.org/downloads/packages/release/2021-09/r/eclipse-ide-cc-developers) selecting the **Linux** package that corresponds to your architecture (most probably it is the one called *x86_64*) in the "Download Links" column
- open a terminal and move to the folder where you downloaded the tar.gz file (usually `~/Downloads`)
- use the following commands to extract Eclipse from the compressed file and make it available for use in the whole system:
 
```
sudo tar xf eclipse-cpp-2021-09-R-linux-gtk-x86_64.tar.gz -C /opt
sudo ln -s /opt/eclipse/eclipse /usr/local/bin/
```
	
- *(optional)* if you want to add a desktop icon to avoid launching Eclipse from terminal you need to create a shortcut:
	- open a terminal and launch:
`sudo nano /usr/share/applications/eclipse.desktop`

	- paste the following text into the new empty file and press `CTLR+x` to save it:

```
[Desktop Entry]
Version = 2021-09
Type = Application
Terminal = false
Name = Eclipse C/C++
Exec = /usr/local/bin/eclipse
Icon = /opt/eclipse/icon.xpm
Categories = Application;
```
 
- run Eclipse using the command `eclipse` from a terminal, or by using the Eclipse icon in your desktop (if you created it)
 
- at the first start, Eclipse will ask for the default workspace folder. If you followed these instructions, select the `esp` folder inside your home and then click on "Launch"
> you may also select "Use this as the default..." if you are going to use Eclipse only for GrowNode

### Step 2: Installing the ESP-IDF plugin for Eclipse

Once Eclipse is launched, do as follows to install the ESP-IDF plugin:

- select "Install New Software" from the "Help" menu
- click on "Add...", a new window opens
- enter:
	- Name: Espressif IDF Plugin for Eclipse
	- Location: https://dl.espressif.com/dl/idf-eclipse-plugin/updates/latest/
- click on "Add"
- select the whole "Espressif IDF" tree, as in figure, and then click on "Next >"

<p align="center"><img alt="Download Espressif IDF Plugin for Eclipse" src="../img/eclipse_install_esp-idf_plugin.png"></p>

- click again on "Next >"
- select "I accept the terms..." and then click on "Finish"
- if a warning about unsigned packages appears, click on "Install anyway"
- at the end of the process click on "Restart Now"
- if a sort of error window appears, close it and re-launch Eclipse manually

### Step 3: Configure Eclipse for using the local ESP-IDF environment

Now you are ready to link the ESP-IDF environment you installed at the beginning of this tutorial to Eclipse:

- select "Download and Configure ESP-IDF" in the "Espressif" menu
- check "Use an existing ESP-IDF directory from the file system"
- choose the `esp-idf` folder you created at the beginning of this tutorial, as in figure

<p align="center"><img alt="Link existing ESP-IDF environment to Eclipse" src="../img/eclipse_link_esp-idf_environment.png"></p>

- click on "Finish"
- click on "Yes" in the pop-up window asking to install some new tools
- in the popup window fill the fields as in figure
	- ESP-IDF Directory: this field should be already filled with the right path
	- Git Executable Location: `/usr/bin/git`
	- Python Executable Location: `/usr/bin/python3`
	
<p align="center"><img alt="Install ESP-IDF tools in Eclipse" src="../img/eclipse_esp-idf_tools.png"></p>

- click on "Install Tools" and monitor the installation in the status bar at the bottom right

### Step 4: Import your first GrowNode project and build it

Once the tool installation of the previous step is finished, import your first GrowNode project doing:

- close the "Welcome" tab
- select "Import..." from the "File" menu
- select "Espressif" > "Existing IDF Project" from the list
- click on "Browse..." and select the `grownode` folder. The field "Project Name" will auto-fill with "grownode"
- click on "Finish"

...and here you go! GrowNode is imported into Eclipse. Well done!
In the Project Explorer on the left you can see the GrowNode workspace:

<p align="center"><img alt="GrowNode Project explorer in Eclipse" src="../img/eclipse_grownode_project_explorer.png"></p>

Try your first build by clicking on the hammer icon at the top left, or typing `CTRL+B`.
In the console at the bottom you should see your project compiling and ending with the message `Build complete (0 errors...`. That means it was successful. Great!

### Step 5: Flash your board from Eclipse

Before flashing you must complete some configuration steps:

- click on the setting gear beside "esp32" in the top icon bar (see point 1 in figure)

<p align="center"><img alt="ESP32 serial port in Eclipse" src="../img/eclipse_flashing_settings.png"></p>

- select the right "Serial Port" from the list (the one you identified while installing ESP-IDF, most probably `/dev/ttyUSB0`) and click on "Finish"

Now plug your board into the USB port and click on the green play icon on the top left, or type `CTRL+F11`. Eclipse will start the flashing procedure (it's possible it will re-build the project before) and if everything goes well you should see the following message:

```
Leaving...
Hard resetting via RTS pin...
Executing action: flash
Running ninja in directory /home/your-username-here/esp/grownode/build
Executing "ninja flash"...
Done
```

Did you?! Yes? Perfect! Congratulations!

If the answer is no, probably you need to manually set the serial port speed and re-trigger the flashing procedure:

- click on the setting gear beside "grownode" in the top icon bar (see point 2 in the figure above)
- in the "Arguments" section add the following bold parameter exactly in the same position:

`idf.py` **`-b 115200`** `-p /dev/your-serial-device-id flash`

### Step 6: Monitor the serial output of your board from Eclipse

To start the ESP-IDF monitor click on the icon "Open a Terminal" in the top icon bar (see point 1 in figure below). Check the correctness of the serial port and then click on "Ok".
The board will restart you you will see the "blinking" messages.

To stop the monitor, just close the "Terminal" window or click on the red "Disconnect Terminal Connection" at the top right of the terminal itself (point 2 in figure below).

<p align="center"><img alt="Monitor ESP-IDF from Eclipse" src="../img/eclipse_monitor.png"></p>

Great job! You are ready to play more with GrowNode ;)
