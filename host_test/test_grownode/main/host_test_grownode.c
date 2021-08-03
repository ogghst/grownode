/* test_mean.c: Implementation of a testable component.

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <limits.h>
#include <string.h>

#include "unity.h"
#include "gn_leaf_context.h"

#include "esp_log.h"

static const char *TAG = "host_test_grownode";

gn_leaf_context_handle_t context;

void test_gn_leaf_context_create() {

	context = gn_leaf_context_create(2);

	TEST_ASSERT(context != 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 0);

	gn_leaf_context_print(context);
	gn_leaf_context_destroy(context);

}

void test_gn_leaf_context_destroy() {

	context = gn_leaf_context_create(2);

	TEST_ASSERT(context != 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 0);

	gn_leaf_context_print(context);
	gn_leaf_context_destroy(context);

}

void test_gn_leaf_context_get_key_at_0() {

	context = gn_leaf_context_create(2);

	char *key = gn_leaf_context_get_key_at(context, 0);

	TEST_ASSERT(key == NULL);

	char *ret = gn_leaf_context_set(context, "key 1", "value 1");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 1") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 1);

	char *key = gn_leaf_context_get_key_at(context, 0);

	TEST_ASSERT(strcmp("key 1", key) == 0);

	gn_leaf_context_destroy(context);
}

void test_gn_leaf_context_get_key() {

	context = gn_leaf_context_create(2);

	char *ret = gn_leaf_context_set(context, "key 1", "value 1");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 1") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 1);

	char *val = gn_leaf_context_get(context, "key 1");
	TEST_ASSERT(strcmp("value 1", val) == 0);

	char *val = gn_leaf_context_get(context, "key 2");
	TEST_ASSERT(strcmp(NULL, val) == 0);

	gn_leaf_context_destroy(context);
}

void test_gn_leaf_context_add() {

	context = gn_leaf_context_create(2);

	char *ret = gn_leaf_context_set(context, "key 1", "value 1");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 1") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 1);

	ret = gn_leaf_context_set(context, "key 2", "value 2");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 2") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 2);

	ret = gn_leaf_context_set(context, "key 3", "value 3");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 3") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 3);

	ret = gn_leaf_context_set(context, "key 3", "value 3 modified");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 3") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 3);

	ret = gn_leaf_context_set(context, "key 4", "value 4");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 4") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 4);

	ret = gn_leaf_context_set(context, "key 1", "value 1 modified");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 1") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 4);

	//gn_leaf_context_print(context);
	gn_leaf_context_destroy(context);

}

void test_gn_leaf_context_delete() {

	context = gn_leaf_context_create(2);

	ESP_LOGI(TAG, "add key 1");
	char *ret = gn_leaf_context_set(context, "key 1", "value 1");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 1") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 1);

	ESP_LOGI(TAG, "add key 2");
	ret = gn_leaf_context_set(context, "key 2", "value 2");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 2") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 2);

	ESP_LOGI(TAG, "del key 2");
	void *value = gn_leaf_context_delete(context, "key 2");
	gn_leaf_context_print(context);
	TEST_ASSERT(value != NULL);
	TEST_ASSERT(strcmp("value 2", (char*)value) == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 1);

	ESP_LOGI(TAG, "del key X");
	value = gn_leaf_context_delete(context, "key X");
	gn_leaf_context_print(context);
	TEST_ASSERT(value == NULL);
	TEST_ASSERT(gn_leaf_context_size(context) == 1);

	ESP_LOGI(TAG, "add key 2");
	ret = gn_leaf_context_set(context, "key 2", "value 2 modified");
	gn_leaf_context_print(context);
	TEST_ASSERT(strcmp(ret, "key 2") == 0);
	TEST_ASSERT(gn_leaf_context_size(context) == 2);

	gn_leaf_context_destroy(context);

}

int main(int argc, char **argv) {
	UNITY_BEGIN();

	ESP_LOGI(TAG, "----- HOST TEST GROWNODE START ------");

	/*
	 RUN_TEST(test_gn_leaf_context_create);
	 RUN_TEST(test_gn_leaf_context_destroy);
	 RUN_TEST(test_gn_leaf_context_push);
	 RUN_TEST(test_gn_leaf_context_push_top);
	 RUN_TEST(test_gn_leaf_context_pop);
	 RUN_TEST(test_gn_leaf_context_pop_empty);
	 */
	ESP_LOGI(TAG, " * * * * * test_gn_leaf_context_create");
	RUN_TEST(test_gn_leaf_context_create);
	ESP_LOGI(TAG, " * * * * * test_gn_leaf_context_get_key_at_0");
	RUN_TEST(test_gn_leaf_context_get_key_at_0);
	ESP_LOGI(TAG, " * * * * * test_gn_leaf_context_get_key");
	RUN_TEST(test_gn_leaf_context_get_key);
	ESP_LOGI(TAG, " * * * * * test_gn_leaf_context_add");
	RUN_TEST(test_gn_leaf_context_add);
	ESP_LOGI(TAG, " * * * * * test_gn_leaf_context_delete");
	RUN_TEST(test_gn_leaf_context_delete);


	ESP_LOGI(TAG, "------ HOST TEST GROWNODE END -------");

	int failures = UNITY_END();

	return failures;

}


/*
 void test_gn_leaf_context_create() {

 context = gn_leaf_context_create("item 1", "value 1");

 TEST_ASSERT(context != 0);
 TEST_ASSERT(gn_leaf_context_size(context) == 1);

 gn_leaf_context_print(context);
 gn_leaf_context_destroy(context);

 }

 void test_gn_leaf_context_destroy() {

 context = gn_leaf_context_create("item 1", "value 1");
 gn_leaf_context_destroy(context);

 TEST_ASSERT(context == 0);

 }

 void test_gn_leaf_context_push() {

 context = gn_leaf_context_create("item 1", "value 1");
 gn_leaf_context_push(context, "item 2", "value 2");

 TEST_ASSERT(gn_leaf_context_size(context) == 2);

 gn_leaf_context_print(context);
 gn_leaf_context_destroy(context);
 }

 void test_gn_leaf_context_push_top() {

 context = gn_leaf_context_create("item 1", "value 1");
 gn_leaf_context_push(context, "item 2", "value 2");
 gn_leaf_context_push_top(&context, "item 0", "value 0");

 TEST_ASSERT(gn_leaf_context_size(context) == 3);

 gn_leaf_context_print(context);
 gn_leaf_context_destroy(context);
 }

 void test_gn_leaf_context_pop() {

 context = gn_leaf_context_create("item 1", "value 1");
 char* ret = (char*)gn_leaf_context_pop(&context);

 TEST_ASSERT(strcmp(ret, "value 1") == 0);
 TEST_ASSERT(gn_leaf_context_size(context) == 0);
 gn_leaf_context_print(context);

 }

 void test_gn_leaf_context_pop_empty() {

 context = gn_leaf_context_create("item 1", "value 1");
 char* ret = (char*)gn_leaf_context_pop(&context);

 TEST_ASSERT(strcmp(ret, "value 1") == 0);
 TEST_ASSERT(gn_leaf_context_size(context) == 0);

 ret = (char*)gn_leaf_context_pop(context);

 TEST_ASSERT(ret == NULL);
 TEST_ASSERT(gn_leaf_context_size(context) == 0);

 }
 */


