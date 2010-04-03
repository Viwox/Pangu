#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "CUnit/CUnit.h"
#include "CUnit/Automated.h"
#include "../src/single_list.h"
single_list_t* test_list;

typedef struct{
	int i;
} test_unit_t;

void test_single_list() {
	single_list_t* list = NULL;

	int create_result = single_list_create(&list);

	CU_ASSERT_EQUAL(create_result, 0);
	CU_ASSERT_NOT_EQUAL(list, NULL);

	test_unit_t a;
	a.i = 1;

	int push_result = single_list_push_back(list, &a);
	CU_ASSERT_EQUAL(push_result, 0);

	int size = single_list_size(list);
	CU_ASSERT_EQUAL(size, 1);

	test_unit_t* unit = single_list_front(list);
	CU_ASSERT_EQUAL(unit->i, a.i);

	single_list_pop_front(list);

	size = single_list_size(list);
	CU_ASSERT_EQUAL(size, 0);

	single_list_destroy(&list);
	CU_ASSERT_EQUAL(list, NULL);
}

CU_TestInfo single_list_testcases[] = {
		{"Testing single_list: ", test_single_list},
		CU_TEST_INFO_NULL
};

int suite_success_init(void) {
	return 0;
}

int suite_success_clean(void) {
	return 0;
}
 
CU_SuiteInfo suites[] = {
	{"Testing the function result: ",  suite_success_init, suite_success_clean, single_list_testcases},
	CU_SUITE_INFO_NULL
};

void AddTests(void) {
	assert(NULL != CU_get_registry());
	assert(!CU_is_test_running());
	if(CUE_SUCCESS != CU_register_suites(suites)) {
		fprintf(stderr, "Register suites failed - %s ", CU_get_error_msg());
		exit(EXIT_FAILURE);
	}
}


int main( int argc, char *argv[] ) {
	if(CU_initialize_registry()) {
		fprintf(stderr, " Initialization of Test Registry failed. ");
		exit(EXIT_FAILURE);
	}else{
		AddTests();
		CU_set_output_filename("Test single");
		CU_list_tests_to_file();
		CU_automated_run_tests();
		CU_cleanup_registry();
	}
	return 0;
}
