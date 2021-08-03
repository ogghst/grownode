/*
 * gn_leaf_context.c
 *
 *  Created on: Jul 24, 2021
 *      Author: nicola
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 #include <stddef.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <limits.h>
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gn_leaf_context.h"
#include "esp_log.h"

//#include "hasht.h"
#include "cc_hashtable.h"

static const char *TAG = "gn_leaf_context";

typedef struct CC_HashTable *gn_leaf_context_handle_int_t;


gn_leaf_context_handle_t gn_leaf_context_create(size_t size) {

	CC_HashTable * string_table;
	CC_HashTableConf config;
	cc_hashtable_conf_init(&config);
	config.key_length = KEY_LENGTH_VARIABLE;
	config.hash = STRING_HASH;
	config.key_compare = CC_CMP_STRING;
	enum cc_stat status = cc_hashtable_new_conf(&config, &string_table);
	if (status != CC_OK)
		return NULL;
	return (gn_leaf_context_handle_int_t)string_table;

}

size_t gn_leaf_context_size(gn_leaf_context_handle_t context) {
	return cc_hashtable_size(context);
}

void gn_leaf_context_destroy(gn_leaf_context_handle_t context) {
    cc_hashtable_remove_all(context);
	cc_hashtable_destroy(context);
}

void gn_leaf_context_print(gn_leaf_context_handle_t context) {

    /* Define and initialize the iterator. */
    CC_HashTableIter iterator;
    cc_hashtable_iter_init(&iterator, context);

    /* Define pair entry output. */
    TableEntry *next_entry;
	ESP_LOGI(TAG, "----- HASHTABLE START ------");
    while (cc_hashtable_iter_next(&iterator, &next_entry) != CC_ITER_END) {
        ESP_LOGI(TAG, "K: %s, V: %s", (char*) next_entry->key, (char*) next_entry->value);
    }
	ESP_LOGI(TAG, "------ HASHTABLE END -------");
}

char* gn_leaf_context_get_key_at(gn_leaf_context_handle_t context,
		size_t index) {

	size_t size = cc_hashtable_size(context);
	if (index >= size)
		return NULL;

    /* Define and initialize the iterator. */
    CC_HashTableIter iterator;
    cc_hashtable_iter_init(&iterator, context);

    int i = 0;
    /* Define pair entry output. */
    TableEntry *next_entry;
    while (cc_hashtable_iter_next(&iterator, &next_entry) != CC_ITER_END) {
    	if (i == index)
    		return (char*) next_entry->key;
    }
    return NULL;

}


void* gn_leaf_context_get(gn_leaf_context_handle_t context, char *key) {

	void* out = NULL;
	enum cc_stat ret = cc_hashtable_get(context, key, out);
	if (ret == CC_OK)
		return out;
	return NULL;
}

void* gn_leaf_context_set(gn_leaf_context_handle_t context, char *key,
		void *value) {

	enum cc_stat stat = cc_hashtable_add(context,  (void*) key,  (void*) value);
	if (stat == CC_OK)
		return key;
	else
		return NULL;

}

void* gn_leaf_context_delete(gn_leaf_context_handle_t context, char *key) {
	void* oldv = NULL;
	enum cc_stat ret = cc_hashtable_remove(context,  (void*) key, &oldv);
	if (ret == CC_OK)
		return oldv;
	return NULL;
}



#ifdef __cplusplus
}
#endif //__cplusplus
