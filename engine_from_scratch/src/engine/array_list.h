#pragma once

#include "types.h"

typedef struct array_list {
    usize len; // Current number of inserted items
    usize capacity; // Total number of items that can be stored
    usize item_size; // Size of each item in bytes
    void *items; // Pointer to where the array is stored in memory
} Array_List;

Array_List *array_list_create(usize item_size, usize initial_capacity); // Returns a pointer to the created list
usize array_list_append(Array_List *list, void *item); // Returns the index where the item was stored or -1 if it fails
void *array_list_get(Array_List *list, usize index); // Returns a pointer to the data at a specified index
u8 array_list_remove(Array_List *list, usize index); // Removes item from the list (returns 0 for success and 1 if it fails)