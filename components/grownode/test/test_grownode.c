// Copyright 2021 Nicola Muratori (nicola.muratori@gmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "unity.h"

#include "esp_log.h"

#include "grownode.h"
#include "grownode_intl.h"

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

TEST_CASE("gn_reboot", "[gn_system]")
{
	TEST_ASSERT_EQUAL(ESP_OK, gn_reboot());
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

TEST_CASE("gn_set_blob_string", "[gn_storage]")
{
	char key[] = "test";
	char value[] = "testval";

	esp_err_t ret = gn_storage_set(&key[0], &value[0], sizeof(value));
	TEST_ASSERT_EQUAL(ret, ESP_OK);
}

TEST_CASE("gn_get_blob_string", "[gn_storage]")
{
	char key[] = "test";
	char value[] = "testval";
	char *retval = 0;

	esp_err_t ret = gn_storage_get(&key[0], (void**)&retval);
	TEST_ASSERT_EQUAL(ret, ESP_OK);
	TEST_ASSERT(strcmp(value, retval) == 0);
}


TEST_CASE("gn_set_blob_double", "[gn_storage]")
{
	char key[] = "test";
	double value= 200.2;

	esp_err_t ret = gn_storage_set(&key[0], &value, sizeof(value));
	TEST_ASSERT_EQUAL(ret, ESP_OK);
}

TEST_CASE("gn_get_blob_double", "[gn_storage]")
{
	char key[] = "test";
	double value =  200.2;
	double *retval = 0;

	esp_err_t ret = gn_storage_get(&key[0], (void**)&retval);
	TEST_ASSERT_EQUAL(ret, ESP_OK);
	TEST_ASSERT(*retval == value);
	free(retval);
}



