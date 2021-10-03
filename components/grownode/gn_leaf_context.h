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

#ifndef GN_LEAF_CONTEXT_H_
#define GN_LEAF_CONTEXT_H_

#include <stddef.h>
#include <stdbool.h>

typedef void *gn_leaf_context_handle_t;

gn_leaf_context_handle_t gn_leaf_context_create();

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
