
/*
* Copyright (C) leon.hong
*/


#ifndef _SINGLE_LIST_H_
#define _SINGLE_LIST_H_

#include <sys/types.h>
#include "pangu.h"

typedef struct list_node {
   	struct list_node *next;
	void* pdata;
} list_node;

typedef struct single_list_t {
	size_t size;
	list_node *header;
	list_node *tail;
} single_list_t;

int single_list_create(single_list_t**);

size_t single_list_size(single_list_t*);

int single_list_push(single_list_t*, void*);

int single_list_pop(single_list_t*);

void* single_list_top(single_list_t*);

int single_list_destroy(single_list_t**);

#endif
