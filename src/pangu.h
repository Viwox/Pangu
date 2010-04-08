#ifndef _PANGU_H_
#define _PANGU_H_

#define PANGU_VERSION "Pangu version 1.0"
#define PANGU_OK 0
#define PANGU_ERROR -1
#define PANGU_MEMORY_NOT_ENOUGH -2
#define PANGU_OPEN_FILE_FAIL -3
#define PANGU_WRITE_FILE_FAIL -4
#define PANGU_READ_FILE_FAIL -5
#define PANGU_STAT_FILE_FAIL -6
#define PANGU_SEEK_FILE_FAIL -7
#define PANGU_SET_FAIL -8
#define PANGU_GET_FAIL -9
#define PANGU_REMOVE_FAIL -10
#define pangu_malloc malloc
#define pangu_free free
#define HDBIOBUF 8188

void error_msg(int ecode, const char *filename, int line, const char *func);

#endif
