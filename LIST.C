#include "list.h"
#include <stdlib.h>
#include <string.h>

struct list_t *list_create(void)
{
	struct list_t *list = calloc(1, sizeof(struct list_t));
	return list;
}

void list_add(struct list_t *list, void *data, size_t data_size, enum list_pos_t position)
{
	struct list_node_t *node;

	/* Sanity check */
	if ((list == NULL) || (data == NULL) || (data_size == 0)) {
		return;
	}

	/* Create new element  */
	node = calloc(1, sizeof(struct list_node_t));

	/* Allocate space and copy data to the new element */
	node->data = calloc(1, data_size);
	memcpy(node->data, data, data_size);

	/* Add it to the list */
	switch (position) {
		case LIST_PREPEND:
			if ((list->head == NULL) || (list->tail == NULL)) { // Empty list case
				list->head = node;
				list->tail = node;
				node->prev = NULL;
				node->next = NULL;
			}
			else {
				node->next = list->head;
				node->prev = NULL;
				list->head->prev = node;
				list->head = node;
			}
			break;
		case LIST_APPEND:
			if ((list->head == NULL) || (list->tail == NULL)) {
				list->head = node;
				list->tail = node;
				node->prev = NULL;
				node->next = NULL;
			}
			else {
				node->prev = list->tail;
				node->next = NULL;
				list->tail->next = node;
				list->tail = node;
			}
			break;
		default:
			break;
	}
}

void list_sort(struct list_t *list, bool (*compare)(const void *, const void *))
{
	bool sorted = true;
	struct list_node_t *last = NULL;
	struct list_node_t *current;
	void *temp;

	/* Sanity check */
	if ((list == NULL) || (compare == NULL)) {
		return;
	}

	/* Empty list case */
	if (list->head == NULL) {
		return;
	}

	/* Bubble sort algorithm - complexity shouldn't matter in such small datasets */
	do {
		sorted = true;
		current = list->head;

		while (current->next != last) {
			if (compare(current->data, current->next->data)) {
				/* Swap data pointers */
				temp = current->data;
				current->data = current->next->data;
				current->next->data = temp;

				sorted = false;
			}
			current = current->next;
		}
		last = current;

	} while (!sorted);
}

void list_traverse(const struct list_t *list, void (*callback)(void *, void *), void *data, enum list_dir_t direction)
{
	struct list_node_t *curr;

	/* Sanity check */
	if ((list == NULL) || (callback == NULL)) {
		return;
	}

	switch (direction) {
		case LIST_DIR_FORWARD:
			curr = list->head;
			while (curr != NULL) {
				callback(curr->data, data);
				curr = curr->next;
			}
			break;

		case LIST_DIR_BACKWARD:
			curr = list->tail;
			while (curr != NULL) {
				callback(curr->data, data);
				curr = curr->prev;
			}
			break;

		default:
			break;
	}
}

void list_destroy(struct list_t *list)
{
	struct list_node_t *curr;
	struct list_node_t *next;

	/* Sanity check */
	if (list == NULL) {
		return;
	}

	curr = list->head;

	while (curr != NULL) {
		next = curr->next; // Get pointer to next node
		free(curr->data); // Free memory allocated for data
		free(curr); // Free memory allocated for node
		curr = next; // Move to next node
	}

	free(list);
}
