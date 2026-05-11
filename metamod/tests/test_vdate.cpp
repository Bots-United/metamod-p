//
// metamod-p - tests for vdate.cpp
//

#include <stdlib.h>
#include <string.h>

#include "info_name.h"
#include "vdate.h"

#include "test_common.h"

// Pull in the source under test
#include "../vdate.cpp"

static int test_compile_time_set(void)
{
	TEST("COMPILE_TIME is non-NULL and non-empty");
	ASSERT_PTR_NOT_NULL(COMPILE_TIME);
	ASSERT_TRUE(strlen(COMPILE_TIME) > 0);

	PASS();
	return 0;
}

static int test_compile_tzone_set(void)
{
	TEST("COMPILE_TZONE is non-NULL and non-empty");
	ASSERT_PTR_NOT_NULL(COMPILE_TZONE);
	ASSERT_TRUE(strlen(COMPILE_TZONE) > 0);

	PASS();
	return 0;
}

int main(void)
{
	int fail = 0;

	printf("test_vdate:\n");
	fail |= test_compile_time_set();
	fail |= test_compile_tzone_set();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
