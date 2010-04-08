
/*
* Copyright (C) leon.hong
*/

#include "pangu.h"

void error_msg(int ecode, const char *filename, int line, const char *func) {
    char err_msg[HDBIOBUF];
    switch(ecode) {
        case PANGU_MEMORY_NOT_ENOUGH :
            strcpy(err_msg, "malloc error!");
            break;
        case PANGU_OPEN_FILE_FAIL :
            strcpy(err_msg, "file open error!");
            break;
        case PANGU_WRITE_FILE_FAIL :
            strcpy(err_msg, "file write error!");
            break;
        case PANGU_STAT_FILE_FAIL :
            strcpy(err_msg, "file stat error!");
            break;
        default :
            strcpy(err_msg, "unknown error!");
            break;
    }
    char buf[HDBIOBUF];
    sprintf(buf, "ERROR :%s:%d:%s:%d:%s", filename, line, func, ecode, err_msg);
    printf("%s\n", buf);
    exit(1);
}
