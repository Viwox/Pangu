
/*
* Copyright (C) leon.hong
*/


#include <stdlib.h>
#include "single_list.h"

ssize_t single_list_create(single_list_s **q) {
	single_list_t *item;
	item = (single_list_t*)pangu_malloc(sizeof(single_list_t));
	if (!item) {
		return PANGU_ERROR;
	}
	item->prev = item, item->next = item, item->data = -1;
	*q = (single_list_s*)pangu_malloc(sizeof(single_list_s));
	if (!*q) {
		return PANGU_ERROR;
	}
	(*q)->header = item, (*q)->size = 0;
	return PANGU_OK;
}

size_t single_list_size(single_list_s *q) {
	return q->size;
}

ssize_t single_list_push(single_list_s *q, void *pfd) {
	single_list_t *item;
	item = (single_list_t*)pangu_malloc(sizeof(single_list_t));
	if (!item) {
		return PANGU_ERROR;
	}
	item->data = *((int*)pfd);
	q->header->prev->next = item;
	item->prev = q->header->prev;
	q->header->prev = item;
	item->next = q->header;
	++q->size;
	return PANGU_OK;
}

ssize_t single_list_pop(single_list_s *q) {
	single_list_t *destory_item;
	destory_item = q->header->next;
	q->header->next = q->header->next->next;
	q->header->next->prev = q->header;
	pangu_free(destory_item);
	--q->size;
	return PANGU_OK;
}

void* single_list_top(single_list_s *q) {
	return (void*)(&(q->header->next->data));
}

ssize_t single_list_destroy(single_list_s **q) {
	while (((*q)->size)--) {
		single_list_t *item;
		item = (*q)->header->next;
		(*q)->header->next = (*q)->header->next->next;
		(*q)->header->next->prev = (*q)->header;
		pangu_free(item);
	}
	pangu_free((*q)->header);
	pangu_free(*q);
	return PANGU_ERROR;
}
