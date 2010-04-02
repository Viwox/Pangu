
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

<<<<<<< HEAD
typedef struct single_list_s {
	size_t size;
	single_list_t *header;
} single_list_s;

ssize_t single_list_create(single_list_s**);
=======
/*
 * return:
 *	0	-	success
 *	-1	-	failure
 */

int single_list_create(single_list_t**);
>>>>>>> c8cc20e9aa511313d119137edf2dd339b7f248b6

size_t single_list_size(single_list_s*);

<<<<<<< HEAD
ssize_t single_list_push(single_list_s*, void*);
=======
/*
 * return:
 *	0	-	success
 *	-1	-	failure
 */
int single_list_push(single_list_t*, void*);
>>>>>>> c8cc20e9aa511313d119137edf2dd339b7f248b6

ssize_t single_list_pop(single_list_s*);

void* single_list_top(single_list_s*);

ssize_t single_list_destroy(single_list_s**);

#endif
