#ifndef SINGLE_LIST_H
#define SINGLE_LIST_H

typedef struct {
} single_list_t;

int single_list_create(single_list_t**);

int single_list_size(single_list_t*);

int single_list_push(single_list_t*, void*);

void single_list_pop(single_list_t*);

void* single_list_top(single_list_t*);

void single_list_destroy(single_list_t**);

#endif
