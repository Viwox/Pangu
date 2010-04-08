#include <stdlib.h>
#include <stdio.h>
#include "storage_engine.h"
#include "pangu.h"

/* global varilables */
const char *g_progname = "'Pangu storage engine'"; /* program name */
const size_t value_size = 1024;

/* function prototypes */
void usage(void);
int procversion(void);
int runversion(int argc, char **argv);

/* main routine */
int main(int argc, char *argv[]) {
	if(argc < 2) usage();
	database_t pgdb;
	if (storage_engine_open(&pgdb, 1024, "pangu.db")) {
		error_msg(PANGU_OPEN_FILE_FAIL, __FILE__, __LINE__, __func__);
		return PANGU_OPEN_FILE_FAIL;
	}
	int rv = 0;
	char value[value_size];
	if(!strcmp(argv[1], "set")) {
		if ((rv = storage_engine_set(&pgdb, argv[2], strlen(argv[2]), argv[3], strlen(argv[3]))) != PANGU_OK)
			printf("set key = %s, value = %s ERROR.\n", argv[2], argv[3]);
		else
			printf("set key = %s, value = %s OK.\n", argv[2], argv[3]);
	} else if(!strcmp(argv[1], "get")) {
		size_t size = 0;
		memset(value, 0x0, sizeof(value));
		if ((rv = storage_engine_get(&pgdb, argv[2], strlen(argv[2]), value, &size)) != PANGU_OK)
			printf("get key = %s ERROR.\n", argv[2]);
		else
			printf("get key = %s, value = %s OK.\n", argv[2], value);
	} else if(!strcmp(argv[1], "remove")) {
		if ((rv = storage_engine_remove(&pgdb, argv[2], strlen(argv[2]))) != PANGU_OK)
			printf("remove key = %s ERROR.\n", argv[2]);
		else
			printf("remove key = %s OK.\n", argv[2]);
	} else if(!strcmp(argv[1], "version") || !strcmp(argv[1], "--version")) {
		rv = runversion(argc, argv);
	} else {
    		usage();
	}
	return rv;
}

/* print the usage and exit */
void usage(void) {
	fprintf(stderr, "%s: the command line utility of the Pangu database API\n", g_progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "  %s set key value\n", g_progname);
	fprintf(stderr, "  %s remove key\n", g_progname);
	fprintf(stderr, "  %s get key\n", g_progname);
	fprintf(stderr, "  %s version\n", g_progname);
	fprintf(stderr, "\n");
	exit(1);
}

/* print version */
int runversion(int argc, char **argv) {
	int rv = procversion();
	return rv;
}

int procversion(void) {
	printf("%s\n", PANGU_VERSION);
	printf("Copyright (C) 2010-2011 viwox, leon.hong\n");
	return 0;
}
