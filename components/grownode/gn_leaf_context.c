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

/*
 typedef struct gn_leaf_context *gn_leaf_context_handle_int_t;


 typedef struct gn_leaf_context {
 char name[16];
 void *data;
 struct gn_leaf_context *next;
 } gn_leaf_context_t;

 gn_leaf_context_handle_t gn_leaf_context_create(const char name[16], void *data) {

 gn_leaf_context_t *head = NULL;
 head = (gn_leaf_context_t*) malloc(sizeof(gn_leaf_context_t));
 if (head == NULL) {
 return NULL;
 }

 head->data = data;
 strcpy(head->name, name);
 head->next = NULL;

 return head;
 }

 void gn_leaf_context_destroy(gn_leaf_context_handle_t context) {

 }

 int gn_leaf_context_size(gn_leaf_context_handle_t context) {

 int i = 0;
 gn_leaf_context_handle_int_t current =
 (gn_leaf_context_handle_int_t) context;

 while (current != NULL && i < INT_MAX) {
 i++;
 current = current->next;
 }

 return i;

 }

 void gn_leaf_context_print(gn_leaf_context_handle_t context) {

 gn_leaf_context_handle_int_t current =
 (gn_leaf_context_handle_int_t) context;

 while (current != NULL) {
 ESP_LOGI(TAG, "name: %s\n", current->name);
 current = current->next;
 }

 }

 void gn_leaf_context_push(gn_leaf_context_handle_t context, const char name[16],
 void *data) {

 gn_leaf_context_handle_int_t current =
 (gn_leaf_context_handle_int_t) context;
 while (current->next != NULL) {
 current = current->next;
 }

 // now we can add a new variable
 current->next = (gn_leaf_context_t*) malloc(sizeof(gn_leaf_context_t));

 current->next->data = data;
 strcpy(current->next->name, name);
 current->next->next = NULL;

 }

 void gn_leaf_context_push_top(gn_leaf_context_handle_t *context,
 const char name[16], void *data) {

 gn_leaf_context_handle_int_t *head = (gn_leaf_context_handle_int_t*) context;

 gn_leaf_context_t *new_node;
 new_node = (gn_leaf_context_t*) malloc(sizeof(gn_leaf_context_t));

 new_node->data = data;
 strcpy(new_node->name, name);
 new_node->next = *head;
 *head = new_node;
 }

 void* gn_leaf_context_pop(gn_leaf_context_handle_t *context) {

 gn_leaf_context_handle_int_t *head = (gn_leaf_context_handle_int_t*) context;

 if (head == NULL) {
 return NULL;
 }

 void *retval = NULL;
 gn_leaf_context_t *next_node = NULL;

 next_node = (*head)->next;
 retval = (*head)->data;
 free(*head);
 *head = next_node;

 return retval;
 }

 int remove_by_index(gn_leaf_context_handle_t * context, int n) {

 gn_leaf_context_handle_int_t *head = (gn_leaf_context_handle_int_t*) context;

 int i = 0;
 void *retval = NULL;
 gn_leaf_context_t * current = *head;
 gn_leaf_context_t * temp_node = NULL;

 if (n == 0) {
 return pop(head);
 }

 for (i = 0; i < n-1; i++) {
 if (current->next == NULL) {
 return -1;
 }
 current = current->next;
 }

 temp_node = current->next;
 retval = temp_node->data;
 current->next = temp_node->next;
 free(temp_node);

 return retval;

 }
 */

/*
typedef struct hasht *gn_leaf_context_handle_int_t;


void _node_delete(struct hasht_node *n) {

	ESP_LOGI(TAG, "deleting");

	if (!n)
		return;
	ESP_LOGI(TAG, "	deleting key %s", (char*)n->key);
	if (n->key)
		free(n->key);
	//ESP_LOGI(TAG, "	deleting value");
	//if (n->value)
	//	free(n->value);
	n->value = NULL;
	ESP_LOGI(TAG, "	deleting node");
	free(n);
}

gn_leaf_context_handle_t gn_leaf_context_create(size_t size) {

	gn_leaf_context_handle_int_t t = hasht_new(size, true, NULL, NULL,
			_node_delete);

	return t;
}

void* gn_leaf_context_delete(gn_leaf_context_handle_t context, char *key) {

	if (key == NULL)
		return NULL;

	gn_leaf_context_handle_int_t _ctx = (gn_leaf_context_handle_int_t) context;

	return hasht_delete(_ctx, key);
}

size_t gn_leaf_context_used(gn_leaf_context_handle_t context) {

	gn_leaf_context_handle_int_t _ctx = (gn_leaf_context_handle_int_t) context;

	return hasht_used(_ctx);

}

void gn_leaf_context_destroy(gn_leaf_context_handle_t context) {

	gn_leaf_context_handle_int_t _ctx = (gn_leaf_context_handle_int_t) context;

	return hasht_free(_ctx);
}

void gn_leaf_context_print(gn_leaf_context_handle_t context) {

	gn_leaf_context_handle_int_t _ctx = (gn_leaf_context_handle_int_t) context;

	ESP_LOGI(TAG, "----- HASHTABLE START ------");
	ESP_LOGI(TAG, "Used: %d", (int ) hasht_used(_ctx));
	for (int i = 0; i < _ctx->size; i++) {
		void *key = hasht_get_key_at(_ctx, i);
		if (key != NULL) {
			ESP_LOGI(TAG, "Key: %s, Value: %p", (char* ) key,
					(void* ) hasht_search(_ctx, key));
		}
	}
	ESP_LOGI(TAG, "------ HASHTABLE END -------");
}

char* gn_leaf_context_get_key_at(gn_leaf_context_handle_t context, size_t index) {

	gn_leaf_context_handle_int_t _ctx = (gn_leaf_context_handle_int_t) context;

	if (index >= _ctx->size)
		return NULL;

	return hasht_get_key_at(_ctx, index);

}

void* gn_leaf_context_set(gn_leaf_context_handle_t context, char *key,
		void *value) {

	if (key == NULL)
		return NULL;

	gn_leaf_context_handle_int_t _ctx = (gn_leaf_context_handle_int_t) context;

	char *_key = malloc(strlen(key) * sizeof(char) + 1);
	strcpy(_key, key);

	ESP_LOGI(TAG, "gn_leaf_context_set-hasht_search: %s", _key);
	void *oldvalue = hasht_search(context, _key);
	if (oldvalue != NULL) {
		//free(oldvalue);
		ESP_LOGI(TAG, "gn_leaf_context_set-found old key: %s", _key);
		hasht_delete(context, _key);
	}
	ESP_LOGI(TAG, "gn_leaf_context_set-hasht_insert: %s", _key);
	return ((struct hasht_node*) hasht_insert(_ctx, _key, value))->key;
}
*/

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
