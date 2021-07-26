#include "unity.h"
#include "grownode.h"
#include "gn_pump.h"

extern gn_config_handle_t config;
extern gn_node_config_handle_t node_config;

TEST_CASE("gn_leaf_create pump", "[pump]")
{
	gn_leaf_config_handle_t pump_config = gn_leaf_create(node_config, "pump",
			gn_pump_task, gn_pump_display_config);
	TEST_ASSERT_EQUAL(gn_node_get_size(node_config),1);
}

TEST_CASE("gn pump_send_on", "[pump]")
{

	gn_leaf_event_t evt;

	evt.id = GN_LEAF_MESSAGE_RECEIVED_EVENT;
	strncpy(evt.leaf_name, "pump", 5);
	char* data = "start";
	evt.data = data;
	evt.data_size = 6;

	//gn_leaf_param_set_bool(leaf_config, "status",
	//true);

	esp_err_t ret = gn_event_send_internal(config, &evt);
	TEST_ASSERT_EQUAL(ret, ESP_OK);

}
