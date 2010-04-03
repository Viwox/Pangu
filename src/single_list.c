
/*
* Copyright (C) leon.hong
*/


#include <stdlib.h>
#include "single_list.h"

int single_list_create(single_list_t **q) {
	list_node *item;
	item = (list_node*)pangu_malloc(sizeof(list_node));
	if (!item) {
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	item->next = item, item->pdata = 0;
	*q = (single_list_t*)pangu_malloc(sizeof(single_list_t));
	if (!*q) {
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	(*q)->header = item, (*q)->tail = item, (*q)->size = 0;
	return PANGU_OK;
}

size_t single_list_size(single_list_t *q) {
	return q->size;
}

int single_list_push_back(single_list_t *q, void *pdata) {
	list_node *item;
	item = (list_node*)pangu_malloc(sizeof(list_node));
	if (!item) {
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	item->pdata = pdata, item->next = 0;
	q->tail->next = item;
	q->tail = item;
	++q->size;
	return PANGU_OK;
}

int single_list_pop_front(single_list_t *q) {
	list_node *item;
	item = q->header->next;
	q->header->next = item->next;
	pangu_free(item);
	--q->size;
	return PANGU_OK;
}

void* single_list_front(single_list_t *q) {
	return q->header->next->pdata;
}

int single_list_destroy(single_list_t **q) {
	while ((*q)->size--) {
		list_node *item;
		item = (*q)->header->next;
		(*q)->header->next = (*q)->header->next->next;
		pangu_free(item);
	}
	pangu_free((*q)->header);
	pangu_free(*q);
	return PANGU_MEMORY_NOT_ENOUGH;
}
