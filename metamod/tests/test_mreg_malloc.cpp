//
// metamod-p - tests for mreg.cpp malloc failure paths
//
// Uses preprocessor macro redirection to mock malloc/realloc/calloc/strdup,
// allowing us to force allocation failures and test error handling code.
//

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <extdll.h>

#include "mreg.h"
#include "metamod.h"
#include "mlist.h"
#include "mplugin.h"
#include "types_meta.h"
#include "log_meta.h"
#include "osdep.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Mock allocator infrastructure
// ============================================================

static int mock_alloc_fail_countdown = -1;

static bool mock_should_fail(void)
{
	if (mock_alloc_fail_countdown < 0)
		return false;
	if (mock_alloc_fail_countdown == 0)
		return true;
	mock_alloc_fail_countdown--;
	return false;
}

static void mock_alloc_reset(void)
{
	mock_alloc_fail_countdown = -1;
}

static void mock_alloc_fail_after(int n_successes)
{
	mock_alloc_fail_countdown = n_successes;
}

static void *realloc_mocked(void *ptr, size_t size)
{
	if (mock_should_fail()) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(ptr, size);
}

static void *calloc_mocked(size_t nmemb, size_t size)
{
	if (mock_should_fail()) {
		errno = ENOMEM;
		return NULL;
	}
	return calloc(nmemb, size);
}

static char *strdup_mocked(const char *s)
{
	if (mock_should_fail()) {
		errno = ENOMEM;
		return NULL;
	}
	return strdup(s);
}

#define realloc realloc_mocked
#define calloc calloc_mocked
#define strdup strdup_mocked

#include "../mreg.cpp"

#undef realloc
#undef calloc
#undef strdup

// ============================================================
// MRegCmdList::add - realloc failure when growing
// ============================================================

static int test_regcmdlist_add_realloc_fail(void)
{
	TEST("MRegCmdList::add - realloc failure returns NULL");
	mock_reset();
	mock_alloc_reset();

	// Create a list and fill it to capacity
	MRegCmdList list;
	char name[32];
	for (int i = 0; i < REG_CMD_GROWSIZE; i++) {
		safevoid_snprintf(name, sizeof(name), "cmd_%d", i);
		ASSERT_PTR_NOT_NULL(list.add(name));
	}
	// List is now full (endlist == size). Next add triggers realloc.
	// Make realloc fail.
	mock_alloc_fail_after(0);
	MRegCmd *result = list.add("overflow_cmd");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCmdList::add - strdup failure after successful growth
// ============================================================

static int test_regcmdlist_add_strdup_fail(void)
{
	TEST("MRegCmdList::add - strdup failure returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCmdList list;
	// Fill to capacity
	char name[32];
	for (int i = 0; i < REG_CMD_GROWSIZE; i++) {
		safevoid_snprintf(name, sizeof(name), "cmd_%d", i);
		ASSERT_PTR_NOT_NULL(list.add(name));
	}
	// Next add: realloc succeeds (for growth), but strdup fails.
	// Growth calls realloc(1), then strdup(1). Fail on strdup (2nd alloc).
	mock_alloc_fail_after(1);
	MRegCmd *result = list.add("strdup_fail_cmd");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCmdList::add - strdup failure (list not full, no growth needed)
// ============================================================

static int test_regcmdlist_add_strdup_fail_no_grow(void)
{
	TEST("MRegCmdList::add - strdup failure without growth returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCmdList list;
	// List has space, so add goes straight to strdup. Fail on first alloc.
	mock_alloc_fail_after(0);
	MRegCmd *result = list.add("fail_cmd");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList::add - realloc failure when growing
// ============================================================

static int test_regcvarlist_add_realloc_fail(void)
{
	TEST("MRegCvarList::add - realloc failure returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCvarList list;
	char name[32];
	for (int i = 0; i < REG_CVAR_GROWSIZE; i++) {
		safevoid_snprintf(name, sizeof(name), "cvar_%d", i);
		MRegCvar *cv = list.add(name);
		ASSERT_PTR_NOT_NULL(cv);
	}
	// List is now full. Next add triggers realloc.
	mock_alloc_fail_after(0);
	MRegCvar *result = list.add("overflow_cvar");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList::add - calloc failure for cvar_t
// ============================================================

static int test_regcvarlist_add_calloc_fail(void)
{
	TEST("MRegCvarList::add - calloc failure for cvar_t returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCvarList list;
	// List has space. add() calls calloc for cvar_t data, then strdup for name.
	// Fail on first alloc (the calloc for cvar_t).
	mock_alloc_fail_after(0);
	MRegCvar *result = list.add("calloc_fail_cvar");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList::add - strdup failure for cvar name
// ============================================================

static int test_regcvarlist_add_strdup_fail(void)
{
	TEST("MRegCvarList::add - strdup failure for name returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCvarList list;
	// Fail on second alloc: calloc succeeds, strdup fails.
	mock_alloc_fail_after(1);
	MRegCvar *result = list.add("strdup_fail_cvar");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);
	ASSERT_PTR_NULL(list.test_vlist()[list.test_endlist()].data);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList::add - realloc succeeds, calloc fails
// ============================================================

static int test_regcvarlist_add_grow_calloc_fail(void)
{
	TEST("MRegCvarList::add - calloc failure after growth returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCvarList list;
	char name[32];
	for (int i = 0; i < REG_CVAR_GROWSIZE; i++) {
		safevoid_snprintf(name, sizeof(name), "cv_%d", i);
		ASSERT_PTR_NOT_NULL(list.add(name));
	}
	// Growth path: realloc(1 alloc) succeeds, then calloc for cvar_t fails.
	mock_alloc_fail_after(1);
	MRegCvar *result = list.add("grow_calloc_fail");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);

	mock_alloc_reset();
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList::add - realloc+calloc succeed, strdup fails
// ============================================================

static int test_regcvarlist_add_grow_strdup_fail(void)
{
	TEST("MRegCvarList::add - strdup failure after growth returns NULL");
	mock_reset();
	mock_alloc_reset();

	MRegCvarList list;
	char name[32];
	for (int i = 0; i < REG_CVAR_GROWSIZE; i++) {
		safevoid_snprintf(name, sizeof(name), "cv_%d", i);
		ASSERT_PTR_NOT_NULL(list.add(name));
	}
	// Growth path: realloc(1) succeeds, calloc(2) succeeds, strdup(3) fails.
	mock_alloc_fail_after(2);
	MRegCvar *result = list.add("grow_strdup_fail");
	ASSERT_PTR_NULL(result);
	ASSERT_TRUE(meta_errno == ME_NOMEM);
	ASSERT_PTR_NULL(list.test_vlist()[list.test_endlist()].data);

	mock_alloc_reset();
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

	printf("test_mreg_malloc:\n");

	// MRegCmdList malloc failure tests
	fail |= test_regcmdlist_add_realloc_fail();
	fail |= test_regcmdlist_add_strdup_fail();
	fail |= test_regcmdlist_add_strdup_fail_no_grow();

	// MRegCvarList malloc failure tests
	fail |= test_regcvarlist_add_realloc_fail();
	fail |= test_regcvarlist_add_calloc_fail();
	fail |= test_regcvarlist_add_strdup_fail();
	fail |= test_regcvarlist_add_grow_calloc_fail();
	fail |= test_regcvarlist_add_grow_strdup_fail();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
