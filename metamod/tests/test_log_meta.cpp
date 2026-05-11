//
// metamod-p - tests for log_meta.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "log_meta.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// META_CONS tests
// ============================================================

static int test_cons_normal(void)
{
	TEST("META_CONS - normal message");
	mock_reset();
	META_CONS("hello %s", "world");
	ASSERT_INT(mock_get_server_print_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "hello world");
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "\n");
	PASS();
	return 0;
}

static int test_cons_long_message(void)
{
	TEST("META_CONS - long message replaces last char with newline");
	mock_reset();
	char longmsg[MAX_LOGMSG_LEN];
	memset(longmsg, 'A', sizeof(longmsg) - 1);
	longmsg[sizeof(longmsg) - 1] = '\0';
	META_CONS("%s", longmsg);
	ASSERT_INT(mock_get_server_print_count(), 1);
	const char *out = mock_get_server_print_msg(0);
	size_t len = strlen(out);
	ASSERT_TRUE(len > 0);
	ASSERT_TRUE(out[len - 1] == '\n');
	PASS();
	return 0;
}

// ============================================================
// META_DEV tests
// ============================================================

static int test_dev_silent_when_zero(void)
{
	TEST("META_DEV - silent when developer=0");
	mock_reset();
	mock_set_cvar_float("developer", 0.0f);
	META_DEV("should not appear");
	ASSERT_INT(mock_get_alert_count(), 0);
	PASS();
	return 0;
}

static int test_dev_logs_when_nonzero(void)
{
	TEST("META_DEV - logs when developer=1");
	mock_reset();
	mock_set_cvar_float("developer", 1.0f);
	META_DEV("dev message %d", 42);
	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "dev:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "dev message 42");
	PASS();
	return 0;
}

// ============================================================
// META_INFO / META_WARNING / META_ERROR / META_LOG tests
// ============================================================

static int test_info_message(void)
{
	TEST("META_INFO - logs with INFO prefix");
	mock_reset();
	META_INFO("info test %d", 1);
	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "INFO:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "info test 1");
	PASS();
	return 0;
}

static int test_warning_message(void)
{
	TEST("META_WARNING - logs with WARNING prefix");
	mock_reset();
	META_WARNING("warn test");
	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "WARNING:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "warn test");
	PASS();
	return 0;
}

static int test_error_message(void)
{
	TEST("META_ERROR - logs with ERROR prefix");
	mock_reset();
	META_ERROR("err test");
	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "ERROR:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "err test");
	PASS();
	return 0;
}

static int test_log_message(void)
{
	TEST("META_LOG - logs with [META] prefix");
	mock_reset();
	META_LOG("log test");
	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "[META]");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "log test");
	PASS();
	return 0;
}

// ============================================================
// META_CLIENT tests
// ============================================================

static edict_t test_edict;

static int test_client_normal(void)
{
	TEST("META_CLIENT - prints to client with newline");
	mock_reset();
	memset(&test_edict, 0, sizeof(test_edict));
	META_CLIENT(&test_edict, "client msg %d", 7);
	ASSERT_INT(mock_get_client_print_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_client_print_msg(0), "client msg 7");
	ASSERT_STR_CONTAINS(mock_get_client_print_msg(0), "\n");
	PASS();
	return 0;
}

static int test_client_long_message(void)
{
	TEST("META_CLIENT - long message replaces last char with newline");
	mock_reset();
	memset(&test_edict, 0, sizeof(test_edict));
	char longmsg[MAX_CLIENTMSG_LEN];
	memset(longmsg, 'B', sizeof(longmsg) - 1);
	longmsg[sizeof(longmsg) - 1] = '\0';
	META_CLIENT(&test_edict, "%s", longmsg);
	ASSERT_INT(mock_get_client_print_count(), 1);
	const char *out = mock_get_client_print_msg(0);
	size_t len = strlen(out);
	ASSERT_TRUE(len > 0);
	ASSERT_TRUE(out[len - 1] == '\n');
	PASS();
	return 0;
}

// ============================================================
// META_DEBUG tests
// ============================================================

static int test_debug_silent_when_low(void)
{
	TEST("META_DEBUG - silent when debug level too low");
	mock_reset();
	meta_debug_value = 0;
	META_DEBUG(3, ("should not appear"));
	ASSERT_INT(mock_get_alert_count(), 0);
	PASS();
	return 0;
}

static int test_debug_logs_when_level_met(void)
{
	TEST("META_DEBUG - logs when debug level >= threshold");
	mock_reset();
	meta_debug_value = 5;
	META_DEBUG(3, ("debug msg %d", 99));
	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "debug:3");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "debug msg 99");
	PASS();
	return 0;
}

// ============================================================
// Buffered path tests (pfnAlertMessage = NULL)
// ============================================================

static int test_buffered_alert_queues(void)
{
	TEST("buffered_ALERT - queues when pfnAlertMessage is NULL");
	mock_reset();
	g_engfuncs.pfnAlertMessage = NULL;
	META_INFO("buffered msg");
	ASSERT_INT(mock_get_alert_count(), 0);
	// Restore and flush to clean up queue
	mock_reset();
	flush_ALERT_buffer();
	PASS();
	return 0;
}

static int test_buffered_flush_roundtrip(void)
{
	TEST("buffered_ALERT + flush_ALERT_buffer - round trip");
	mock_reset();

	// Save the mock alert function pointer
	void (*saved_alert)(ALERT_TYPE, char *, ...) = g_engfuncs.pfnAlertMessage;

	// Disable to force buffering
	g_engfuncs.pfnAlertMessage = NULL;
	META_INFO("queued info");
	META_WARNING("queued warn");
	ASSERT_INT(mock_get_alert_count(), 0);

	// Restore and flush
	g_engfuncs.pfnAlertMessage = saved_alert;
	flush_ALERT_buffer();

	ASSERT_INT(mock_get_alert_count(), 2);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "INFO:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "queued info");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(1), "WARNING:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(1), "queued warn");
	PASS();
	return 0;
}

static int test_buffered_flush_skips_dev_when_zero(void)
{
	TEST("flush_ALERT_buffer - skips DEV messages when developer=0");
	mock_reset();
	mock_set_cvar_float("developer", 1.0f);

	void (*saved_alert)(ALERT_TYPE, char *, ...) = g_engfuncs.pfnAlertMessage;

	// Buffer a DEV message and an INFO message
	g_engfuncs.pfnAlertMessage = NULL;
	META_DEV("dev buffered");
	META_INFO("info buffered");
	ASSERT_INT(mock_get_alert_count(), 0);

	// Flush with developer=0 -- DEV message should be skipped
	g_engfuncs.pfnAlertMessage = saved_alert;
	mock_set_cvar_float("developer", 0.0f);
	flush_ALERT_buffer();

	ASSERT_INT(mock_get_alert_count(), 1);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "INFO:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "info buffered");
	PASS();
	return 0;
}

static int test_buffered_flush_shows_dev_when_nonzero(void)
{
	TEST("flush_ALERT_buffer - shows DEV messages when developer=1");
	mock_reset();
	mock_set_cvar_float("developer", 1.0f);

	void (*saved_alert)(ALERT_TYPE, char *, ...) = g_engfuncs.pfnAlertMessage;

	g_engfuncs.pfnAlertMessage = NULL;
	META_DEV("dev buffered");
	META_INFO("info buffered");
	ASSERT_INT(mock_get_alert_count(), 0);

	g_engfuncs.pfnAlertMessage = saved_alert;
	flush_ALERT_buffer();

	ASSERT_INT(mock_get_alert_count(), 2);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "dev:");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "dev buffered");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(1), "INFO:");
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

	printf("test_log_meta:\n");

	fail |= test_cons_normal();
	fail |= test_cons_long_message();

	fail |= test_dev_silent_when_zero();
	fail |= test_dev_logs_when_nonzero();

	fail |= test_info_message();
	fail |= test_warning_message();
	fail |= test_error_message();
	fail |= test_log_message();

	fail |= test_client_normal();
	fail |= test_client_long_message();

	fail |= test_debug_silent_when_low();
	fail |= test_debug_logs_when_level_met();

	fail |= test_buffered_alert_queues();
	fail |= test_buffered_flush_roundtrip();
	fail |= test_buffered_flush_skips_dev_when_zero();
	fail |= test_buffered_flush_shows_dev_when_nonzero();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
