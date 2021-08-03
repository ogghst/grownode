#ifndef GN_LEAF_CONTEXT_H_
#define GN_LEAF_CONTEXT_H_

#include <stddef.h>
#include <stdbool.h>

typedef void *gn_leaf_context_handle_t;

gn_leaf_context_handle_t gn_leaf_context_create(size_t size);

size_t gn_leaf_context_size(gn_leaf_context_handle_t context);

void gn_leaf_context_destroy(gn_leaf_context_handle_t context);

void gn_leaf_context_print(gn_leaf_context_handle_t context);

char* gn_leaf_context_get_key_at(gn_leaf_context_handle_t context,
		size_t index);

void* gn_leaf_context_set(gn_leaf_context_handle_t context, char *key,
		void *value);

void* gn_leaf_context_delete(gn_leaf_context_handle_t context, char *key);

void* gn_leaf_context_get(gn_leaf_context_handle_t context, char *key);


#endif
