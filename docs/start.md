# Getting Started

This page describe the steps needed to have a working development environment on your PC. 

## Setup your environment

### IDE

 - Prepare your favorite development environment (I am personally using [eclipse](https://www.eclipse.org/downloads/) with [ESP-IDF plugin](https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md)

### ESP-IDF

 - install ESP-IDF as per [ESP-IDF getting started guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
 - install ESP_IDF-LIB as per [ESP-IDF Components library](https://github.com/UncleRus/esp-idf-lib) in the same root folder of ESP-IDF
 - you should have then a folder structure like:
>     root folder
>     +--esp-idf-master
>     +--esp-idf-lib-master


## Obtain GrowNode


 - Clone GrowNode repository using command `git clone --recurse-submodules https://github.com/ogghst/grownode.git`
 - setup `CMakeLists.txt` file according to your local paths
 - add `lvgl` (release/v8) and `lvgl_esp32_drivers` (master) as components as specified [here](https://github.com/lvgl/lv_port_esp32)

