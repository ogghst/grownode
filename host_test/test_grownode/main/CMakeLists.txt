

idf_component_register(
					SRCS 
						"host_test_grownode.c"
						"${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode/gn_leaf_context.c"
#						"${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode/hasht.c"
#						"${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode/lookup3.c"
						"${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode/cc_common.c"
						"${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode/cc_hashtable.c"
						"${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode/cc_array.c"
                    INCLUDE_DIRS
                    "."
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../components/grownode"
        			#"${IDF_PATH}/components/esp_event/include"
                    REQUIRES cmock log)

target_compile_options(${COMPONENT_LIB} PUBLIC --coverage)
target_link_libraries(${COMPONENT_LIB} --coverage)