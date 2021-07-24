#include "unity.h"
#include "grownode.h"
#include "grownode_intl.h"

gn_config_handle_t config;

void setUp() {

}

void tearDown() {

}


TEST_CASE("gn_init", "[grownode]")
{
        // Add test here
	config = gn_init();
	TEST_ASSERT_EQUAL(GN_CONFIG_STATUS_OK, config->status);
}


TEST_CASE("gn_firmware_update", "[gn_system]")
{
	TEST_ASSERT_EQUAL(ESP_OK, gn_firmware_update());
}

TEST_CASE("gn_reset", "[gn_system]")
{
	TEST_ASSERT_EQUAL(ESP_OK, gn_reset());
}

TEST_CASE("gn_node_create", "[grownode]")
{
	gn_node_config_handle_t node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL_STRING("node", gn_get_node_config_name(node_config));
}




