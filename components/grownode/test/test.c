#include "unity.h"
#include "grownode.h"
#include "grownode_intl.h"

gn_config_handle_t config;

void setUp() {

}

void tearDown() {

}

TEST_CASE("test_startup", "[grownode]")
{
        // Add test here
	config = gn_init();
	TEST_ASSERT_EQUAL(GN_CONFIG_STATUS_OK, config->status);
	gn_node_config_handle_t node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL(NULL, node_config);

}




