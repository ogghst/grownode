cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components)
#set(EXTRA_COMPONENT_DIRS ../../components)
#idf_component_set_property(spi_flash USE_MOCK 1)
#idf_build_set_property(USE_MOCK 1)
set(COMPONENTS main)

#idf_component_set_property(spi_flash USE_MOCK 1)
idf_build_set_property(COMPILE_DEFINITIONS "-DNO_DEBUG_STORAGE" APPEND)
project(grownode_host_test)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/build/coverage.info"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    COMMAND lcov --capture --directory . --output-file coverage.info
    COMMENT "Create coverage report"
    )

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/build/coverage_report/"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build/coverage.info"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    COMMAND genhtml coverage.info --output-directory coverage_report/
    COMMENT "Turn coverage report into html-based visualization"
    )

add_custom_target(coverage
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    DEPENDS "coverage_report/"
    )