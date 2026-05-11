//
// metamod-p - tests for osdep.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "osdep.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// my_strlwr tests (Linux only)
// ============================================================

static int test_strlwr_null(void)
{
	TEST("my_strlwr - NULL returns NULL");
	ASSERT_PTR_NULL(my_strlwr(NULL));
	PASS();
	return 0;
}

static int test_strlwr_empty(void)
{
	TEST("my_strlwr - empty string");
	char s[] = "";
	ASSERT_STR(my_strlwr(s), "");
	PASS();
	return 0;
}

static int test_strlwr_mixed(void)
{
	TEST("my_strlwr - mixed case");
	char s[] = "Hello WORLD 123!";
	ASSERT_STR(my_strlwr(s), "hello world 123!");
	PASS();
	return 0;
}

// ============================================================
// safe_snprintf / safe_vsnprintf tests
// ============================================================

static int test_safe_snprintf_normal(void)
{
	TEST("safe_snprintf - normal format");
	char buf[64];
	int ret = safe_snprintf(buf, sizeof(buf), "hello %s %d", "world", 42);
	ASSERT_STR(buf, "hello world 42");
	ASSERT_INT(ret, 14);
	PASS();
	return 0;
}

static int test_safe_snprintf_truncate(void)
{
	TEST("safe_snprintf - truncation");
	char buf[8];
	int ret = safe_snprintf(buf, sizeof(buf), "hello world");
	ASSERT_TRUE(ret == 11);
	ASSERT_TRUE(buf[6] == 'w');
	ASSERT_TRUE(buf[7] == '\0');
	PASS();
	return 0;
}

static int test_safe_snprintf_zero_size(void)
{
	TEST("safe_snprintf - zero size buffer");
	int ret = safe_snprintf(NULL, 0, "hello %s", "world");
	ASSERT_TRUE(ret > 0);
	PASS();
	return 0;
}

static int test_safe_snprintf_null_dest(void)
{
	TEST("safe_snprintf - NULL dest with n>0");
	int ret = safe_snprintf(NULL, 10, "hello");
	ASSERT_INT(ret, -1);
	PASS();
	return 0;
}

static int test_safe_snprintf_empty_format(void)
{
	TEST("safe_snprintf - empty format string");
	char buf[16] = "garbage";
	int ret = safe_snprintf(buf, sizeof(buf), "");
	ASSERT_INT(ret, 0);
	ASSERT_STR(buf, "");
	PASS();
	return 0;
}

static int test_safe_snprintf_null_format(void)
{
	TEST("safe_snprintf - NULL format");
	char buf[16] = "garbage";
	int ret = safe_snprintf(buf, sizeof(buf), NULL);
	ASSERT_INT(ret, 0);
	ASSERT_STR(buf, "");
	PASS();
	return 0;
}

static int test_safe_snprintf_size_one(void)
{
	TEST("safe_snprintf - size 1 buffer");
	char buf[1] = { 'X' };
	int ret = safe_snprintf(buf, 1, "hello");
	ASSERT_TRUE(ret > 0);
	ASSERT_TRUE(buf[0] == '\0');
	PASS();
	return 0;
}

static int test_safe_snprintf_exact_fit(void)
{
	TEST("safe_snprintf - exact fit (res == n)");
	char buf[5];
	int ret = safe_snprintf(buf, 5, "1234");
	ASSERT_INT(ret, 4);
	ASSERT_STR(buf, "1234");
	PASS();
	return 0;
}

static int test_safe_snprintf_zero_size_with_result(void)
{
	TEST("safe_snprintf - zero size copies via tmpbuf");
	char buf[4] = "old";
	int ret = safe_snprintf(buf, 0, "hi");
	ASSERT_TRUE(ret == 2);
	ASSERT_STR(buf, "old");
	PASS();
	return 0;
}

static int test_safe_snprintf_small_buf_long_str(void)
{
	TEST("safe_snprintf - small buf, long format via tmpbuf");
	char buf[4];
	int ret = safe_snprintf(buf, sizeof(buf), "%s",
		"this is a fairly long string that exceeds the buffer");
	ASSERT_TRUE(ret > 3);
	ASSERT_TRUE(buf[3] == '\0');
	PASS();
	return 0;
}

// ============================================================
// safevoid_snprintf tests
// ============================================================

static int test_safevoid_snprintf_normal(void)
{
	TEST("safevoid_snprintf - normal format");
	char buf[64];
	safevoid_snprintf(buf, sizeof(buf), "test %d", 123);
	ASSERT_STR(buf, "test 123");
	PASS();
	return 0;
}

static int test_safevoid_snprintf_truncate(void)
{
	TEST("safevoid_snprintf - truncation null-terminates");
	char buf[6];
	safevoid_snprintf(buf, sizeof(buf), "hello world");
	ASSERT_TRUE(buf[5] == '\0');
	ASSERT_TRUE(strlen(buf) == 5);
	PASS();
	return 0;
}

static int test_safevoid_snprintf_null_dest(void)
{
	TEST("safevoid_snprintf - NULL dest does not crash");
	safevoid_snprintf(NULL, 10, "test");
	PASS();
	return 0;
}

static int test_safevoid_snprintf_zero_size(void)
{
	TEST("safevoid_snprintf - zero size does not crash");
	char buf[4] = "abc";
	safevoid_snprintf(buf, 0, "test");
	ASSERT_STR(buf, "abc");
	PASS();
	return 0;
}

static int test_safevoid_snprintf_empty_format(void)
{
	TEST("safevoid_snprintf - empty format");
	char buf[16] = "garbage";
	safevoid_snprintf(buf, sizeof(buf), "");
	ASSERT_STR(buf, "");
	PASS();
	return 0;
}

static int test_safevoid_snprintf_null_format(void)
{
	TEST("safevoid_snprintf - NULL format");
	char buf[16] = "garbage";
	safevoid_snprintf(buf, sizeof(buf), NULL);
	ASSERT_STR(buf, "");
	PASS();
	return 0;
}

// ============================================================
// DLFNAME tests
// ============================================================

static int test_dlfname_known_function(void)
{
	TEST("DLFNAME - known function returns non-NULL");
	const char *name = DLFNAME((void *)test_dlfname_known_function);
	ASSERT_PTR_NOT_NULL(name);
	ASSERT_TRUE(strlen(name) > 0);
	PASS();
	return 0;
}

static int test_dlfname_null(void)
{
	TEST("DLFNAME - NULL returns NULL with ME_NOTFOUND");
	const char *name = DLFNAME(NULL);
	ASSERT_PTR_NULL(name);
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

// ============================================================
// IS_VALID_PTR tests
// ============================================================

static int test_is_valid_ptr_valid(void)
{
	TEST("IS_VALID_PTR - valid function pointer");
	ASSERT_TRUE(IS_VALID_PTR((void *)test_is_valid_ptr_valid) == mTRUE);
	PASS();
	return 0;
}

static int test_is_valid_ptr_null(void)
{
	TEST("IS_VALID_PTR - NULL returns mFALSE");
	ASSERT_TRUE(IS_VALID_PTR(NULL) == mFALSE);
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

// ============================================================
// os_safe_call tests
// ============================================================

static int safe_call_counter;
static void safe_call_test_fn(void) { safe_call_counter++; }

static int test_os_safe_call_valid(void)
{
	TEST("os_safe_call - valid function executes");
	safe_call_counter = 0;
	ASSERT_TRUE(os_safe_call(safe_call_test_fn) == mTRUE);
	ASSERT_INT(safe_call_counter, 1);
	PASS();
	return 0;
}

static int test_os_safe_call_null(void)
{
	TEST("os_safe_call - NULL function returns mFALSE");
	ASSERT_TRUE(os_safe_call(NULL) == mFALSE);
	PASS();
	return 0;
}

static int test_safe_snprintf_res_equals_n(void)
{
	TEST("safe_snprintf - result exactly equals buffer size");
	char buf[5];
	int ret = safe_snprintf(buf, 5, "12345");
	ASSERT_INT(ret, 5);
	ASSERT_TRUE(buf[4] == '\0');
	ASSERT_TRUE(buf[3] == '4');
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

	printf("test_osdep:\n");

	fail |= test_strlwr_null();
	fail |= test_strlwr_empty();
	fail |= test_strlwr_mixed();

	fail |= test_safe_snprintf_normal();
	fail |= test_safe_snprintf_truncate();
	fail |= test_safe_snprintf_zero_size();
	fail |= test_safe_snprintf_null_dest();
	fail |= test_safe_snprintf_empty_format();
	fail |= test_safe_snprintf_null_format();
	fail |= test_safe_snprintf_size_one();
	fail |= test_safe_snprintf_exact_fit();
	fail |= test_safe_snprintf_zero_size_with_result();
	fail |= test_safe_snprintf_small_buf_long_str();
	fail |= test_safe_snprintf_res_equals_n();

	fail |= test_safevoid_snprintf_normal();
	fail |= test_safevoid_snprintf_truncate();
	fail |= test_safevoid_snprintf_null_dest();
	fail |= test_safevoid_snprintf_zero_size();
	fail |= test_safevoid_snprintf_empty_format();
	fail |= test_safevoid_snprintf_null_format();

	fail |= test_dlfname_known_function();
	fail |= test_dlfname_null();

	fail |= test_is_valid_ptr_valid();
	fail |= test_is_valid_ptr_null();

	fail |= test_os_safe_call_valid();
	fail |= test_os_safe_call_null();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
