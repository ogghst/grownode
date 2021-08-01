#include "unity.h"
#include "grownode.h"
#include "esp_log.h"

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

TEST_CASE("gn_node_create", "[gn_system]")
{
	node_config = gn_node_create(config, "node");
	TEST_ASSERT_EQUAL_STRING("node", gn_get_node_config_name(node_config));
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config),0);
}

TEST_CASE("gn_start", "[gn_system]")
{
esp_err_t ret = gn_node_start(node_config);
		TEST_ASSERT_EQUAL(ret, ESP_OK);
}

TEST_CASE("gn_loop_1s", "[gn_system]")
{
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	TEST_ASSERT(true);
}
