//
// metamod-p - tests for conf_meta.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "conf_meta.h"

#include "engine_mock.h"
#include "test_common.h"

// Test option variables
static int test_int_val;
static int test_bool_val;
static char *test_str_val;
static char *test_path_val;

static option_t test_options[] = {
	{(char *)"debug",       CF_INT,  &test_int_val,  (char *)"0"},
	{(char *)"verbose",     CF_BOOL, &test_bool_val, (char *)"false"},
	{(char *)"gamedll",     CF_STR,  &test_str_val,  NULL},
	{(char *)"plugins_file", CF_PATH, &test_path_val, NULL},
	{NULL, CF_NONE, NULL, NULL},
};

static void reset_test_vars(void)
{
	test_int_val = -999;
	test_bool_val = -999;
	if (test_str_val) { free(test_str_val); test_str_val = NULL; }
	if (test_path_val) { free(test_path_val); test_path_val = NULL; }
}

// ============================================================
// MConfig constructor tests
// ============================================================

static int test_config_constructor(void)
{
	TEST("MConfig - constructor zeros fields");
	MConfig cfg;
	ASSERT_PTR_NULL(cfg.gamedll);
	ASSERT_PTR_NULL(cfg.plugins_file);
	ASSERT_PTR_NULL(cfg.exec_cfg);
	ASSERT_INT(cfg.debuglevel, 0);
	PASS();
	return 0;
}

// ============================================================
// MConfig::init tests
// ============================================================

static int test_config_init(void)
{
	TEST("MConfig::init - sets defaults from options table");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_INT(test_int_val, 0);
	ASSERT_INT(test_bool_val, 0);
	ASSERT_PTR_NULL(test_str_val);
	ASSERT_PTR_NULL(test_path_val);
	PASS();
	return 0;
}

// ============================================================
// MConfig::set tests (by key)
// ============================================================

static int test_config_set_int(void)
{
	TEST("MConfig::set - CF_INT sets integer value");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("debug", "42") == mTRUE);
	ASSERT_INT(test_int_val, 42);
	PASS();
	return 0;
}

static int test_config_set_int_invalid(void)
{
	TEST("MConfig::set - CF_INT rejects non-digit");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("debug", "abc") == mFALSE);
	ASSERT_INT(meta_errno, ME_FORMAT);
	PASS();
	return 0;
}

static int test_config_set_bool_true(void)
{
	TEST("MConfig::set - CF_BOOL 'true'/'yes'/'1'");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("verbose", "true") == mTRUE);
	ASSERT_INT(test_bool_val, 1);
	ASSERT_TRUE(cfg.set("verbose", "yes") == mTRUE);
	ASSERT_INT(test_bool_val, 1);
	ASSERT_TRUE(cfg.set("verbose", "1") == mTRUE);
	ASSERT_INT(test_bool_val, 1);
	PASS();
	return 0;
}

static int test_config_set_bool_false(void)
{
	TEST("MConfig::set - CF_BOOL 'false'/'no'/'0'");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	cfg.set("verbose", "true");
	ASSERT_TRUE(cfg.set("verbose", "false") == mTRUE);
	ASSERT_INT(test_bool_val, 0);
	cfg.set("verbose", "true");
	ASSERT_TRUE(cfg.set("verbose", "no") == mTRUE);
	ASSERT_INT(test_bool_val, 0);
	cfg.set("verbose", "true");
	ASSERT_TRUE(cfg.set("verbose", "0") == mTRUE);
	ASSERT_INT(test_bool_val, 0);
	PASS();
	return 0;
}

static int test_config_set_bool_invalid(void)
{
	TEST("MConfig::set - CF_BOOL rejects invalid string");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("verbose", "maybe") == mFALSE);
	ASSERT_INT(meta_errno, ME_FORMAT);
	PASS();
	return 0;
}

static int test_config_set_str(void)
{
	TEST("MConfig::set - CF_STR sets string");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("gamedll", "cs.so") == mTRUE);
	ASSERT_PTR_NOT_NULL(test_str_val);
	ASSERT_STR(test_str_val, "cs.so");
	PASS();
	return 0;
}

static int test_config_set_str_replace(void)
{
	TEST("MConfig::set - CF_STR frees old and replaces");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	cfg.set("gamedll", "old.so");
	ASSERT_TRUE(cfg.set("gamedll", "new.so") == mTRUE);
	ASSERT_STR(test_str_val, "new.so");
	PASS();
	return 0;
}

static int test_config_set_path(void)
{
	TEST("MConfig::set - CF_PATH resolves path");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("plugins_file", "/tmp") == mTRUE);
	ASSERT_PTR_NOT_NULL(test_path_val);
	ASSERT_STR(test_path_val, "/tmp");
	PASS();
	return 0;
}

static int test_config_set_null_value(void)
{
	TEST("MConfig::set - NULL value returns mTRUE (no-op)");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	option_t opt = {(char *)"test", CF_INT, &test_int_val, NULL};
	ASSERT_TRUE(cfg.set("debug", NULL) == mTRUE);
	(void)opt;
	PASS();
	return 0;
}

static int test_config_set_unknown_key(void)
{
	TEST("MConfig::set - unknown key returns mFALSE");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.set("nonexistent", "val") == mFALSE);
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

// ============================================================
// MConfig::load tests
// ============================================================

static int test_config_load_valid(void)
{
	TEST("MConfig::load - loads valid config file");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	const char *path = make_tmp_file("debug 5\nverbose true\n");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(cfg.load(path) == mTRUE);
	ASSERT_INT(test_int_val, 5);
	ASSERT_INT(test_bool_val, 1);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_load_comments(void)
{
	TEST("MConfig::load - skips comments and blank lines");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	const char *path = make_tmp_file("# comment\n; another\n//third\ndebug 7\n");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(cfg.load(path) == mTRUE);
	ASSERT_INT(test_int_val, 7);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_load_missing_file(void)
{
	TEST("MConfig::load - missing file returns mFALSE");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	ASSERT_TRUE(cfg.load("/tmp/metamod_test_nonexistent_xyz") == mFALSE);
	ASSERT_INT(meta_errno, ME_NOFILE);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_load_unknown_option(void)
{
	TEST("MConfig::load - unknown option warns and continues");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	const char *path = make_tmp_file("unknown_key value\ndebug 3\n");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(cfg.load(path) == mTRUE);
	ASSERT_INT(test_int_val, 3);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_load_missing_value(void)
{
	TEST("MConfig::load - missing value warns and continues");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	const char *path = make_tmp_file("debug\nverbose true\n");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(cfg.load(path) == mTRUE);
	ASSERT_INT(test_bool_val, 1);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

// ============================================================
// MConfig::show tests
// ============================================================

static int test_config_show_with_filename(void)
{
	TEST("MConfig::show - with filename loaded");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	const char *path = make_tmp_file("debug 1\n");
	ASSERT_PTR_NOT_NULL(path);
	cfg.load(path);
	cfg.show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_show_without_filename(void)
{
	TEST("MConfig::show - without filename");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	cfg.show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	PASS();
	return 0;
}

static int test_config_load_missing_option_name(void)
{
	TEST("MConfig::load - line with only whitespace is bad format");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	cfg.init(test_options);
	const char *path = make_tmp_file(" \t\ndebug 5\n");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(cfg.load(path) == mTRUE);
	ASSERT_INT(test_int_val, 5);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_set_invalid_type(void)
{
	TEST("MConfig::set - unknown config type returns false");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	option_t bad_options[] = {
		{(char *)"badopt", CF_NONE, &test_int_val, (char *)"0"},
		{NULL, CF_NONE, NULL, NULL},
	};
	cfg.init(bad_options);
	option_t *optp = cfg.test_find("badopt");
	ASSERT_PTR_NOT_NULL(optp);
	ASSERT_TRUE(cfg.test_set(optp, "value") == mFALSE);
	PASS();
	return 0;
}

static int test_config_load_set_fails(void)
{
	TEST("MConfig::load - set failure warns and continues");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	option_t mixed_options[] = {
		{(char *)"badopt", CF_NONE, &test_int_val, (char *)"0"},
		{(char *)"debug", CF_INT, &test_int_val, (char *)"0"},
		{NULL, CF_NONE, NULL, NULL},
	};
	cfg.init(mixed_options);
	const char *path = make_tmp_file("badopt foo\ndebug 7\n");
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(cfg.load(path) == mTRUE);
	ASSERT_INT(test_int_val, 7);
	free(cfg.test_get_filename()); cfg.test_set_filename(NULL);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_config_show_cf_none(void)
{
	TEST("MConfig::show - CF_NONE option is skipped");
	mock_reset();
	reset_test_vars();
	MConfig cfg;
	option_t none_options[] = {
		{(char *)"noop", CF_NONE, NULL, NULL},
		{NULL, CF_NONE, NULL, NULL},
	};
	cfg.init(none_options);
	cfg.show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
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

	printf("test_conf_meta:\n");

	fail |= test_config_constructor();
	fail |= test_config_init();

	fail |= test_config_set_int();
	fail |= test_config_set_int_invalid();
	fail |= test_config_set_bool_true();
	fail |= test_config_set_bool_false();
	fail |= test_config_set_bool_invalid();
	fail |= test_config_set_str();
	fail |= test_config_set_str_replace();
	fail |= test_config_set_path();
	fail |= test_config_set_null_value();
	fail |= test_config_set_unknown_key();

	fail |= test_config_load_valid();
	fail |= test_config_load_comments();
	fail |= test_config_load_missing_file();
	fail |= test_config_load_unknown_option();
	fail |= test_config_load_missing_value();

	fail |= test_config_show_with_filename();
	fail |= test_config_show_without_filename();
	fail |= test_config_load_missing_option_name();
	fail |= test_config_set_invalid_type();
	fail |= test_config_load_set_fails();
	fail |= test_config_show_cf_none();

	reset_test_vars();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
