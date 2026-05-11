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
// Tests for strncasematch
// ============================================================

static int test_strncasematch_prefix(void)
{
	TEST("strncasematch ignores case for prefix");
	ASSERT_TRUE(strncasematch("Hello World", "hello", 5));
	ASSERT_TRUE(strncasematch("HELLO", "hello", 5));

	PASS();
	return 0;
}

static int test_strncasematch_null(void)
{
	TEST("strncasematch returns false for NULL");
	ASSERT_FALSE(strncasematch(NULL, "hello", 5));
	ASSERT_FALSE(strncasematch("hello", NULL, 5));

	PASS();
	return 0;
}

// ============================================================
// Tests for valid_gamedir_file
// ============================================================

static int test_valid_gamedir_file_null(void)
{
	TEST("valid_gamedir_file - NULL returns FALSE");
	ASSERT_INT(valid_gamedir_file(NULL), FALSE);
	PASS();
	return 0;
}

static int test_valid_gamedir_file_devnull(void)
{
	TEST("valid_gamedir_file - /dev/null returns TRUE");
	ASSERT_INT(valid_gamedir_file("/dev/null"), TRUE);
	PASS();
	return 0;
}

static int test_valid_gamedir_file_absolute_exists(void)
{
	TEST("valid_gamedir_file - absolute path to existing file");
	const char *path = make_tmp_file("content");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_INT(valid_gamedir_file(path), TRUE);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_valid_gamedir_file_absolute_missing(void)
{
	TEST("valid_gamedir_file - absolute path to nonexistent file");
	ASSERT_INT(valid_gamedir_file("/tmp/metamod_test_nonexistent_xyz"), FALSE);
	PASS();
	return 0;
}

static int test_valid_gamedir_file_empty_file(void)
{
	TEST("valid_gamedir_file - empty file returns FALSE");
	const char *path = make_tmp_file("");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_INT(valid_gamedir_file(path), FALSE);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_valid_gamedir_file_directory(void)
{
	TEST("valid_gamedir_file - directory returns FALSE");
	ASSERT_INT(valid_gamedir_file("/tmp"), FALSE);
	PASS();
	return 0;
}

static int test_valid_gamedir_file_relative(void)
{
	TEST("valid_gamedir_file - relative path uses gamedir");
	const char *path = make_tmp_file("data");
	ASSERT_PTR_NOT_NULL(path);
	const char *basename = strrchr(path, '/');
	ASSERT_PTR_NOT_NULL(basename);
	basename++;
	STRNCPY(GameDLL.gamedir, "/tmp", sizeof(GameDLL.gamedir));
	ASSERT_INT(valid_gamedir_file(basename), TRUE);
	memset(&GameDLL, 0, sizeof(GameDLL));
	cleanup_tmp_file();
	PASS();
	return 0;
}

// ============================================================
// Tests for full_gamedir_path
// ============================================================

static int test_full_gamedir_path_absolute(void)
{
	TEST("full_gamedir_path - absolute path passes through");
	char result[PATH_MAX];
	full_gamedir_path("/tmp", result);
	ASSERT_STR(result, "/tmp");
	PASS();
	return 0;
}

static int test_full_gamedir_path_relative(void)
{
	TEST("full_gamedir_path - relative path prepends gamedir");
	char result[PATH_MAX];
	STRNCPY(GameDLL.gamedir, "/tmp", sizeof(GameDLL.gamedir));
	full_gamedir_path(".", result);
	ASSERT_STR(result, "/tmp");
	memset(&GameDLL, 0, sizeof(GameDLL));
	PASS();
	return 0;
}

static int test_full_gamedir_path_nonexistent(void)
{
	TEST("full_gamedir_path - nonexistent falls back to input");
	char result[PATH_MAX];
	full_gamedir_path("/no/such/path/xyz", result);
	ASSERT_STR(result, "/no/such/path/xyz");
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
	fail |= test_strncasematch_prefix();
	fail |= test_strncasematch_null();

	fail |= test_valid_gamedir_file_null();
	fail |= test_valid_gamedir_file_devnull();
	fail |= test_valid_gamedir_file_absolute_exists();
	fail |= test_valid_gamedir_file_absolute_missing();
	fail |= test_valid_gamedir_file_empty_file();
	fail |= test_valid_gamedir_file_directory();
	fail |= test_valid_gamedir_file_relative();

	fail |= test_full_gamedir_path_absolute();
	fail |= test_full_gamedir_path_relative();
	fail |= test_full_gamedir_path_nonexistent();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
