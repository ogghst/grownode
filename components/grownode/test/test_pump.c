#include "unity.h"
#include "grownode.h"
#include "grownode_intl.h"
#include "gn_pump.h"
#include "esp_log.h"

extern gn_config_handle_t config;
extern gn_node_config_handle_t node_config;

gn_leaf_config_handle_t pump_config;


TEST_CASE("gn_leaf_create pump", "[pump]")
{
pump_config = gn_leaf_create(node_config, "pump",
	gn_pump_task, gn_pump_display_config, 4096);

TEST_ASSERT_EQUAL(gn_node_get_size(node_config),1);

gn_leaf_config_handle_intl_t leaf = (gn_leaf_config_handle_intl_t)pump_config;

TEST_ASSERT_EQUAL(gn_node_get_size(node_config),1);


}

TEST_CASE("gn pump_send_on", "[pump]")
{

gn_leaf_event_t evt;

evt.id = GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT;
strncpy(evt.leaf_name, "pump", 5);
strncpy(evt.param_name, "status", 7);
char* data = "true";
evt.data = data;
evt.data_size = 5;

esp_err_t ret = gn_event_send_internal(config, &evt);
TEST_ASSERT_EQUAL(ret, ESP_OK);

}

TEST_CASE("gn pump_send_off", "[pump]")
{

gn_leaf_event_t evt;

evt.id = GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT;
strncpy(evt.leaf_name, "pump", 5);
strncpy(evt.param_name, "status", 7);
char* data = "false";
evt.data = data;
evt.data_size = 6;

esp_err_t ret = gn_event_send_internal(config, &evt);
TEST_ASSERT_EQUAL(ret, ESP_OK);

}

TEST_CASE("gn pump_send_pwm_500", "[pump]")
{

gn_leaf_event_t evt;

evt.id = GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT;
strncpy(evt.leaf_name, "pump", 5);
strncpy(evt.param_name, "power", 7);
char* data = "500";
evt.data = data;
evt.data_size = 4;

esp_err_t ret = gn_event_send_internal(config, &evt);
TEST_ASSERT_EQUAL(ret, ESP_OK);

}

TEST_CASE("gn pump_send_pwm_0", "[pump]")
{

gn_leaf_event_t evt;

evt.id = GN_LEAF_PARAM_MESSAGE_RECEIVED_EVENT;
strncpy(evt.leaf_name, "pump", 5);
strncpy(evt.param_name, "power", 7);
char* data = "0";
evt.data = data;
evt.data_size = 4;

esp_err_t ret = gn_event_send_internal(config, &evt);
TEST_ASSERT_EQUAL(ret, ESP_OK);

}
