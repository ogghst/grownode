#include "unity.h"
#include "grownode.h"

gn_config_handle_t config;
gn_node_config_handle_t node_config;

void setUp() {

}

void tearDown() {

}

TEST_CASE("gn_init", "[gn_system]")
{
	config = gn_init();
	TEST_ASSERT(config != NULL);
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
	node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL_STRING("node", gn_get_node_config_name(node_config));
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config),0);
}
