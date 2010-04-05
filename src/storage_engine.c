
/*
* Copyright (C) leon.hong
*/


#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pangu.h"
#include "storage_engine.h"

int storage_engine_open(database_t *hdb, uint64_t bucket_num, char *path) {
	hdb->bucket_num = bucket_num;
	if (storage_engine_str_dup(path, &hdb->path) != PANGU_OK) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	int fd = open(path, O_RDWR|O_CREAT, HDBFILEMODE);
	if (fd < 0) {
		error_msg(PANGU_OPEN_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_OPEN_FILE_FAIL;
	}
	struct stat sbuf;
	if (fstat(fd, &sbuf) == -1) {
		error_msg(PANGU_STAT_FILE_FAIL, __FILE__, __LINE__, __func__); 
		return PANGU_STAT_FILE_FAIL;
	}
	hdb->mtime = sbuf.st_mtime;
	hdb->fd = fd;
	hdb->file_size = HDBHEADERSIZE + hdb->bucket_num * sizeof(uint64_t);
	hdb->first_rec_off = hdb->file_size;
	hdb->last_rec_off = hdb->file_size;
	char buf[HDBHEADERSIZE];
	storage_engine_dup_meta(hdb, buf);
	if (storage_engine_write(fd, buf, HDBHEADERSIZE) != PANGU_OK) {
		error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
		return PANGU_WRITE_FILE_FAIL;
	}
	hdb->map_size = HDBHEADERSIZE + hdb->bucket_num * sizeof(uint64_t);
	void *map = mmap(0, hdb->map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	hdb->map = map;
	hdb->hashtable = (uint64_t*)((char*)map + HDBHEADERSIZE);
	return PANGU_OK;
}

int storage_engine_write(int fd, const void *buf, size_t size) {
	assert(fd >= 0 && buf && size >=0);
	char *p = buf;
	do {
		int write_size = write(fd, p, size);
		switch(write_size) {
			case -1 : 
				if (errno != EINTR) {
					error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
					return PANGU_WRITE_FILE_FAIL;
				}
			case 0 : break;
			default :
				p += write_size;
				size -= write_size;
				break;
		}
	}while(size > 0);
	return PANGU_OK;
}

void storage_engine_dup_meta(database_t *hdb, const char *hbuf) {
	memset(hbuf, 0, HDBHEADERSIZE);
	sprintf(hbuf, "%s\n", PANGU_VERSION);
	uint64_t lnum;
	lnum = hdb->bucket_num;
	memcpy(hbuf + 32, &lnum, sizeof(lnum));
	lnum = hdb->rec_num;
	memcpy(hbuf + 40, &lnum, sizeof(lnum));
	lnum = hdb->file_size;
	memcpy(hbuf + 48, &lnum, sizeof(lnum));
	lnum = hdb->first_rec_off;
	memcpy(hbuf + 56, &lnum, sizeof(lnum));  
}

int storage_engine_str_dup(const void *str, void **res) {
	assert(str);
	size_t size = strlen(str);
	char *p = (char*)pangu_malloc(size + 1);
	if (!p) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	memcpy(p, str, size);
	p[size] = '\0';
	*res = p;
	return PANGU_OK;
}

int storage_engine_set(database_t *hdb, const void *key_buf, size_t key_size, const void* value_buf, size_t value_size) {
	uint64_t bucketid = storage_engine_hash2(hdb, key_buf, key_size);
	uint64_t off = storage_engine_get_bucket(hdb, bucketid);
	record_t *hrec = (record_t*)malloc(sizeof(record_t));
	while (off > 0) {
		if (storage_engine_read_record(hrec, hdb->fd, off) != PANGU_OK) {
			error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
			return PANGU_READ_FILE_FAIL;
		}
		off = hrec->next_off;
	}
	hrec->off = hdb->file_size;
	hrec->next_off = hdb->file_size+ hrec->size;
	hrec->prev_off = off;
	hrec->key_size = key_size;
	hrec->value_size = value_size;
	if (storage_engine_str_dup(key_buf, &hrec->key) != PANGU_OK) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	if (storage_engine_str_dup(value_buf, &hrec->value) != PANGU_OK) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	if (storage_engine_write_record(hdb->fd, hrec) != PANGU_OK) {
		error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_WRITE_FILE_FAIL;
	}
	if (!hdb->hashtable[bucketid]) {
		hdb->hashtable[bucketid] = hrec->off;
	}
	hdb->file_size += hrec->size;
	hdb->last_rec_off = hrec->off;
	pangu_free(hrec);
	return PANGU_OK;
}

uint64_t storage_engine_hash1(database_t *hdb, const char *key_buf, int key_size) {
	assert(hdb && key_buf && key_size >= 0);
	uint64_t hash = 19780211;
	while(key_size--)
		hash = (hash << 5) + (hash << 2) + hash + *(uint8_t *)key_buf++;
	return hash % hdb->bucket_num;
}

uint64_t storage_engine_hash2(database_t *hdb, const char *key_buf, int key_size) {
	assert(hdb && key_buf && key_size >= 0);
	uint64_t hash = 5381;
	while (key_size--)
		hash = ((hash << 5) + hash) + (*key_buf++); /* hash * 33 + c */
	return hash % hdb->bucket_num;
}

uint64_t storage_engine_get_bucket(database_t *hdb, uint64_t id) {
	return hdb->hashtable[id];
}

int storage_engine_write_record(int fd, record_t *hrec) {
	char hbuf[HDBIOBUF];
	memset(hbuf, 0, HDBIOBUF);
	uint64_t lnum;
	lnum = hrec->off;
	memcpy(hbuf, &lnum, sizeof(lnum));
	lnum = hrec->prev_off;
	memcpy(hbuf + 8, &lnum, sizeof(lnum));
	lnum = hrec->next_off;
	memcpy(hbuf + 16, &lnum, sizeof(lnum));
	lnum = hrec->key_size;
	memcpy(hbuf + 24, &lnum, sizeof(lnum));
	lnum = hrec->value_size;
	memcpy(hbuf + 32, &lnum, sizeof(lnum));
	memcpy(hbuf + 40, hrec->key, hrec->key_size);
	memcpy(hbuf + hrec->key_size, hrec->value, hrec->value_size);
	hrec->size = 40 + hrec->key_size + hrec->value_size;
	return storage_engine_seekwrite(fd, hrec->off, hbuf, hrec->size);
}

int storage_engine_read_record(record_t *hrec, int fd, uint64_t off) {
	char hbuf[HDBIOBUF];
	memset(hbuf, 0, HDBIOBUF);
	char *p = hbuf;
	if (storage_engine_seekread(fd, off, hbuf, HDBIOBUF) != PANGU_OK) {
		error_msg(PANGU_SEEK_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_SEEK_FILE_FAIL;
	}
	hrec->off = *(uint64_t*)(p++);
	hrec->prev_off = *(uint64_t*)(p++);
	hrec->next_off = *(uint64_t*)(p++);
	hrec->key_size = *(uint64_t*)(p++);
	hrec->value_size = *(uint64_t*)(p++);
	hrec->size = *(uint64_t*)(p++);
	memcpy(hrec->key, p, hrec->key_size);
	p += hrec->key_size;
	memcpy(hrec->value, p, hrec->value_size);
	p += hrec->value_size;
	return PANGU_OK;
}

int storage_engine_seekread(int fd, off_t off, void *buf, size_t size) {
	if (lseek(fd, off, SEEK_SET) == -1) {
		error_msg(PANGU_SEEK_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_SEEK_FILE_FAIL;
	}
	if (storage_engine_read(fd, buf, size) != PANGU_OK) {
		error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_READ_FILE_FAIL;
	}
	return PANGU_OK;
}

int storage_engine_seekwrite(int fd, off_t off, void *buf, size_t size) {
	if (lseek(fd, off, SEEK_SET) == -1) {
		error_msg(PANGU_SEEK_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_SEEK_FILE_FAIL;
	}
	if (storage_engine_write(fd, buf, size) != PANGU_OK) {
		error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_READ_FILE_FAIL;
	}
	return PANGU_OK;
}

int storage_engine_read(int fd, void* buf, size_t size) {
	assert(fd >= 0 && buf && size >= 0);
	char *p = buf;
	do {
		int read_size = read(fd, p, size);
		switch(read_size) {
			case -1 : 
				if (errno != EINTR) {
					error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__); 
					return PANGU_READ_FILE_FAIL;
				}
			case 0 : break;
			default :
				p += read_size;
				size -= read_size;
				break;
		}
		break;	/*debug*/
	} while(size > 0);
	return PANGU_OK;
}
