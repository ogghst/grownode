

idf_component_register(SRCS "host_test_grownode.c"
                    INCLUDE_DIRS
                    "."

                    "${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../."
                    REQUIRES cmock grownode log)

target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
target_link_libraries(${COMPONENT_LIB} --coverage)