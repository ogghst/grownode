# Getting Started

This page describe the steps needed to have a working development environment on your PC. 

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

