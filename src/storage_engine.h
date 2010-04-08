
/*
* Copyright (C) leon.hong
*/


#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_

#include <stdint.h>
#include <time.h>

#define HDBFILEMODE 0644
#define HDBHEADERSIZE 256

typedef struct {                            /* structure for "Pangu" database */
	uint64_t bucket_num;                /* number of the bucket array */
	char *path;                         /* path of the database file */
	int fd;                             /* file descriptor of the database file */
	uint64_t rec_num;                   /* number of the records */
	uint64_t file_size;                 /* size of the database file */
	uint64_t first_rec_off;             /* offset of the first record */
	uint64_t last_rec_off;              /* offset of the last record */
	char *map;                          /* pointer to the mapped memory */
	uint64_t map_size;                  /* size of the mapped memory */
	uint64_t *hashtable;                /* a bucket array */
	time_t mtime;                       /* modification time */
} database_t;

typedef struct {                            /* structure of "Pangu" database for record*/
	uint64_t off;                       /* offset of the record */
	uint64_t prev_off;                  /* offset of the prev record */
	uint64_t next_off;                  /* offset of the next record */
	uint64_t key_size;                  /* size of key */
	uint64_t value_size;                /* size of value */
	uint64_t size;                      /* size of record */
	char *key;                          /* key */
	char *value;                        /* value */
} record_t;


/* Get a record in a hash database object.
 * `hdb' specifies the database object.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * `value_buf' specifies the pointer to the region of the value.
 * `value_size' specifies the size of the region of the value.
 * If successful, the return value is PANGU_OK. */
int storage_engine_get(database_t *hdb, const void *key_buf, size_t key_size, char *value_buf, size_t *value_size);

/* Store a record into a hash database object.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * `value_buf' specifies the pointer to the region of the value.
 * `value_size' specifies the size of the region of the value.
 * If successful, the return value is PANGU_OK. */
int storage_engine_set(database_t *hdb, const void *key_buf, size_t key_size, const void* value_buf, size_t value_size);

/* Remove a record of a database object.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * If successful, the return value is PANGU_OK. */
int storage_engine_remove(database_t *hdb, const void *key_buf, size_t key_size);

/* Open record in a database object.
 * `hdb' specifies the database object.
 * `bucket_num' specifies the bucket number.
 * `path' specifies the path of the "pangu.db".
 * If successful, the return value is PANGU_OK. */
int storage_engine_open(database_t *hdb, uint64_t bucket_num, char *path);

/* Copy a strings.
 * `str' source strings.
 * `res' output strings.
 * If successful, the return value is PANGU_OK. */
int storage_engine_str_dup(const void *str, void **res);

/* Write data into a file.
 * `fd' fd of specifies file.
 * `buf' specifies the pointer to the data region.
 * `size' specifies the size of the region.
 * If successful, the return value is PANGU_OK. */
int storage_engine_write(int fd, const void *buf, size_t size);

/* Copy data header of hdb from a buf.
 * `hdb' specifies the hash database object connected as a writer.
 * `hbuf' specifies the pointer to the data region.*/
void storage_engine_dup_meta(database_t *hdb, const char *hbuf);

/* Hash function1.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * the return value is hash value. */
uint64_t storage_engine_hash1(database_t *hdb, const char *key_buf, int key_size);

/* Hash function2.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * the return value is hash value. */
uint64_t storage_engine_hash2(database_t *hdb, const char *key_buf, int key_size);

/* Get bucketid.
 * `hdb' specifies the hash database object connected as a writer.
 * `key_buf' specifies the pointer to the region of the key.
 * `key_size' specifies the size of the region of the key.
 * the return value is offset of the record. */
uint64_t storage_engine_get_bucket(database_t *hdb, uint64_t id);

/* Write the record.
 * `fd' fd of specifies file.
 * `hrec' specifies the pointer to the region of the record.
 * If successful, the return value is PANGU_OK. */
int storage_engine_write_record(int fd, record_t *hrec);

/* Read the record.
 * `hrec' specifies the pointer to the region of the record.
 * `fd' fd of specifies file.
 * `off' offset of the recode.
 * If successful, the return value is PANGU_OK. */
int storage_engine_read_record(record_t *hrec, int fd, uint64_t off);

/* Seek and read data into a file.
 * `fd' fd of specifies file.
 * `off' offset of the recode.
 * `buf' specifies the pointer to the data region.
 * `size' specifies the size of the region.
 * If successful, the return value is PANGU_OK. */
int storage_engine_seekread(int fd, off_t off, void *buf, size_t size);

/* Seek and Write data into a file.
 * `fd' fd of specifies file.
 * `off' offset of the recode.
 * `buf' specifies the pointer to the data region.
 * `size' specifies the size of the region.
 * If successful, the return value is PANGU_OK. */
int storage_engine_seekwrite(int fd, off_t off, void *buf, size_t size);

/* Read data into a file.
 * `fd' fd of specifies file.
 * `buf' specifies the pointer to the data region.
 * `size' specifies the size of the region.
 * If successful, the return value is PANGU_OK. */
int storage_engine_read(int fd, void* buf, size_t size);

/* Write offset of next record into a file.
 * `fd' fd of specifies file.
 * `off' offset of the recode.
 * `hrec' specifies the pointer to the region of the record.
 * If successful, the return value is PANGU_OK. */
int storage_engine_write_currecord_nextoff(int fd, off_t off, record_t *hrec);

/* Read size of the record.
 * `fd' fd of specifies file.
 * `off' offset of the recode.
 * `hrec' specifies the pointer to the region of the record.
 * `psize' specifies the size of the record.
 * If successful, the return value is PANGU_OK. */
int storage_engine_read_record_size(int fd, record_t *hrec, off_t off, uint64_t *psize);

/* Copy a record.
 * `src' pointer to the src ecord.
 * `des' pointer to the des record.
 * If successful, the return value is PANGU_OK. */
int storage_engine_copy_record(record_t *src, record_t *des);

#endif
