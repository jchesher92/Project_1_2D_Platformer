#include <stdlib.h>
#include <string.h>
#include "../util.h"
#include "../array_list.h"

/*
The array_list_create is fairly straightforward; here, memory is allocated for the list and return NULL if that fails.
After that set up the members and allocate space for the items based on the initial capacity.
Finally, return a pointer to the list.
*/
Array_List *array_list_create(usize item_size, usize initial_capacity) {
    Array_List *list = malloc(sizeof(Array_List));
    if (!list) {
        ERROR_RETURN(NULL, "Could not allocate memory for Array_List\n");
    }
    list->item_size = item_size;
    list->capacity = initial_capacity;
    list->len = 0;
    list->items = malloc(item_size * initial_capacity);

    if (!list->items) {
        ERROR_RETURN(NULL, "Could not allocate memory for Array_List\n");
    }
    return list;
}

/*
The append function starts with checking the list to see if the list is full. 
If it is, then increase the capacity. If the capacity = 0, then increase it; such that, capacity = 1.
Otherwise, capacity *= 2. 

WARNING 
This could potentially waste memory, however, for simplicity, current implementation is okay.

realloc is used to get more space. Return out if that fails or update the pointer if that succeeds.
*/
usize array_list_append(Array_List *list, void *item) {
	if (list->len == list->capacity) {
		list->capacity = list->capacity > 0 ? list->capacity * 2 : 1;
		void *items = realloc(list->items, list->item_size * list->capacity);

		if (!items) {
			ERROR_RETURN(-1, "Could not allocate memory for Array_List\n");
        }

		list->items = items;
	}

	usize index = list->len++;

	memcpy((u8*)list->items + index * list->item_size, item, list->item_size);

	return index;
}

/*
This handles the pointer arithmetic and out of bounds checking.
Returns NULL if it failed and a pointer to the item if it succeeded.
*/
void *array_list_get(Array_List *list, usize index) {
	if (index >= list->len) {
		ERROR_RETURN(NULL, "Index out of bounds\n");
    }

	return (u8*)list->items + index * list->item_size;
}

/*
This what's know as an 'Out of Order Remove'; that means that if you need to keep all of
the items in the list in the same order, this won't work. Instead, copy the last item into the
spot where the item to be removed is and then decrease the length by -1.
*/
u8 array_list_remove(Array_List *list, usize index) {
	if (list->len == 0) {
		ERROR_RETURN(1, "List is empty\n");
    }
	if (index >= list->len) {
		ERROR_RETURN(1, "Index out of bounds\n");
    }

	if (list->len == 1) {
		list->len = 0;
		return 0;
	}

	--list->len;

	u8 *item_ptr = (u8*)list->items + index * list->item_size;
	u8 *end_ptr = (u8*)list->items + list->len * list->item_size;
	memcpy(item_ptr, end_ptr, list->item_size);

	return 0;
}