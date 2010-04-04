
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
#include "pangu.h"
#include "storage_engine.h"

int storage_engine_init(database_t *hdb, uint64_t bucket_num, char *path) {
	hdb->bucket_num = bucket_num;
	if (storage_engine_str_dup(path, &hdb->path) != PANGU_OK) {
		storage_engine_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	int fd = open(path, O_RDWR|O_CREAT, HDBFILEMODE);
	if (fd < 0) {
		storage_engine_msg(PANGU_OPEN_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_OPEN_FILE_FAIL;
	}
	struct stat sbuf;
	if (fstat(fd, &sbuf) == -1) {
		storage_engine_msg(PANGU_STAT_FILE_FAIL, __FILE__, __LINE__, __func__); 
		return PANGU_STAT_FILE_FAIL;
	}
	hdb->mtime = sbuf.st_mtime;
	hdb->fd = fd;
	hdb->file_size = HDBHEADERSIZE + hdb->bucket_num * sizeof(uint64_t);
	hdb->first_rec_off = hdb->file_size;
	hdb->last_rec_off = hdb->file_size;
	char buf[HDBIOBUF];
	storage_engine_dup_meta(hdb, buf);
	if (storage_engine_write(fd, buf, HDBIOBUF) != PANGU_OK) {
		storage_engine_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
		return PANGU_WRITE_FILE_FAIL;
	}
	hdb->map_size = HDBHEADERSIZE + hdb->bucket_num * sizeof(uint64_t);
	void *map = mmap(0, hdb->map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	hdb->map = map;
	hdb->hashtable = (uint32_t*)((char*)map + HDBHEADERSIZE);
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
					storage_engine_msg(PANGU_WRITE_FILE_FAIL, __FILE__, __LINE__, __func__); 
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
		storage_engine_msg(PANGU_MEMORY_NOT_ENOUGH, __FILE__, __LINE__, __func__);
		return PANGU_MEMORY_NOT_ENOUGH;
	}
	memcpy(p, str, size);
	p[size] = '\0';
	*res = p;
	return PANGU_OK;
}

void storage_engine_msg(int ecode, const char *filename, int line, const char *func) {
	char err_msg[HDBIOBUF];
	switch(ecode) {
		case PANGU_MEMORY_NOT_ENOUGH : 
			strcpy(err_msg, "malloc error!\n"); 
			break;
		case PANGU_OPEN_FILE_FAIL :
			strcpy(err_msg, "file open error!\n");
			break;
		case PANGU_WRITE_FILE_FAIL :
			strcpy(err_msg, "file write error!\n");
			break;
		case PANGU_STAT_FILE_FAIL :
			strcpy(err_msg, "file stat error!\n");
			break;
		default :
			strcpy(err_msg, "unknown error!\n");
			break;
	}
	char buf[HDBIOBUF];
	sprintf(buf, "ERROR :%s:%d:%s:%d:%s\n", filename, line, func, ecode, err_msg);
	printf("%s\n", buf);
	return;
}
