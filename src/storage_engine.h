
/*
* Copyright (C) leon.hong
*/


#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_

#include <stdint.h>
#include <time.h>

typedef struct {                        /* structure for "Pangu" database */
	uint64_t bucket_num;                /* number of the bucket array */
	uint8_t apow;                       /* power of record alignment */
	char *path;                         /* path of the database file */
	int fd;                             /* file descriptor of the database file */
	uint64_t rec_num;                   /* number of the records */
	uint64_t file_size;                 /* size of the database file */
	uint64_t first_rec_off;             /* offset of the first record */
	uint64_t last_rec_off;              /* offset of the last record */
	char *map;                          /* pointer to the mapped memory */
	uint64_t map_size;                  /* size of the mapped memory */
	uint32_t *hashtable;                /* a bucket array */
	time_t mtime;                       /* modification time */
} database_t;

typedef struct {                        /* structure of "Pangu" database for record*/
	uint32_t off;                       /* offset of the record */
	uint32_t prev_off;                  /* offset of the prev record */
	uint32_t next_off;                  /* offset of the next record */
	uint32_t key_size;                  /* size of key */
	uint32_t value_size;                /* size of value */
	char *key;                          /* key */
	char *value;                        /* value */
} record_t;


/* Get a record in a hash database object.
 * `hdb' specifies the database object.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * If successful, the return value is the pointer to the region of the value of the corresponding
 * record.  `NULL' is returned if no record corresponds. */
int storage_engine_get(database_t *hdb, const void *key_buf, size_t key_size);


/* Store a record into a hash database object.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * `value_buf' specifies the pointer to the region of the value.
 * `value_size' specifies the size of the region of the value.
 * If successful, the return value is true, else, it is false.
 * If a record with the same key exists in the database, it is overwritten. */
int storage_engine_set(database_t *hdb, const void *key_buf, size_t key_size, const void* value_buf, size_t value_size);


/* Remove a record of a database object.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * If successful, the return value is true, else, it is false. */
int storage_engine_remove(database_t *hdb, const void *key_buf, size_t key_size);

#endif
