#ifndef _PANGU_H_
#define _PANGU_H_

#define PANGU_VERSION "Pangu version 1.0"
#define PANGU_OK 0
#define PANGU_ERROR -1
#define PANGU_MEMORY_NOT_ENOUGH -2
#define PANGU_OPEN_FILE_FAIL -3
#define PANGU_WRITE_FILE_FAIL -4
#define PANGU_STAT_FILE_FAIL -5
#define pangu_malloc malloc
#define pangu_free free
#define HDBIOBUF 1024

void error_msg(int ecode, const char *filename, int line, const char *func) {
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

#endif
