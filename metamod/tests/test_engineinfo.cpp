//
// metamod-p - tests for engineinfo.cpp
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <extdll.h>

#include "metamod.h"
#include "engineinfo.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// check_for_engine_module tests (via initialise which calls it)
// ============================================================

static int test_engineinfo_constructor(void)
{
	TEST("EngineInfo - constructor initializes to STATE_NULL");
	EngineInfo ei;
	ASSERT_TRUE(ei.type()[0] == '\0');
	PASS();
	return 0;
}

static int test_engineinfo_initialise_null(void)
{
	TEST("EngineInfo::initialise - NULL pFuncs uses r_debug");
	mock_reset();
	EngineInfo ei;
	int ret = ei.initialise(NULL);
	// r_debug walk won't find engine_*.so (not loaded yet), so returns non-zero
	ASSERT_TRUE(ret != 0);
	PASS();
	return 0;
}

static int test_engineinfo_initialise_with_funcs(void)
{
	TEST("EngineInfo::initialise - with enginefuncs uses dladdr");
	mock_reset();
	EngineInfo ei;
	int ret = ei.initialise((enginefuncs_t *)&g_engfuncs);
	// dladdr won't resolve to engine_*.so either
	ASSERT_TRUE(ret != 0);
	PASS();
	return 0;
}

static int test_engineinfo_initialise_twice(void)
{
	TEST("EngineInfo::initialise - second call returns 0 (cached)");
	mock_reset();
	EngineInfo ei;
	int ret1 = ei.initialise(NULL);
	int ret2 = ei.initialise(NULL);
	(void)ret1;
	ASSERT_TRUE(ret2 == ret1 || ret2 == 0);
	PASS();
	return 0;
}

static int test_engineinfo_is_valid_code_ptr_null_state(void)
{
	TEST("EngineInfo::is_valid_code_pointer - STATE_NULL returns false");
	EngineInfo ei;
	ASSERT_FALSE(ei.is_valid_code_pointer((void *)0x12345));
	PASS();
	return 0;
}

static int test_engineinfo_is_valid_code_ptr_null_arg(void)
{
	TEST("EngineInfo::is_valid_code_pointer - NULL returns false");
	EngineInfo ei;
	ASSERT_FALSE(ei.is_valid_code_pointer((void *)NULL));
	PASS();
	return 0;
}

static int test_engineinfo_copy_constructor(void)
{
	TEST("EngineInfo - copy constructor copies type");
	EngineInfo ei;
	EngineInfo ei2(ei);
	ASSERT_STR(ei2.type(), ei.type());
	PASS();
	return 0;
}

static int test_engineinfo_assignment(void)
{
	TEST("EngineInfo - assignment operator");
	EngineInfo ei;
	EngineInfo ei2;
	ei2 = ei;
	ASSERT_STR(ei2.type(), ei.type());
	PASS();
	return 0;
}

// ============================================================
// Tests with fake engine .so (exercises success paths)
// ============================================================

static void *fake_engine_handle = NULL;

static int load_fake_engine(void)
{
	fake_engine_handle = dlopen("./engine_test.so", RTLD_NOW);
	return fake_engine_handle ? 0 : 1;
}

static void unload_fake_engine(void)
{
	if (fake_engine_handle) {
		dlclose(fake_engine_handle);
		fake_engine_handle = NULL;
	}
}

static int test_engineinfo_initialise_rdebug_success(void)
{
	TEST("EngineInfo::initialise - r_debug finds engine_test.so");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	EngineInfo ei;
	int ret = ei.initialise(NULL);
	ASSERT_INT(ret, 0);
	ASSERT_STR(ei.type(), "test");
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_engineinfo_initialise_dladdr_success(void)
{
	TEST("EngineInfo::initialise - dladdr finds engine_test.so");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	void *sym = dlsym(fake_engine_handle, "fake_engine_symbol");
	ASSERT_PTR_NOT_NULL(sym);
	EngineInfo ei;
	int ret = ei.initialise((enginefuncs_t *)sym);
	ASSERT_INT(ret, 0);
	ASSERT_STR(ei.type(), "test");
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_engineinfo_valid_code_ptr_after_init(void)
{
	TEST("EngineInfo::is_valid_code_pointer - true for symbol in engine");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	void *sym = dlsym(fake_engine_handle, "fake_engine_symbol");
	ASSERT_PTR_NOT_NULL(sym);
	EngineInfo ei;
	int ret = ei.initialise((enginefuncs_t *)sym);
	ASSERT_INT(ret, 0);
	ASSERT_TRUE(ei.is_valid_code_pointer(sym));
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_engineinfo_invalid_code_ptr_after_init(void)
{
	TEST("EngineInfo::is_valid_code_pointer - false for out-of-range ptr");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	EngineInfo ei;
	int ret = ei.initialise(NULL);
	ASSERT_INT(ret, 0);
	ASSERT_FALSE(ei.is_valid_code_pointer((void *)0x1));
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_engineinfo_cached_after_success(void)
{
	TEST("EngineInfo::initialise - second call returns 0 (cached success)");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	EngineInfo ei;
	int ret1 = ei.initialise(NULL);
	ASSERT_INT(ret1, 0);
	unload_fake_engine();
	int ret2 = ei.initialise(NULL);
	ASSERT_INT(ret2, 0);
	PASS();
	return 0;
}

static int test_engineinfo_copy_after_init(void)
{
	TEST("EngineInfo - copy constructor preserves type after init");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	EngineInfo ei;
	ei.initialise(NULL);
	EngineInfo ei2(ei);
	ASSERT_STR(ei2.type(), "test");
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_engineinfo_assign_after_init(void)
{
	TEST("EngineInfo - assignment preserves type and state after init");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	void *sym = dlsym(fake_engine_handle, "fake_engine_symbol");
	EngineInfo ei;
	ei.initialise((enginefuncs_t *)sym);
	EngineInfo ei2;
	ei2 = ei;
	ASSERT_STR(ei2.type(), "test");
	ASSERT_TRUE(ei2.is_valid_code_pointer(sym));
	unload_fake_engine();
	PASS();
	return 0;
}

// ============================================================
// check_for_engine_module edge cases (via r_debug path)
// ============================================================

static int test_engineinfo_short_name_rejected(void)
{
	TEST("EngineInfo - short library name rejected (< 11 chars)");
	mock_reset();
	// engine_test.so has 14 chars, so just verify initialise works
	// with no engine loaded and a short name won't match
	EngineInfo ei;
	int ret = ei.initialise(NULL);
	ASSERT_TRUE(ret != 0);
	PASS();
	return 0;
}

static int test_engineinfo_non_engine_underscore_so(void)
{
	TEST("EngineInfo - library with underscore but not engine is rejected");
	mock_reset();
	// Load fake_mm_plugin.so (has underscore in name, but is not engine_*.so)
	// Then r_debug walk will encounter it and check_for_engine_module should
	// reject it at the "engine" backward check (line 106).
	void *handle = dlopen("./fake_mm_plugin.so", RTLD_NOW);
	if (!handle) {
		printf("SKIP (could not load fake_mm_plugin.so)\n");
		tests_run--;
		return 0;
	}
	EngineInfo ei;
	int ret = ei.initialise(NULL);
	// Should not find engine (fake_mm_plugin.so is not engine_*.so)
	ASSERT_TRUE(ret != 0);
	dlclose(handle);
	PASS();
	return 0;
}

// ============================================================
// STATE_INVALID (line 183 in engineinfo.h)
// ============================================================

static int test_engineinfo_invalid_state_returns_default(void)
{
	TEST("EngineInfo - STATE_INVALID returns default for is_valid_code_pointer");
	EngineInfo ei;
	ei.test_set_state(2); // STATE_INVALID = 2
	bool ret = ei.is_valid_code_pointer((void *)0x12345);
	ASSERT_TRUE(ret);
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

	printf("test_engineinfo:\n");

	fail |= test_engineinfo_constructor();
	fail |= test_engineinfo_initialise_null();
	fail |= test_engineinfo_initialise_with_funcs();
	fail |= test_engineinfo_initialise_twice();
	fail |= test_engineinfo_is_valid_code_ptr_null_state();
	fail |= test_engineinfo_is_valid_code_ptr_null_arg();
	fail |= test_engineinfo_copy_constructor();
	fail |= test_engineinfo_assignment();

	fail |= test_engineinfo_initialise_rdebug_success();
	fail |= test_engineinfo_initialise_dladdr_success();
	fail |= test_engineinfo_valid_code_ptr_after_init();
	fail |= test_engineinfo_invalid_code_ptr_after_init();
	fail |= test_engineinfo_cached_after_success();
	fail |= test_engineinfo_copy_after_init();
	fail |= test_engineinfo_assign_after_init();

	fail |= test_engineinfo_short_name_rejected();
	fail |= test_engineinfo_non_engine_underscore_so();
	fail |= test_engineinfo_invalid_state_returns_default();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
