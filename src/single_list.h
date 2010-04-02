
/*
* Copyright (C) leon.hong
*/


#ifndef _SINGLE_LIST_H_
#define _SINGLE_LIST_H_

#include <sys/types.h>
#include "pangu.h"

typedef struct single_list_t {
	struct single_list_t *prev;
   	struct single_list_t *next;
	int data;
} single_list_t;

typedef struct single_list_s {
	size_t size;
	single_list_t *header;
} single_list_s;

ssize_t single_list_create(single_list_s**);

size_t single_list_size(single_list_s*);

ssize_t single_list_push(single_list_s*, void*);

ssize_t single_list_pop(single_list_s*);

void* single_list_top(single_list_s*);

ssize_t single_list_destroy(single_list_s**);

#endif
