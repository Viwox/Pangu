
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

/* Open record in a database object. */
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
	hdb->first_rec_off = hdb->file_size;
	hdb->last_rec_off = hdb->file_size;
	if (sbuf.st_size < 1) {
		char buf[HDBIOBUF];
		storage_engine_dup_meta(hdb, buf);
		if (storage_engine_write(fd, buf, HDBHEADERSIZE) != PANGU_OK) {
			error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
			return PANGU_WRITE_FILE_FAIL;
		}
		uint64_t bucket_size = hdb->bucket_num * sizeof(uint64_t);
		memset(buf, 0x0, sizeof(buf));
		while (bucket_size) {
			if (bucket_size < HDBIOBUF) {
				if (storage_engine_write(fd, buf, bucket_size) != PANGU_OK) {
					error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
					return PANGU_WRITE_FILE_FAIL;
				}
				bucket_size = 0;
			}
			else {
				if (storage_engine_write(fd, buf, HDBIOBUF) != PANGU_OK) {
					error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
					return PANGU_WRITE_FILE_FAIL;
				}
				bucket_size -= HDBIOBUF;
			}
		} /* end while */
		hdb->file_size = HDBHEADERSIZE + hdb->bucket_num * sizeof(uint64_t);
	} else {
		hdb->file_size = sbuf.st_size;
	}
	hdb->map_size = HDBHEADERSIZE + hdb->bucket_num * sizeof(uint64_t);
	void *map = mmap(0, hdb->map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	hdb->map = map;
	hdb->hashtable = (uint64_t*)((char*)map + HDBHEADERSIZE);
	return PANGU_OK;
}

/* Write data into a file. */
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

/* Copy data header of hdb from a buf. */
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

/* Copy a strings. */
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

/* Store a record into a hash database object. */
int storage_engine_set(database_t *hdb, const void *key_buf, size_t key_size, const void* value_buf, size_t value_size) {
	assert(key_buf && key_size >= 0 && value_buf && value_size >= 0);
	uint64_t bucketid = storage_engine_hash2(hdb, key_buf, key_size);
	uint64_t off = storage_engine_get_bucket(hdb, bucketid);
	record_t *hrec = (record_t*)pangu_malloc(sizeof(record_t));
	if (!hrec) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	if (!off) {
		hrec->off = hdb->file_size;
		hrec->prev_off = off;
	} else {
		hrec->prev_off = off;
		hrec->off = hdb->file_size;
	}
	while (off > 0) {
		if (storage_engine_read_record(hrec, hdb->fd, off) != PANGU_OK) {
			error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
			return PANGU_READ_FILE_FAIL;
		}
		if (!strcmp(hrec->key, key_buf) && !strcmp(hrec->value, value_buf)) {
			return PANGU_SET_FAIL;
		}
		off = hrec->next_off;
		if (!off) {
			hrec->next_off = hdb->file_size;
			/* write next_off of cur record */
			if (storage_engine_write_currecord_nextoff(hdb->fd, hrec->off, hrec) != PANGU_OK) {
				error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__);
				return PANGU_WRITE_FILE_FAIL;
			}
		}
		pangu_free(hrec->key);
		pangu_free(hrec->value);
	} /* end while */
	hrec->next_off = 0;
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
	uint64_t llnum = hdb->file_size + hrec->size;
	memcpy(hdb->map + 40, &llnum, sizeof(llnum));
	if (!hdb->hashtable[bucketid]) {
		hdb->hashtable[bucketid] = hrec->off;
	}
	hdb->file_size += hrec->size;
	hdb->last_rec_off = hrec->off;
	pangu_free(hrec);
	return PANGU_OK;
}

/* Hash function1. */
uint64_t storage_engine_hash1(database_t *hdb, const char *key_buf, int key_size) {
	assert(hdb && key_buf && key_size >= 0);
	uint64_t hash = 19780211;
	while(key_size--)
		hash = (hash << 5) + (hash << 2) + hash + *(uint8_t *)key_buf++;
	return hash % hdb->bucket_num;
}

/* Hash function2. */
uint64_t storage_engine_hash2(database_t *hdb, const char *key_buf, int key_size) {
	assert(hdb && key_buf && key_size >= 0);
	uint64_t hash = 5381;
	while (key_size--)
		hash = ((hash << 5) + hash) + (*key_buf++); /* hash * 33 + c */
	return hash % hdb->bucket_num;
}

/* Get bucketid. */
uint64_t storage_engine_get_bucket(database_t *hdb, uint64_t id) {
	return hdb->hashtable[id];
}

/* Write offset of next record into a file. */
int storage_engine_write_currecord_nextoff(int fd, off_t off, record_t *hrec) {
	if (lseek(fd, off, SEEK_SET) == -1) {
		error_msg(PANGU_SEEK_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_SEEK_FILE_FAIL;
	}
	if (storage_engine_write(fd, &hrec->next_off, sizeof(hrec->next_off)) != PANGU_OK) {
		error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_READ_FILE_FAIL;
	}
	return PANGU_OK;
}

/* Write the record. */
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
	hrec->size = hrec->key_size + hrec->value_size + 48;
	memcpy(hbuf + 40, &hrec->size, sizeof(lnum));
	memcpy(hbuf + 48, hrec->key, hrec->key_size);
	memcpy(hbuf + 48 + hrec->key_size, hrec->value, hrec->value_size);
	return storage_engine_seekwrite(fd, hrec->off, hbuf, hrec->size);
}

/* Read size of the record. */
int storage_engine_read_record_size(int fd, record_t *hrec, off_t off, uint64_t *psize) {
	return storage_engine_seekread(fd, off + 40, psize, sizeof(uint64_t));
}

/* Read size of the record. */
int storage_engine_read_record(record_t *hrec, int fd, uint64_t off) {
	uint64_t hsiz = 0;
	if (storage_engine_read_record_size(fd, hrec, off, &hsiz) != PANGU_OK) {
		error_msg(PANGU_SEEK_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_SEEK_FILE_FAIL;
	}	
	char hbuf[HDBIOBUF];
	memset(hbuf, 0, HDBIOBUF);
	char *p = hbuf;
	if (storage_engine_seekread(fd, off, hbuf, hsiz) != PANGU_OK) {
		error_msg(PANGU_SEEK_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_SEEK_FILE_FAIL;
	}
	hrec->off = *(uint64_t*)p;
	p += 8;
	hrec->prev_off = *(uint64_t*)p;
	p += 8;
	hrec->next_off = *(uint64_t*)p;
	p += 8;
	hrec->key_size = *(uint64_t*)p;
	p += 8;
	hrec->value_size = *(uint64_t*)p;
	p += 8;
	hrec->size = *(uint64_t*)p;
	p += 8;
	hrec->key = (char*)pangu_malloc(hrec->key_size + 1);
	if (!hrec->key) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	memcpy(hrec->key, p, hrec->key_size);
	hrec->key[hrec->key_size] = '\0';
	p += hrec->key_size;
	hrec->value = (char*)pangu_malloc(hrec->value_size + 1);
	if (!hrec->value) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	memcpy(hrec->value, p, hrec->value_size);
	hrec->value[hrec->value_size] = '\0';
	p += hrec->value_size;
	return PANGU_OK;
}

/* Seek and read data into a file. */
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

/* Seek and Write data into a file. */
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

/* Read data into a file. */
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
			case 0 : 
				return size < 1 ? PANGU_OK : PANGU_READ_FILE_FAIL;
			default :
				p += read_size;
				size -= read_size;
				break;
		}
	} while(size > 0);
	return PANGU_OK;
}

/* Get a record in a hash database object. */
int storage_engine_get(database_t *hdb, const void *key_buf, size_t key_size, char *value_buf, size_t *value_size) {
	assert(key_buf && key_size >= 0 && value_buf && value_size >= 0);
	uint64_t bucketid = storage_engine_hash2(hdb, key_buf, key_size);
	uint64_t off = storage_engine_get_bucket(hdb, bucketid);
	if (!off) {
		return PANGU_GET_FAIL;
	}
	record_t *hrec = (record_t*)pangu_malloc(sizeof(record_t));
	if (!hrec) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	while (off > 0) {
		if (storage_engine_read_record(hrec, hdb->fd, off) != PANGU_OK) {
			error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
			return PANGU_READ_FILE_FAIL;
		}
		if (!strcmp(hrec->key, key_buf)) {
			memcpy(value_buf, hrec->value, hrec->value_size);
			value_buf[hrec->value_size] = '\0';
			*value_size = hrec->value_size;
			return PANGU_OK;
		}
		off = hrec->next_off;
		pangu_free(hrec->key);
		pangu_free(hrec->value);
	} /* end while */
	pangu_free(hrec);
	return PANGU_GET_FAIL;
}

/* Remove a record of a database object. */
int storage_engine_remove(database_t *hdb, const void *key_buf, size_t key_size) {
	assert(key_buf && key_size >= 0);
	uint64_t bucketid = storage_engine_hash2(hdb, key_buf, key_size);
	uint64_t off = storage_engine_get_bucket(hdb, bucketid);
	if (!off) {
		return PANGU_REMOVE_FAIL;
	}
	record_t *hrec = (record_t*)pangu_malloc(sizeof(record_t));
	if (!hrec) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	record_t *prev;
	while (off > 0) {
		if (storage_engine_read_record(hrec, hdb->fd, off) != PANGU_OK) {
			error_msg(PANGU_READ_FILE_FAIL, __FILE__, __LINE__, __func__);
			return PANGU_READ_FILE_FAIL;
		}
		prev = (char*)pangu_malloc(sizeof(record_t));
		if (!prev) {
			error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
			return PANGU_MEMORY_NOT_ENOUGH;
		}
		if (!strcmp(hrec->key, key_buf) && hrec->next_off) {
			prev->next_off = hrec->next_off;
			/* write prev record*/
			if (storage_engine_write_currecord_nextoff(hdb->fd, prev->off, prev) != PANGU_OK) {
				error_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__);
				return PANGU_WRITE_FILE_FAIL;
			}
			break;
		}
		if (!hrec->next_off) {
			hdb->hashtable[bucketid] = 0;
			break;
		}
		if (storage_engine_copy_record(hrec, prev) != PANGU_OK) {
			error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
			return PANGU_MEMORY_NOT_ENOUGH;
		}
		off = hrec->next_off;
		pangu_free(hrec->key);
		pangu_free(hrec->value);
		pangu_free(prev);
	}
	pangu_free(hrec->key);
	pangu_free(hrec->value);
	pangu_free(prev);
	pangu_free(hrec);
	return PANGU_OK;
}

/* Copy a record. */
int storage_engine_copy_record(record_t *src, record_t *des) {
	assert(src && des);
	des->off = src->off;
	des->prev_off = src->prev_off;
	des->next_off = src->next_off;
	des->key_size = src->key_size;
	des->value_size = src->value_size;
	des->size = src->size;
	des->key = (char*)pangu_malloc(sizeof(des->key_size) + 1);
	if (!des->key) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	memcpy(des->key, src->key, des->key_size);
	des->key[des->key_size] = '\0';
	des->value = (char*)pangu_malloc(sizeof(des->value_size) + 1);
	if (!des->value) {
		error_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	memcpy(des->value, src->value, des->value_size);
	des->value[des->value_size] = '\0';
	return PANGU_OK;
}
