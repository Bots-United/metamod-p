//
// metamod-p - tests for osdep_p.cpp
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <extdll.h>

#include "metamod.h"
#include "osdep_p.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// get_module_handle_of_memptr tests
// ============================================================

static int test_get_module_handle_known(void)
{
	TEST("get_module_handle_of_memptr - libc function");
	// Use dlsym to get the real libc address, not an ASan interceptor wrapper
	void *real_printf = dlsym(RTLD_NEXT, "printf");
	if(!real_printf)
		real_printf = (void *)printf;
	DLHANDLE h = get_module_handle_of_memptr(real_printf);
	ASSERT_PTR_NOT_NULL(h);
	dlclose(h);
	PASS();
	return 0;
}

static int test_get_module_handle_null(void)
{
	TEST("get_module_handle_of_memptr - NULL returns NULL");
	DLHANDLE h = get_module_handle_of_memptr(NULL);
	ASSERT_PTR_NULL(h);
	PASS();
	return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
	int fail = 0;

	mock_reset();

	printf("test_osdep_p:\n");

	fail |= test_get_module_handle_known();
	fail |= test_get_module_handle_null();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
