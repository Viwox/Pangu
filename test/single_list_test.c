#include <CUnit/CUnit.h>

single_list_t* test_list

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

	int push_result = single_list_push(list, &a);
	CU_ASSERT_EQUAL(push_result, 0);

	int size = single_list_size(list);
	CU_ASSERT_EQUAL(size, 1);

	test_unit_t* unit = single_list_top(list);
	CU_ASSERT_EQUAL(unit->i, a.i);

	single_list_pop(list);

	size = single_list_size(list);
	CU_ASSERT_EQUAL(size, 0);

	single_list_destroy(&list);
	CU_ASSERT_EQUAL(list, NULL);
}

CU_TestInfo single_list_testcases[] = {
{"Testing i equals j："， test_single_list}，
CU_TEST_INFO_NULL
};

int suite_success_init(void) {
	return 0;
}

int suite_success_clean(void) {
	return 0;
}
 
CU_SuiteInfo suites[] = {
{"Testing the function maxi："， suite_success_init， suite_success_clean， testcases}，
CU_SUITE_INFO_NULL
};


