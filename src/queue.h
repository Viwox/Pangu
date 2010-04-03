#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "pangu.h"

typedef struct {
	single_list_t* list;
} queue_t;

int queue_create(queue_t** q) {
	*q = (queue_t*)pangu_malloc(sizeof(queue_t));
	if (*q == NULL) {
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	return single_list_create(&((*q)->list));
}

size_t queue_size(queue_t* q) {
	return single_list_size(q->list);
}

int queue_push(queue_t* q, void* data) {
	return single_list_push_back(q->list, data);
}

void queue_pop_front(queue_t* q) {
	return single_list_pop_front(q->list);
}

void* queue_top(queue_t* q) {
	return single_list_front(q->list);
}

void queue_destroy(queue_t** q) {
	single_list_destroy(&((*q)->list));
	pangu_free(*q);
}

#endif
