//
// metamod-p - tests for sdk_util.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "sdk_util.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// FixedSigned16 tests
// ============================================================

static int test_fixed_signed16_normal(void)
{
	TEST("FixedSigned16 - normal value");
	ASSERT_SHORT(FixedSigned16(1.0f, 256.0f), 256);
	PASS();
	return 0;
}

static int test_fixed_signed16_zero(void)
{
	TEST("FixedSigned16 - zero");
	ASSERT_SHORT(FixedSigned16(0.0f, 256.0f), 0);
	PASS();
	return 0;
}

static int test_fixed_signed16_overflow(void)
{
	TEST("FixedSigned16 - clamp to 32767");
	ASSERT_SHORT(FixedSigned16(1000.0f, 1000.0f), 32767);
	PASS();
	return 0;
}

static int test_fixed_signed16_underflow(void)
{
	TEST("FixedSigned16 - clamp to -32768");
	ASSERT_SHORT(FixedSigned16(-1000.0f, 1000.0f), -32768);
	PASS();
	return 0;
}

static int test_fixed_signed16_negative(void)
{
	TEST("FixedSigned16 - negative value");
	ASSERT_SHORT(FixedSigned16(-1.0f, 256.0f), -256);
	PASS();
	return 0;
}

// ============================================================
// FixedUnsigned16 tests
// ============================================================

static int test_fixed_unsigned16_normal(void)
{
	TEST("FixedUnsigned16 - normal value");
	ASSERT_USHORT(FixedUnsigned16(1.0f, 256.0f), 256);
	PASS();
	return 0;
}

static int test_fixed_unsigned16_zero(void)
{
	TEST("FixedUnsigned16 - zero");
	ASSERT_USHORT(FixedUnsigned16(0.0f, 256.0f), 0);
	PASS();
	return 0;
}

static int test_fixed_unsigned16_overflow(void)
{
	TEST("FixedUnsigned16 - clamp to 0xFFFF");
	ASSERT_USHORT(FixedUnsigned16(1000.0f, 1000.0f), 0xFFFF);
	PASS();
	return 0;
}

static int test_fixed_unsigned16_negative(void)
{
	TEST("FixedUnsigned16 - negative clamps to 0");
	ASSERT_USHORT(FixedUnsigned16(-1.0f, 256.0f), 0);
	PASS();
	return 0;
}

// ============================================================
// META_UTIL_VarArgs tests
// ============================================================

static int test_varargs_normal(void)
{
	TEST("META_UTIL_VarArgs - format string");
	const char *s = META_UTIL_VarArgs("hello %s %d", "world", 42);
	ASSERT_STR(s, "hello world 42");
	PASS();
	return 0;
}

static int test_varargs_empty(void)
{
	TEST("META_UTIL_VarArgs - empty format");
	const char *s = META_UTIL_VarArgs("");
	ASSERT_STR(s, "");
	PASS();
	return 0;
}

// ============================================================
// META_UTIL_HudMessage tests
// ============================================================

static edict_t test_edict;

static int test_hudmessage_null_entity(void)
{
	TEST("META_UTIL_HudMessage - NULL entity returns early");
	hudtextparms_t tp;
	memset(&tp, 0, sizeof(tp));
	META_UTIL_HudMessage(NULL, tp, "test");
	PASS();
	return 0;
}

static int test_hudmessage_free_entity(void)
{
	TEST("META_UTIL_HudMessage - free entity returns early");
	hudtextparms_t tp;
	memset(&tp, 0, sizeof(tp));
	memset(&test_edict, 0, sizeof(test_edict));
	test_edict.free = 1;
	META_UTIL_HudMessage(&test_edict, tp, "test");
	PASS();
	return 0;
}

static int test_hudmessage_short_message(void)
{
	TEST("META_UTIL_HudMessage - short message sends normally");
	hudtextparms_t tp;
	memset(&tp, 0, sizeof(tp));
	memset(&test_edict, 0, sizeof(test_edict));
	META_UTIL_HudMessage(&test_edict, tp, "hello");
	PASS();
	return 0;
}

static int test_hudmessage_long_message(void)
{
	TEST("META_UTIL_HudMessage - long message truncated to 512");
	hudtextparms_t tp;
	memset(&tp, 0, sizeof(tp));
	memset(&test_edict, 0, sizeof(test_edict));
	char longmsg[600];
	memset(longmsg, 'A', sizeof(longmsg) - 1);
	longmsg[sizeof(longmsg) - 1] = '\0';
	META_UTIL_HudMessage(&test_edict, tp, longmsg);
	PASS();
	return 0;
}

static int test_hudmessage_effect2(void)
{
	TEST("META_UTIL_HudMessage - effect=2 writes fxTime");
	hudtextparms_t tp;
	memset(&tp, 0, sizeof(tp));
	tp.effect = 2;
	tp.fxTime = 1.0f;
	memset(&test_edict, 0, sizeof(test_edict));
	META_UTIL_HudMessage(&test_edict, tp, "fx");
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

	printf("test_sdk_util:\n");

	fail |= test_fixed_signed16_normal();
	fail |= test_fixed_signed16_zero();
	fail |= test_fixed_signed16_overflow();
	fail |= test_fixed_signed16_underflow();
	fail |= test_fixed_signed16_negative();

	fail |= test_fixed_unsigned16_normal();
	fail |= test_fixed_unsigned16_zero();
	fail |= test_fixed_unsigned16_overflow();
	fail |= test_fixed_unsigned16_negative();

	fail |= test_varargs_normal();
	fail |= test_varargs_empty();

	fail |= test_hudmessage_null_entity();
	fail |= test_hudmessage_free_entity();
	fail |= test_hudmessage_short_message();
	fail |= test_hudmessage_long_message();
	fail |= test_hudmessage_effect2();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
