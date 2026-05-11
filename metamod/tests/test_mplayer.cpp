//
// metamod-p - tests for mplayer.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "mplayer.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// MPlayer tests
// ============================================================

static int test_mplayer_constructor(void)
{
	TEST("MPlayer - constructor initializes to not querying");
	MPlayer p;
	ASSERT_PTR_NULL(p.is_querying_cvar());
	PASS();
	return 0;
}

static int test_mplayer_set_query(void)
{
	TEST("MPlayer - set_cvar_query stores cvar name");
	MPlayer p;
	p.set_cvar_query("cl_updaterate");
	ASSERT_PTR_NOT_NULL(p.is_querying_cvar());
	ASSERT_STR(p.is_querying_cvar(), "cl_updaterate");
	PASS();
	return 0;
}

static int test_mplayer_set_query_null(void)
{
	TEST("MPlayer - set_cvar_query(NULL) sets ME_ARGUMENT");
	mock_reset();
	MPlayer p;
	p.set_cvar_query(NULL);
	ASSERT_PTR_NULL(p.is_querying_cvar());
	ASSERT_INT(meta_errno, ME_ARGUMENT);
	PASS();
	return 0;
}

static int test_mplayer_clear_query(void)
{
	TEST("MPlayer - clear_cvar_query clears state");
	MPlayer p;
	p.set_cvar_query("cl_cmdrate");
	ASSERT_PTR_NOT_NULL(p.is_querying_cvar());
	p.clear_cvar_query();
	ASSERT_PTR_NULL(p.is_querying_cvar());
	PASS();
	return 0;
}

static int test_mplayer_set_query_replace(void)
{
	TEST("MPlayer - set_cvar_query replaces previous");
	MPlayer p;
	p.set_cvar_query("first");
	p.set_cvar_query("second");
	ASSERT_STR(p.is_querying_cvar(), "second");
	PASS();
	return 0;
}

static int test_mplayer_destructor_with_cvar(void)
{
	TEST("MPlayer - destructor frees cvarName");
	{
		MPlayer p;
		p.set_cvar_query("test_cvar");
	}
	PASS();
	return 0;
}

static int test_mplayer_clear_without_set(void)
{
	TEST("MPlayer - clear_cvar_query when not set is safe");
	MPlayer p;
	p.clear_cvar_query();
	ASSERT_PTR_NULL(p.is_querying_cvar());
	PASS();
	return 0;
}

static int test_mplayer_copy_constructor(void)
{
	TEST("MPlayer - copy constructor duplicates state");
	MPlayer a;
	a.set_cvar_query("rate");
	MPlayer b(a);
	ASSERT_PTR_NOT_NULL(b.is_querying_cvar());
	ASSERT_STR(b.is_querying_cvar(), "rate");
	a.clear_cvar_query();
	ASSERT_PTR_NOT_NULL(b.is_querying_cvar());
	ASSERT_STR(b.is_querying_cvar(), "rate");
	PASS();
	return 0;
}

static int test_mplayer_copy_constructor_empty(void)
{
	TEST("MPlayer - copy constructor from empty");
	MPlayer a;
	MPlayer b(a);
	ASSERT_PTR_NULL(b.is_querying_cvar());
	PASS();
	return 0;
}

static int test_mplayer_assignment(void)
{
	TEST("MPlayer - assignment operator copies state");
	MPlayer a, b;
	a.set_cvar_query("cl_cmdrate");
	b = a;
	ASSERT_PTR_NOT_NULL(b.is_querying_cvar());
	ASSERT_STR(b.is_querying_cvar(), "cl_cmdrate");
	a.clear_cvar_query();
	ASSERT_PTR_NOT_NULL(b.is_querying_cvar());
	PASS();
	return 0;
}

static int test_mplayer_assignment_overwrites(void)
{
	TEST("MPlayer - assignment overwrites existing cvar");
	MPlayer a, b;
	a.set_cvar_query("first");
	b.set_cvar_query("old_value");
	b = a;
	ASSERT_STR(b.is_querying_cvar(), "first");
	PASS();
	return 0;
}

static int test_mplayer_assignment_self(void)
{
	TEST("MPlayer - self-assignment is safe");
	MPlayer a;
	a.set_cvar_query("test");
	a = a;
	ASSERT_STR(a.is_querying_cvar(), "test");
	PASS();
	return 0;
}

// ============================================================
// MPlayerList tests
// ============================================================

static edict_t test_edict;

static int test_playerlist_set_query(void)
{
	TEST("MPlayerList - set_player_cvar_query stores cvar");
	mock_reset();
	mock_set_index_of_edict(1);
	memset(&test_edict, 0, sizeof(test_edict));
	g_Players.set_player_cvar_query(&test_edict, "rate");
	const char *q = g_Players.is_querying_cvar(&test_edict);
	ASSERT_PTR_NOT_NULL(q);
	ASSERT_STR(q, "rate");
	PASS();
	return 0;
}

static int test_playerlist_clear_query(void)
{
	TEST("MPlayerList - clear_player_cvar_query clears");
	mock_reset();
	mock_set_index_of_edict(1);
	memset(&test_edict, 0, sizeof(test_edict));
	g_Players.set_player_cvar_query(&test_edict, "rate");
	g_Players.clear_player_cvar_query(&test_edict);
	ASSERT_PTR_NULL(g_Players.is_querying_cvar(&test_edict));
	PASS();
	return 0;
}

static int test_playerlist_clear_all(void)
{
	TEST("MPlayerList - clear_all_cvar_queries clears all");
	mock_reset();
	mock_set_index_of_edict(1);
	memset(&test_edict, 0, sizeof(test_edict));
	g_Players.set_player_cvar_query(&test_edict, "rate");
	g_Players.clear_all_cvar_queries();
	ASSERT_PTR_NULL(g_Players.is_querying_cvar(&test_edict));
	PASS();
	return 0;
}

static int test_playerlist_invalid_index_zero(void)
{
	TEST("MPlayerList - index 0 returns early from set/clear");
	mock_reset();
	mock_set_index_of_edict(0);
	memset(&test_edict, 0, sizeof(test_edict));
	g_Players.set_player_cvar_query(&test_edict, "rate");
	g_Players.clear_player_cvar_query(&test_edict);
	PASS();
	return 0;
}

static int test_playerlist_is_querying_invalid(void)
{
	TEST("MPlayerList - is_querying index 0 returns ME_NOTFOUND");
	mock_reset();
	mock_set_index_of_edict(0);
	memset(&test_edict, 0, sizeof(test_edict));
	const char *q = g_Players.is_querying_cvar(&test_edict);
	ASSERT_PTR_NULL(q);
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

static int test_playerlist_invalid_index_over_max(void)
{
	TEST("MPlayerList - index > maxClients returns ME_NOTFOUND");
	mock_reset();
	mock_set_index_of_edict(33);
	memset(&test_edict, 0, sizeof(test_edict));
	const char *q = g_Players.is_querying_cvar(&test_edict);
	ASSERT_PTR_NULL(q);
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

static int test_playerlist_index_at_num_slots(void)
{
	TEST("MPlayerList - index >= NUM_SLOTS rejected by set");
	mock_reset();
	mock_set_index_of_edict(MAX_PLAYERS + 1);
	memset(&test_edict, 0, sizeof(test_edict));
	g_Players.set_player_cvar_query(&test_edict, "rate");
	PASS();
	return 0;
}

static int test_playerlist_not_querying(void)
{
	TEST("MPlayerList - is_querying returns NULL when not set");
	mock_reset();
	mock_set_index_of_edict(1);
	memset(&test_edict, 0, sizeof(test_edict));
	ASSERT_PTR_NULL(g_Players.is_querying_cvar(&test_edict));
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

	printf("test_mplayer:\n");

	fail |= test_mplayer_constructor();
	fail |= test_mplayer_set_query();
	fail |= test_mplayer_set_query_null();
	fail |= test_mplayer_clear_query();
	fail |= test_mplayer_set_query_replace();
	fail |= test_mplayer_destructor_with_cvar();
	fail |= test_mplayer_clear_without_set();

	fail |= test_playerlist_set_query();
	fail |= test_playerlist_clear_query();
	fail |= test_playerlist_clear_all();
	fail |= test_playerlist_invalid_index_zero();
	fail |= test_playerlist_is_querying_invalid();
	fail |= test_playerlist_invalid_index_over_max();
	fail |= test_playerlist_index_at_num_slots();
	fail |= test_playerlist_not_querying();

	fail |= test_mplayer_copy_constructor();
	fail |= test_mplayer_copy_constructor_empty();
	fail |= test_mplayer_assignment();
	fail |= test_mplayer_assignment_overwrites();
	fail |= test_mplayer_assignment_self();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
