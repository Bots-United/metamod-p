//
// metamod-p - tests for support_meta.h / support_meta.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "support_meta.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Tests for STRNCPY
// ============================================================

static int test_strncpy_basic(void)
{
	char buf[64];

	TEST("STRNCPY copies string");
	STRNCPY(buf, "hello", sizeof(buf));
	ASSERT_STR(buf, "hello");

	PASS();
	return 0;
}

static int test_strncpy_truncate(void)
{
	char buf[4];

	TEST("STRNCPY truncates to size-1");
	STRNCPY(buf, "hello world", sizeof(buf));
	ASSERT_INT((int)strlen(buf), 3);

	PASS();
	return 0;
}

static int test_strncpy_empty(void)
{
	char buf[64];

	TEST("STRNCPY handles empty string");
	STRNCPY(buf, "", sizeof(buf));
	ASSERT_STR(buf, "");

	PASS();
	return 0;
}

// ============================================================
// Tests for strmatch
// ============================================================

static int test_strmatch_equal(void)
{
	TEST("strmatch returns true for equal strings");
	ASSERT_TRUE(strmatch("hello", "hello"));

	PASS();
	return 0;
}

static int test_strmatch_different(void)
{
	TEST("strmatch returns false for different strings");
	ASSERT_FALSE(strmatch("hello", "world"));

	PASS();
	return 0;
}

static int test_strmatch_null(void)
{
	TEST("strmatch returns false for NULL args");
	ASSERT_FALSE(strmatch(NULL, "hello"));
	ASSERT_FALSE(strmatch("hello", NULL));
	ASSERT_FALSE(strmatch(NULL, NULL));

	PASS();
	return 0;
}

// ============================================================
// Tests for strnmatch
// ============================================================

static int test_strnmatch_prefix(void)
{
	TEST("strnmatch matches prefix");
	ASSERT_TRUE(strnmatch("hello world", "hello", 5));

	PASS();
	return 0;
}

static int test_strnmatch_null(void)
{
	TEST("strnmatch returns false for NULL");
	ASSERT_FALSE(strnmatch(NULL, "hello", 5));
	ASSERT_FALSE(strnmatch("hello", NULL, 5));

	PASS();
	return 0;
}

// ============================================================
// Tests for strcasematch
// ============================================================

static int test_strcasematch_equal(void)
{
	TEST("strcasematch ignores case");
	ASSERT_TRUE(strcasematch("Hello", "hello"));
	ASSERT_TRUE(strcasematch("HELLO", "hello"));

	PASS();
	return 0;
}

static int test_strcasematch_different(void)
{
	TEST("strcasematch returns false for different strings");
	ASSERT_FALSE(strcasematch("hello", "world"));

	PASS();
	return 0;
}

static int test_strcasematch_null(void)
{
	TEST("strcasematch returns false for NULL");
	ASSERT_FALSE(strcasematch(NULL, "hello"));
	ASSERT_FALSE(strcasematch("hello", NULL));

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

	printf("test_support_meta:\n");
	fail |= test_strncpy_basic();
	fail |= test_strncpy_truncate();
	fail |= test_strncpy_empty();
	fail |= test_strmatch_equal();
	fail |= test_strmatch_different();
	fail |= test_strmatch_null();
	fail |= test_strnmatch_prefix();
	fail |= test_strnmatch_null();
	fail |= test_strcasematch_equal();
	fail |= test_strcasematch_different();
	fail |= test_strcasematch_null();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
