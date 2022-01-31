# How to get started with Grownode on Ubuntu 18.04 and 20.04 - Easy version

> Updated on January 18, 2022

## Index

- [Introduction](#introduction)
	* [Prerequisites](#prerequisites)
- [Step 1: Prepare your system](#step-1-prepare-your-system)
- [Step 2: Installing Eclipse IDE for C/C++ Developers 2021-09](#step-2-installing-eclipse-ide-for-cc-developers-2021-09)
- [Step 3: Installing the ESP-IDF plugin for Eclipse](#step-3-installing-the-esp-idf-plugin-for-eclipse)
- [Step 4: Download the ESP-IDF environment from Eclipse](#step-4-download-the-esp-idf-environment-from-eclipse)
- [Step 5: Download Grownode and build it](#step-5-download-grownode-and-build-it)
- [Step 6: Flash your board from Eclipse](#step-6-flash-your-board-from-eclipse)
- [Step 7: Monitor the serial output of your board from Eclipse](#step-7-monitor-the-serial-output-of-your-board-from-eclipse)
- [Appendix](#appendix)
	* [A. How to get the name of the serial device associated to the ESP32 board](#a-how-to-get-the-name-of-the-serial-device-associated-to-the-esp32-board)
		+ [If you are NOT working inside a virtual machine](#if-you-are-not-working-inside-a-virtual-machine)
		+ [If you are working inside a virtual machine](#if-you-are-working-inside-a-virtual-machine)
	* [B. Installing Grownode into an Ubuntu virtual machine using VirtualBox](#b-installing-grownode-into-an-ubuntu-virtual-machine-using-virtualbox)
		+ [Properly link the host and guest serial ports](#properly-link-the-host-and-guest-serial-ports)

## Introduction

With this step-by-step tutorial you will be able to quickly deploy the Eclipse IDE with the Grownode development environment on your Ubuntu 18.04 or 20.04 workstation.

At the end of the tutorial you will have:
- installed the Eclipse IDE
- installed the ESP-IDF plugin for Eclipse
- installed the main packages required to Grownode
- installed the latest ESP-IDF environment
- compiled and run your first Grownode test board via Eclipse
 
> *Notes:*
> - *The same steps should also apply for Ubuntu 18.10 and 20.10, but we haven't tried yet. Let us know if you did!*
> - *Older versions of Ubuntu do not satisfy the minimal requirements out-of-the-box, so they are not officially supported by Grownode.*
> - *This procedure could be also used to create an Ubuntu virtual machine to play with Grownode without affecting your system. Read more details on this in the Appendix below.*

### Prerequisites

- at least 5.5 Gb free space on hard drive 
- at least 4 Gb RAM
- an ESP32 board with USB cable
 
The ESP32 board could be a ready-to-use one (like the Wemos D1 R32), or other more compact ones that need some soldering (like the ESP32-DevKitC V4 and D1 Mini ESP32).

## Step 1: Prepare your system

Before starting installing any software, you need to prepare your system downloading some basic packages.

Open a terminal and type this commands:

```
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
mkdir -p ~/esp
```

Then, you can make your serial devices always accessible with:

```
sudo usermod -a -G dialout $USER 
```

The latter command becomes effective only once you re-login to your workstation. So, it's better if do a logout/login procedure (or a restart) now to avoid forgetting it :)

## Step 2: Installing Eclipse IDE for C/C++ Developers 2021-09

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
 
- at the first start, Eclipse will ask for the default workspace folder. Click on "Browse...", select the folder called `esp` inside your home, click on "Open" and then on "Launch"
> you may also select "Use this as the default..." if you are going to use Eclipse only for Grownode

## Step 3: Installing the ESP-IDF plugin for Eclipse

Once Eclipse is launched, do as follows to install the ESP-IDF plugin:

- close the "Welcome" tab
- select "Install New Software" from the "Help" menu
- click on "Add...", a new window opens
- enter:
	- Name: Espressif IDF Plugin for Eclipse
	- Location: https://dl.espressif.com/dl/idf-eclipse-plugin/updates/latest/
- click on "Add"
- select the whole "Espressif IDF" tree, as in figure, and then click on "Next >"

<p align="center"><img alt="Download Espressif IDF Plugin for Eclipse" src="img/eclipse_install_esp-idf_plugin.png"></p>

- click again on "Next >"
- select "I accept the terms..." and then click on "Finish"
- if a warning about unsigned packages appears, click on "Install anyway"
- at the end of the process click on "Restart Now"
- if a sort of error window appears, close it and re-launch Eclipse manually

## Step 4: Download the ESP-IDF environment from Eclipse

Now you are ready to download the ESP-IDF environment from Eclipse:

- select "Download and Configure ESP-IDF" in the "Espressif" menu
- in the "Download ESP-IDF" box select the "master" version and the folder `esp` you created before, as in figure

<p align="center"><img alt="Install ESP-IDF from Eclipse" src="../img/eclipse_install_esp-idf.png"></p>

- click on "Finish"
- in the Console area you will see the status of ESP-IDF download
- when the download is ended, click on "Yes" in the pop-up window asking to install some new tools
- in the popup window fill the fields as in figure
	- ESP-IDF Directory: this field should be already filled with the right path
	- Git Executable Location: `/usr/bin/git`
	- Python Executable Location: `/usr/bin/python3`
	
<p align="center"><img alt="Install ESP-IDF tools in Eclipse" src="../img/eclipse_esp-idf_tools.png"></p>

- click on "Install Tools" and monitor the installation in the status bar at the bottom right

## Step 5: Download Grownode and build it

Once the tool installation of the previous step is finished, import your first Grownode project doing:

- select "Import..." from the "File" menu
- select "Git" > "Projects from Git" from the list and click on "Next >"
- select "Clone URI" from the list and click on "Next >"
- fill the field "URI" with `https://github.com/ogghst/grownode.git` and the remaining fields will auto-complete, as in figure, and click on "Next >"

<p align="center"><img alt="Grownode GIT clone URI in Eclipse" src="../img/eclipse_grownode_git_clone_uri.png"></p>

- unless you are interested in other branches, select only the "master" branch, as in figure, and then click on "Next >" (with smart import)

<p align="center"><img alt="Grownode GIT master clone in Eclipse" src="../img/eclipse_grownode_git_clone_master.png"></p>

- change the "Directory" field setting a `grownode` folder inside the `esp` folder you created before and flag the "Clone submodules" option, as in figure; click on "Next >"

<p align="center"><img alt="Grownode GIT folder clone in Eclipse" src="../img/eclipse_grownode_git_clone_folder.png"></p>

- click on "Next >" in the following window, leaving unchanged the selection of "Import existing Eclipse projects"

- unless you are interested in other projects, select only the `grownode` project from the list, as in figure, and click on "Finish"

<p align="center"><img alt="Grownode GIT project clone in Eclipse" src="../img/eclipse_grownode_git_clone_project.png"></p>

...and here you go! Grownode is imported into Eclipse. Well done!
In the Project Explorer on the left you can see the Grownode workspace:

<p align="center"><img alt="Grownode Project explorer in Eclipse" src="../img/eclipse_grownode_project_explorer.png"></p>

Try your first build by clicking on the hammer icon at the top left, or typing `CTRL+B`.
In the console at the bottom you should see your project compiling and ending with the message `Build complete (0 errors...`. That means it was successful. Great!

## Step 6: Flash your board from Eclipse

Before flashing you must complete some configuration steps:

- click on the setting gear beside "esp32" in the top icon bar (see point 1 in figure)

<p align="center"><../img alt="ESP32 serial port in Eclipse" src="img/eclipse_flashing_settings.png"></p>

- select the right "Serial Port" from the list (the one you identified while installing ESP-IDF, most probably `/dev/ttyUSB0`) and click on "Finish"

Now plug your board into the USB port and click on the green play icon on the top left, or type `CTRL+F11`. Eclipse will start the flashing procedure (it's possible it will re-build the project before) and if everything goes well you should see the following message:

```
Leaving...
Hard resetting via RTS pin...
Executing action: flash
Running ninja in directory /home/your-username-here/esp/grownode/build
Executing "ninja flash"...
```

Did you?! Yes? Perfect! Congratulations!

If the answer is no, probably you need to manually set the serial port speed and re-trigger the flashing procedure:

- click on the setting gear beside "grownode" in the top icon bar (see point 2 in the figure above)
- in the "Arguments" section add the following bold parameter exactly in the same position:

	`idf.py` **`-b 115200`** `-p /dev/your-serial-device-id flash`

## Step 7: Monitor the serial output of your board from Eclipse

To start the ESP-IDF monitor click on the icon "Open a Terminal" in the top icon bar (see point 1 in figure below). Check the correctness of the serial port and then click on "Ok".
The board will restart you you will see the "blinking" messages.

To stop the monitor, just close the "Terminal" window or click on the red "Disconnect Terminal Connection" at the top right of the terminal itself (point 2 in figure below).

<p align="center"><img alt="Monitor ESP-IDF from Eclipse" src="../img/eclipse_monitor.png"></p>

Great job! You are ready to play more with Grownode ;)

## Appendix

### A. How to get the name of the serial device associated to the ESP32 board

The procedure differes depending on if you are working or not inside a virtual machine. Let's see the two cases.

#### If you are NOT working inside a virtual machine

In this case, the host serial device associated to the ESP32 board is usually called `/dev/ttyUSB0`.
If you want to check the actual name, follow these steps:

1. open a terminal
2. plug the USB cable
3. type the command `dmesg`
4. a line with a text similar to the following should appear at the end of the log, indicating the name of your new serial device in place of `your-serial-device-id`:

	`usb 1-1: ch341-uart converter now attached to`**`your-serial-device-id`**

5. use the name `/dev/your-serial-device-id` when flashing your board or when looking at debug messages through a serial monitor (i.e., in steps 4 and 5 replace `/dev/ttyUSB0` with the `/dev/your-serial-device-id` you discovered here)

#### If you are working inside a virtual machine

In this case the serial device depends on the way you set up the virtual machine.
If you are using VirtualBox and you followed the instructions below, your serial device will depend on the COM you selected as "Host Device":
- `COM1` corresponds to `/dev/ttyS0`
- `COM2` corresponds to `/dev/ttyS1`
- ...and so on.

### B. Installing Grownode into an Ubuntu virtual machine using VirtualBox

If you wish to create a new Ubuntu virtual machine (VM) from scratch using VirtualBox just to have a try of Grownode without affecting your system, you can still install the software environment inside the VM following the instructions of this page.

> How to create an Ubuntu VM using VirtualBox is out of scope here. You can find many guides about this topic on the web.

When working inside a VM you have to be aware of the following issues/requirements in order to be able to flash and properly work with your board:
- you must link the host serial device to a VM serial device
- you must plug the board via USB **BEFORE** starting the VM
- you should never unplug the board while the VM is running
- you will probably need to manually set the flashing baud rate to 115200 baud (as explained at the end of Step 4 above)

#### Properly link the host and guest serial ports

In VirtualBox, open the "Serial Ports" tab of the VM settings, shown in figure.

<p align="center"><img alt="Serial Ports tab in VirtualBox" src="../img/appendix_b_virtualbox_serial.png"></p>

To properly set the serial communication, you have to:
- flag "Enable Serial Port"
- select the "Port Number" from the list (`COM1` will be ok in most cases)
- select "Host Device" in "Port Mode"
- write the name of the host serial port into the "Path/Address" field
	> Follow the instructions of the previous section *"If you are NOT working inside a virtual machine"* to identify it

