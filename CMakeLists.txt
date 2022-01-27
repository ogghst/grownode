# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(ENV{COLLECTIONS_C_PATH} "ext/collections-c")

#uncomment if you are using ESP_IDF_LIB sensors. makes the build process slower
set(ENV{IDF_LIB_PATH} "ext/esp-idf-lib/components")

#uncomment if you are using LVGL. makes the build process slower
#set(ENV{LVGL_PATH} "ext/lvgl/lvgl ext/lvgl/lvgl_esp32_drivers")

set(EXTRA_COMPONENT_DIRS $ENV{IDF_LIB_PATH} $ENV{LVGL_PATH} $ENV{COLLECTIONS_C_PATH})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(grownode)