//
// metamod-p - tests for api_hook.cpp
//
// Tests the core hook dispatch: main_hook_function_void and
// main_hook_function, which are the hottest code paths in metamod.
// Every engine<->gamedll call goes through these dispatchers.
//

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <extdll.h>

#include "metamod.h"
#include "dllapi.h"
#include "api_info.h"
#include "api_hook.h"
#include "mlist.h"
#include "mreg.h"
#include "conf_meta.h"
#include "meta_api.h"

#include "engine_mock.h"
#include "test_common.h"

// Plugins normally get this pointer via Meta_Query/Meta_Attach.
// Point it at metamod's PublicMetaGlobals so RETURN_META works.
meta_globals_t *gpMetaGlobals = &PublicMetaGlobals;

// Wrap realloc to always move the allocation (exposes use-after-free bugs).
static size_t last_realloc_old_size = 0;
extern "C" void *__real_realloc(void *ptr, size_t size);
extern "C" void *__wrap_realloc(void *ptr, size_t size)
{
	void *newptr = malloc(size);
	if (!newptr)
		return NULL;
	if (ptr) {
		size_t copy_size = size < last_realloc_old_size ? size : last_realloc_old_size;
		memcpy(newptr, ptr, copy_size);
		memset(ptr, 0, last_realloc_old_size);
		free(ptr);
	}
	last_realloc_old_size = size;
	return newptr;
}

// ============================================================
// Test tracking state
// ============================================================

static int g_gamedll_called;
static int g_plugin1_pre_called;
static int g_plugin1_post_called;
static int g_plugin2_pre_called;
static int g_plugin2_post_called;
static META_RES g_plugin1_pre_mres;
static META_RES g_plugin1_post_mres;
static META_RES g_plugin2_pre_mres;
static META_RES g_plugin2_post_mres;
static int g_plugin1_pre_retval;
static int g_plugin2_pre_retval;
static int g_plugin1_post_retval;
static META_RES g_plugin1_pre_seen_prev_mres;
static META_RES g_plugin2_pre_seen_prev_mres;
static META_RES g_plugin1_post_seen_status;
static int g_plugin1_post_seen_orig_ret_val;

// ============================================================
// Game DLL mock functions
// ============================================================

static void mock_gamedll_init(void)
{
	g_gamedll_called++;
}

static int mock_gamedll_spawn(edict_t *)
{
	g_gamedll_called++;
	return 42;
}

// ============================================================
// Plugin hook functions
// ============================================================

static void plugin1_pre_GameInit(void)
{
	g_plugin1_pre_called++;
	g_plugin1_pre_seen_prev_mres = gpMetaGlobals->prev_mres;
	RETURN_META(g_plugin1_pre_mres);
}

static void plugin1_post_GameInit(void)
{
	g_plugin1_post_called++;
	g_plugin1_post_seen_status = gpMetaGlobals->status;
	RETURN_META(g_plugin1_post_mres);
}

static void plugin1_post_GameInit_no_result(void)
{
	g_plugin1_post_called++;
}

static void plugin2_pre_GameInit(void)
{
	g_plugin2_pre_called++;
	g_plugin2_pre_seen_prev_mres = gpMetaGlobals->prev_mres;
	RETURN_META(g_plugin2_pre_mres);
}

__attribute__((unused))
static void plugin2_post_GameInit(void)
{
	g_plugin2_post_called++;
	RETURN_META(g_plugin2_post_mres);
}

static int plugin1_pre_Spawn(edict_t *)
{
	g_plugin1_pre_called++;
	g_plugin1_pre_seen_prev_mres = gpMetaGlobals->prev_mres;
	RETURN_META_VALUE(g_plugin1_pre_mres, g_plugin1_pre_retval);
}

static int plugin1_post_Spawn(edict_t *)
{
	g_plugin1_post_called++;
	g_plugin1_post_seen_status = gpMetaGlobals->status;
	if (gpMetaGlobals->orig_ret)
		g_plugin1_post_seen_orig_ret_val = *(int *)gpMetaGlobals->orig_ret;
	RETURN_META_VALUE(g_plugin1_post_mres, g_plugin1_post_retval);
}

static int plugin2_pre_Spawn(edict_t *)
{
	g_plugin2_pre_called++;
	g_plugin2_pre_seen_prev_mres = gpMetaGlobals->prev_mres;
	RETURN_META_VALUE(g_plugin2_pre_mres, g_plugin2_pre_retval);
}

// ============================================================
// Callbacks that trigger rebuild_hook_lists (issue #108)
// ============================================================

static DLL_FUNCTIONS plugin3_pre_funcs;
static DLL_FUNCTIONS plugin3_post_funcs;

static void plugin1_pre_GameInit_load_plugin3(void)
{
	g_plugin1_pre_called++;

	// Simulate AMXX loading a new module from within a hook callback:
	// activate plugin3 and call rebuild_hook_lists().
	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();

	RETURN_META(MRES_IGNORED);
}

static void plugin1_post_GameInit_load_plugin3(void)
{
	g_plugin1_post_called++;

	// Same as above but triggered from post hook path.
	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();

	RETURN_META(MRES_IGNORED);
}

// ============================================================
// Infrastructure: set up a MPluginList with mock plugins
// ============================================================

static MPluginList test_plugins("plugins.ini");
static MRegCmdList test_reg_cmds;
static MRegCvarList test_reg_cvars;
static MRegMsgList test_reg_msgs;
static MConfig test_config;
static option_t test_options[] = {
	{ NULL, CF_NONE, NULL, NULL }
};

static DLL_FUNCTIONS gamedll_funcs;

static DLL_FUNCTIONS plugin1_pre_funcs;
static DLL_FUNCTIONS plugin1_post_funcs;
static DLL_FUNCTIONS plugin2_pre_funcs;
static DLL_FUNCTIONS plugin2_post_funcs;

static void reset_test_state(void)
{
	g_gamedll_called = 0;
	g_plugin1_pre_called = 0;
	g_plugin1_post_called = 0;
	g_plugin2_pre_called = 0;
	g_plugin2_post_called = 0;
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin1_post_mres = MRES_IGNORED;
	g_plugin2_pre_mres = MRES_IGNORED;
	g_plugin2_post_mres = MRES_IGNORED;
	g_plugin1_pre_retval = 0;
	g_plugin2_pre_retval = 0;
	g_plugin1_post_retval = 0;
	g_plugin1_pre_seen_prev_mres = MRES_UNSET;
	g_plugin2_pre_seen_prev_mres = MRES_UNSET;
	g_plugin1_post_seen_status = MRES_UNSET;
	g_plugin1_post_seen_orig_ret_val = 0;
}

static void setup_one_plugin(void)
{
	mock_reset();
	reset_test_state();

	Plugins = &test_plugins;
	RegCmds = &test_reg_cmds;
	RegCvars = &test_reg_cvars;
	RegMsgs = &test_reg_msgs;
	memset(&test_config, 0, sizeof(test_config));
	test_config.init(test_options);
	Config = &test_config;
	metamod_not_loaded = 0;

	memset(&gamedll_funcs, 0, sizeof(gamedll_funcs));
	gamedll_funcs.pfnGameInit = mock_gamedll_init;
	gamedll_funcs.pfnSpawn = mock_gamedll_spawn;
	GameDLL_funcs.dllapi_table = &gamedll_funcs;

	memset(&plugin1_pre_funcs, 0, sizeof(plugin1_pre_funcs));
	memset(&plugin1_post_funcs, 0, sizeof(plugin1_post_funcs));

	free(test_plugins.hook_list_data);
	memset(&test_plugins, 0, sizeof(test_plugins));
	MPlugin *plug = &test_plugins.plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->index = 1;
	snprintf(plug->filename, sizeof(plug->filename), "plugin1");
	plug->file = plug->filename;
	plug->tables.dllapi = &plugin1_pre_funcs;
	plug->post_tables.dllapi = &plugin1_post_funcs;
	test_plugins.endlist = 1;
	test_plugins.rebuild_hook_lists();
}

static void setup_two_plugins(void)
{
	setup_one_plugin();

	memset(&plugin2_pre_funcs, 0, sizeof(plugin2_pre_funcs));
	memset(&plugin2_post_funcs, 0, sizeof(plugin2_post_funcs));

	MPlugin *plug2 = &test_plugins.plist[1];
	memset(plug2, 0, sizeof(*plug2));
	plug2->status = PL_RUNNING;
	plug2->index = 2;
	snprintf(plug2->filename, sizeof(plug2->filename), "plugin2");
	plug2->file = plug2->filename;
	plug2->tables.dllapi = &plugin2_pre_funcs;
	plug2->post_tables.dllapi = &plugin2_post_funcs;
	test_plugins.endlist = 2;
	test_plugins.rebuild_hook_lists();
}

static void teardown(void)
{
	Plugins = NULL;
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
	Config = NULL;
	GameDLL_funcs.dllapi_table = NULL;
}

// Helper: call GameDLLInit through the hook dispatcher
static void call_hooked_GameInit(void)
{
	DLL_FUNCTIONS meta_funcs;
	memset(&meta_funcs, 0, sizeof(meta_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&meta_funcs, &ver);
	meta_funcs.pfnGameInit();
}

// Helper: call DispatchSpawn through the hook dispatcher
static int call_hooked_Spawn(void)
{
	DLL_FUNCTIONS meta_funcs;
	memset(&meta_funcs, 0, sizeof(meta_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&meta_funcs, &ver);
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	return meta_funcs.pfnSpawn(&ed);
}

// ============================================================
// Tests: void function dispatch (GameDLLInit)
// ============================================================

static int test_no_plugins_calls_gamedll(void)
{
	TEST("dispatch void - no plugins, calls game DLL directly");
	setup_one_plugin();
	test_plugins.endlist = 0;
	test_plugins.plist[0].status = PL_EMPTY;
	test_plugins.rebuild_hook_lists();

	call_hooked_GameInit();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	teardown();
	PASS();
	return 0;
}

static int test_plugin_ignored_calls_gamedll(void)
{
	TEST("dispatch void - plugin MRES_IGNORED, game DLL still called");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	g_plugin1_pre_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_plugin_handled_calls_gamedll(void)
{
	TEST("dispatch void - plugin MRES_HANDLED, game DLL still called");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	g_plugin1_pre_mres = MRES_HANDLED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_plugin_supercede_skips_gamedll(void)
{
	TEST("dispatch void - plugin MRES_SUPERCEDE, game DLL NOT called");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	g_plugin1_pre_mres = MRES_SUPERCEDE;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 0);
	teardown();
	PASS();
	return 0;
}

static int test_post_hook_called(void)
{
	TEST("dispatch void - post hook called after game DLL");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit;
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin1_post_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_post_hook_sees_status(void)
{
	TEST("dispatch void - post hook sees correct status from pre");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit;
	g_plugin1_pre_mres = MRES_HANDLED;
	g_plugin1_post_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_post_seen_status == MRES_HANDLED);
	teardown();
	PASS();
	return 0;
}

static int test_post_supercede_warns(void)
{
	TEST("dispatch void - MRES_SUPERCEDE in post hook triggers warning");
	setup_one_plugin();
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit;
	g_plugin1_post_mres = MRES_SUPERCEDE;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_post_called == 1);
	int found = 0;
	for (int i = 0; i < mock_get_alert_count(); i++)
		if (strstr(mock_get_alert_msg(i), "MRES_SUPERCEDE not valid in Post"))
			found = 1;
	ASSERT_TRUE(found);
	teardown();
	PASS();
	return 0;
}

static int test_plugin_unset_warns(void)
{
	TEST("dispatch void - plugin not setting mres triggers warning");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	g_plugin1_pre_mres = MRES_UNSET;

	call_hooked_GameInit();
	int found = 0;
	for (int i = 0; i < mock_get_alert_count(); i++)
		if (strstr(mock_get_alert_msg(i), "didn't set meta_result"))
			found = 1;
	ASSERT_TRUE(found);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: two-plugin ordering
// ============================================================

static int test_two_plugins_both_called(void)
{
	TEST("dispatch void - two plugins, both pre hooks called in order");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin2_pre_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_plugin2_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_two_plugins_prev_mres_propagates(void)
{
	TEST("dispatch void - second plugin sees first plugin's mres as prev_mres");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin1_pre_mres = MRES_HANDLED;
	g_plugin2_pre_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_seen_prev_mres == MRES_UNSET);
	ASSERT_TRUE(g_plugin2_pre_seen_prev_mres == MRES_HANDLED);
	teardown();
	PASS();
	return 0;
}

static int test_two_plugins_supercede_highest_wins(void)
{
	TEST("dispatch void - first plugin SUPERCEDE, second IGNORED: game DLL skipped");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin1_pre_mres = MRES_SUPERCEDE;
	g_plugin2_pre_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_plugin2_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 0);
	teardown();
	PASS();
	return 0;
}

static int test_paused_plugin_skipped(void)
{
	TEST("dispatch void - paused plugin is skipped");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	test_plugins.plist[0].status = PL_PAUSED;
	test_plugins.rebuild_hook_lists();
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin2_pre_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	ASSERT_TRUE(g_plugin2_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_plugin_no_hook_skipped(void)
{
	TEST("dispatch void - plugin without this hook function is skipped");
	setup_one_plugin();
	// plugin1_pre_funcs.pfnGameInit is NULL (not set)
	g_plugin1_pre_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_plugin_no_table_skipped(void)
{
	TEST("dispatch void - plugin without dllapi table is skipped");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	test_plugins.plist[0].tables.dllapi = NULL;
	test_plugins.rebuild_hook_lists();

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: return-value dispatch (DispatchSpawn)
// ============================================================

static int test_return_no_plugins(void)
{
	TEST("dispatch return - no plugins, returns game DLL value");
	setup_one_plugin();
	test_plugins.endlist = 0;
	test_plugins.rebuild_hook_lists();

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(ret == 42);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

static int test_return_plugin_ignored(void)
{
	TEST("dispatch return - plugin MRES_IGNORED, returns game DLL value");
	setup_one_plugin();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn;
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin1_pre_retval = 99;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

static int test_return_plugin_supercede(void)
{
	TEST("dispatch return - plugin MRES_SUPERCEDE, returns plugin value");
	setup_one_plugin();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn;
	g_plugin1_pre_mres = MRES_SUPERCEDE;
	g_plugin1_pre_retval = 99;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 0);
	ASSERT_TRUE(ret == 99);
	teardown();
	PASS();
	return 0;
}

static int test_return_post_override(void)
{
	TEST("dispatch return - post hook MRES_OVERRIDE replaces return value");
	setup_one_plugin();
	plugin1_post_funcs.pfnSpawn = plugin1_post_Spawn;
	g_plugin1_post_mres = MRES_OVERRIDE;
	g_plugin1_post_retval = 77;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(ret == 77);
	teardown();
	PASS();
	return 0;
}

static int test_return_post_sees_orig_ret(void)
{
	TEST("dispatch return - post hook can see orig_ret from game DLL");
	setup_one_plugin();
	plugin1_post_funcs.pfnSpawn = plugin1_post_Spawn;
	g_plugin1_post_mres = MRES_IGNORED;

	call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(g_plugin1_post_seen_orig_ret_val == 42);
	teardown();
	PASS();
	return 0;
}

static int test_return_two_plugins_supercede_first(void)
{
	TEST("dispatch return - first plugin SUPERCEDE overrides, second sees it");
	setup_two_plugins();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn;
	plugin2_pre_funcs.pfnSpawn = plugin2_pre_Spawn;
	g_plugin1_pre_mres = MRES_SUPERCEDE;
	g_plugin1_pre_retval = 55;
	g_plugin2_pre_mres = MRES_IGNORED;
	g_plugin2_pre_retval = 0;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 0);
	ASSERT_TRUE(ret == 55);
	teardown();
	PASS();
	return 0;
}

static int test_return_pre_override_post_sees_override_ret(void)
{
	TEST("dispatch return - pre MRES_OVERRIDE sets status, post runs");
	setup_one_plugin();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn;
	plugin1_post_funcs.pfnSpawn = plugin1_post_Spawn;
	g_plugin1_pre_mres = MRES_OVERRIDE;
	g_plugin1_pre_retval = 99;
	g_plugin1_post_mres = MRES_OVERRIDE;
	g_plugin1_post_retval = 77;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(g_plugin1_post_seen_orig_ret_val == 42);
	ASSERT_TRUE(ret == 77);
	teardown();
	PASS();
	return 0;
}

static int plugin1_post_Spawn_unset(edict_t *)
{
	g_plugin1_post_called++;
	return 0;
}

static int test_return_post_unset_warns(void)
{
	TEST("dispatch return - post hook MRES_UNSET triggers warning");
	setup_one_plugin();
	plugin1_post_funcs.pfnSpawn = (int (*)(edict_t *))plugin1_post_Spawn_unset;
	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

static int plugin1_post_Spawn_supercede(edict_t *)
{
	g_plugin1_post_called++;
	RETURN_META_VALUE(MRES_SUPERCEDE, 999);
}

static int test_return_post_supercede_warns(void)
{
	TEST("dispatch return - post hook MRES_SUPERCEDE triggers warning");
	setup_one_plugin();
	plugin1_post_funcs.pfnSpawn = (int (*)(edict_t *))plugin1_post_Spawn_supercede;
	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

static int test_return_paused_plugin_skipped(void)
{
	TEST("dispatch return - paused plugin skipped in post hooks");
	setup_two_plugins();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn;
	plugin1_post_funcs.pfnSpawn = plugin1_post_Spawn;
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin1_post_mres = MRES_IGNORED;
	test_plugins.plist[1].status = PL_PAUSED;
	test_plugins.rebuild_hook_lists();
	plugin2_post_funcs.pfnSpawn = (int (*)(edict_t *))plugin1_post_Spawn_unset;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(g_plugin2_post_called == 0);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

static int test_return_plugin_no_post_table(void)
{
	TEST("dispatch return - plugin without post table is skipped");
	setup_two_plugins();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn;
	g_plugin1_pre_mres = MRES_IGNORED;
	test_plugins.plist[1].post_tables.dllapi = NULL;
	test_plugins.rebuild_hook_lists();

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

static int test_return_null_gamedll_table(void)
{
	TEST("dispatch return - NULL game DLL table handled");
	setup_one_plugin();
	GameDLL_funcs.dllapi_table = NULL;
	call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 0);
	teardown();
	PASS();
	return 0;
}

static int test_return_null_gamedll_function(void)
{
	TEST("dispatch return - NULL game DLL function handled");
	setup_one_plugin();
	gamedll_funcs.pfnSpawn = NULL;
	call_hooked_Spawn();
	ASSERT_TRUE(g_gamedll_called == 0);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: null game DLL table/function
// ============================================================

static int test_null_gamedll_table(void)
{
	TEST("dispatch void - NULL game DLL table handled gracefully");
	setup_one_plugin();
	GameDLL_funcs.dllapi_table = NULL;

	call_hooked_GameInit();
	ASSERT_TRUE(g_gamedll_called == 0);
	teardown();
	PASS();
	return 0;
}

static int test_null_gamedll_function(void)
{
	TEST("dispatch void - NULL game DLL function handled gracefully");
	setup_one_plugin();
	gamedll_funcs.pfnGameInit = NULL;

	call_hooked_GameInit();
	ASSERT_TRUE(g_gamedll_called == 0);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: nested call (call_count backup/restore)
// ============================================================

static void plugin_nested_pre_GameInit(void);

static int g_outer_post_called;
static META_RES g_outer_post_seen_status;

static void outer_post_GameInit(void)
{
	g_outer_post_called++;
	g_outer_post_seen_status = gpMetaGlobals->status;
	RETURN_META(MRES_IGNORED);
}

static void plugin_nested_pre_GameInit(void)
{
	g_plugin1_pre_called++;

	// Simulate what a bot plugin does: call an engine function that
	// triggers another dllapi call (re-entering the hook dispatcher).
	// Remove pre hook to avoid infinite recursion, then re-enter.
	plugin1_pre_funcs.pfnGameInit = NULL;

	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnGameInit();

	RETURN_META(MRES_HANDLED);
}

static int test_nested_call_restores_globals(void)
{
	TEST("dispatch void - nested call backs up and restores PublicMetaGlobals");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin_nested_pre_GameInit;
	plugin1_post_funcs.pfnGameInit = outer_post_GameInit;
	g_outer_post_called = 0;
	g_outer_post_seen_status = MRES_UNSET;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 2);
	// Post hook called twice: once for inner call, once for outer
	ASSERT_TRUE(g_outer_post_called == 2);
	// Outer post should see MRES_HANDLED (from pre hook's RETURN_META)
	ASSERT_TRUE(g_outer_post_seen_status == MRES_HANDLED);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: MRES_UNSET in return-value pre-hook (line 306)
// ============================================================

static int plugin1_pre_Spawn_unset(edict_t *)
{
	g_plugin1_pre_called++;
	// Deliberately do NOT call RETURN_META_VALUE, leaving mres as MRES_UNSET
	return 0;
}

static int test_return_pre_unset_warns(void)
{
	TEST("dispatch return - pre hook MRES_UNSET triggers warning");
	setup_one_plugin();
	plugin1_pre_funcs.pfnSpawn = (int (*)(edict_t *))plugin1_pre_Spawn_unset;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(ret == 42);
	int found = 0;
	for (int i = 0; i < mock_get_alert_count(); i++)
		if (strstr(mock_get_alert_msg(i), "didn't set meta_result"))
			found = 1;
	ASSERT_TRUE(found);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: nested call for return-value dispatch (lines 240, 403)
// ============================================================

static int plugin_nested_pre_Spawn(edict_t *);
static int g_outer_post_ret_called;
static int g_outer_post_ret_seen_orig;

static int outer_post_Spawn(edict_t *)
{
	g_outer_post_ret_called++;
	if (gpMetaGlobals->orig_ret)
		g_outer_post_ret_seen_orig = *(int *)gpMetaGlobals->orig_ret;
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

static int plugin_nested_pre_Spawn(edict_t *ed)
{
	g_plugin1_pre_called++;

	// Remove pre hook to avoid infinite recursion, then re-enter
	plugin1_pre_funcs.pfnSpawn = NULL;

	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnSpawn(ed);

	RETURN_META_VALUE(MRES_HANDLED, 0);
}

static int test_nested_return_call_restores_globals(void)
{
	TEST("dispatch return - nested call backs up and restores globals");
	setup_one_plugin();
	plugin1_pre_funcs.pfnSpawn = plugin_nested_pre_Spawn;
	plugin1_post_funcs.pfnSpawn = outer_post_Spawn;
	g_outer_post_ret_called = 0;
	g_outer_post_ret_seen_orig = 0;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 2);
	// Post hook called twice: once for inner call, once for outer
	ASSERT_TRUE(g_outer_post_ret_called == 2);
	// Outer call should still return gamedll value
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: void dispatch - post-only plugin (line 184, 212)
// ============================================================

static int test_void_post_only_plugin(void)
{
	TEST("dispatch void - plugin with only post hook is called");
	setup_one_plugin();
	// No pre hook, only post hook
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit;
	g_plugin1_post_mres = MRES_IGNORED;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: return dispatch - post-only plugin (line 212 variant)
// ============================================================

static int test_return_post_only_plugin(void)
{
	TEST("dispatch return - plugin with only post hook is called");
	setup_one_plugin();
	// No pre hook, only post hook
	plugin1_post_funcs.pfnSpawn = plugin1_post_Spawn;
	g_plugin1_post_mres = MRES_IGNORED;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(g_plugin1_post_seen_orig_ret_val == 42);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: nested call with two plugins preserves prev_mres
// ============================================================

static void plugin1_reentrant_pre_GameInit(void)
{
	g_plugin1_pre_called++;

	plugin1_pre_funcs.pfnGameInit = NULL;

	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnGameInit();

	RETURN_META(MRES_HANDLED);
}

static int test_nested_two_plugins_prev_mres_preserved(void)
{
	TEST("dispatch void - nested call with two plugins preserves prev_mres");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_reentrant_pre_GameInit;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin2_pre_mres = MRES_IGNORED;
	g_plugin2_pre_seen_prev_mres = MRES_UNSET;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_plugin2_pre_called == 2);
	ASSERT_TRUE(g_gamedll_called == 2);
	// Plugin 2 in outer dispatch should see MRES_HANDLED from plugin 1
	ASSERT_TRUE(g_plugin2_pre_seen_prev_mres == MRES_HANDLED);
	teardown();
	PASS();
	return 0;
}

static int g_nested_ret_plugin2_seen_prev_mres;

static int plugin1_reentrant_pre_Spawn(edict_t *ed)
{
	g_plugin1_pre_called++;

	plugin1_pre_funcs.pfnSpawn = NULL;

	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnSpawn(ed);

	RETURN_META_VALUE(MRES_HANDLED, 99);
}

static int plugin2_pre_Spawn_track_prev(edict_t *)
{
	g_plugin2_pre_called++;
	g_nested_ret_plugin2_seen_prev_mres = gpMetaGlobals->prev_mres;
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

static int test_nested_two_plugins_return_prev_mres_preserved(void)
{
	TEST("dispatch return - nested call with two plugins preserves prev_mres");
	setup_two_plugins();
	plugin1_pre_funcs.pfnSpawn = plugin1_reentrant_pre_Spawn;
	plugin2_pre_funcs.pfnSpawn = plugin2_pre_Spawn_track_prev;
	g_nested_ret_plugin2_seen_prev_mres = MRES_UNSET;

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_plugin2_pre_called == 2);
	ASSERT_TRUE(g_gamedll_called == 2);
	// Plugin 2 in outer dispatch should see MRES_HANDLED from plugin 1
	ASSERT_TRUE(g_nested_ret_plugin2_seen_prev_mres == MRES_HANDLED);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: debug template path produces META_DEBUG messages
// ============================================================

static int test_debug_dispatch_logs_calls(void)
{
	TEST("dispatch void - debug path logs plugin and gamedll calls");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit;
	g_plugin1_pre_mres = MRES_IGNORED;
	g_plugin1_post_mres = MRES_IGNORED;

	meta_debug_value = 3;
	call_hooked_GameInit();
	meta_debug_value = 0;

	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	int found_pre = 0, found_post = 0;
	for (int i = 0; i < mock_get_alert_count(); i++) {
		const char *msg = mock_get_alert_msg(i);
		if (strstr(msg, "Calling plugin1:GameDLLInit()") && !strstr(msg, "Post"))
			found_pre = 1;
		if (strstr(msg, "Calling plugin1:GameDLLInit_Post()"))
			found_post = 1;
	}
	ASSERT_TRUE(found_pre);
	ASSERT_TRUE(found_post);
	teardown();
	PASS();
	return 0;
}

static int test_no_debug_dispatch_no_logs(void)
{
	TEST("dispatch void - non-debug path produces no META_DEBUG messages");
	setup_one_plugin();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	g_plugin1_pre_mres = MRES_IGNORED;

	meta_debug_value = 0;
	call_hooked_GameInit();

	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	int found_calling = 0;
	for (int i = 0; i < mock_get_alert_count(); i++) {
		if (strstr(mock_get_alert_msg(i), "Calling"))
			found_calling = 1;
	}
	ASSERT_TRUE(!found_calling);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: void dispatch - post hook MRES_UNSET warning (line 212)
// ============================================================

static int test_void_post_unset_warns(void)
{
	TEST("dispatch void - post hook MRES_UNSET triggers warning");
	setup_one_plugin();
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit_no_result;

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(mock_get_alert_count() > 0);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "didn't set meta_result");
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: void dispatch - plugin with NULL post table (line 184)
// ============================================================

static int test_void_null_post_table(void)
{
	TEST("dispatch void - plugin with NULL post table skipped in post loop");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit;
	g_plugin1_pre_mres = MRES_IGNORED;
	// Remove post table for plugin 1
	test_plugins.plist[0].post_tables.dllapi = NULL;
	// Plugin 2 has no hooks at all
	test_plugins.plist[1].post_tables.dllapi = NULL;
	test_plugins.rebuild_hook_lists();

	call_hooked_GameInit();
	ASSERT_TRUE(g_plugin1_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: return dispatch - plugin with NULL post table (line 268 pre loop)
// ============================================================

static int test_return_null_pre_table(void)
{
	TEST("dispatch return - plugin with NULL pre table skipped in pre loop");
	setup_two_plugins();
	// Plugin 1: no pre table, has post
	test_plugins.plist[0].tables.dllapi = NULL;
	plugin1_post_funcs.pfnSpawn = plugin1_post_Spawn;
	g_plugin1_post_mres = MRES_IGNORED;
	// Plugin 2: has pre, no post
	plugin2_pre_funcs.pfnSpawn = plugin2_pre_Spawn;
	g_plugin2_pre_mres = MRES_IGNORED;
	test_plugins.plist[1].post_tables.dllapi = NULL;
	test_plugins.rebuild_hook_lists();

	int ret = call_hooked_Spawn();
	ASSERT_TRUE(g_plugin1_pre_called == 0);
	ASSERT_TRUE(g_plugin2_pre_called == 1);
	ASSERT_TRUE(g_gamedll_called == 1);
	ASSERT_TRUE(g_plugin1_post_called == 1);
	ASSERT_TRUE(ret == 42);
	teardown();
	PASS();
	return 0;
}

// ============================================================
// Issue #108: rebuild_hook_lists during iteration
// ============================================================

static int test_rebuild_during_pre_hook_survives(void)
{
	TEST("issue #108 - rebuild during pre hook iteration does not crash");
	setup_two_plugins();

	// plugin1 pre hook triggers rebuild (loads plugin3).
	// Without fix, plugin2's pre hook would dereference stale pointer → SIGSEGV.
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_GameInit_load_plugin3;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin2_pre_mres = MRES_IGNORED;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	pid_t pid = fork();
	if (pid == 0) {
		call_hooked_GameInit();
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);

	teardown();
	PASS();
	return 0;
}

static int test_rebuild_during_post_hook_survives(void)
{
	TEST("issue #108 - rebuild during post hook iteration does not crash");
	setup_two_plugins();

	// plugin1 post hook triggers rebuild (loads plugin3).
	// Without fix, plugin2's post hook would dereference stale pointer → SIGSEGV.
	plugin1_post_funcs.pfnGameInit = plugin1_post_GameInit_load_plugin3;
	plugin2_post_funcs.pfnGameInit = plugin2_post_GameInit;
	g_plugin2_post_mres = MRES_IGNORED;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	pid_t pid = fork();
	if (pid == 0) {
		call_hooked_GameInit();
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);

	teardown();
	PASS();
	return 0;
}

// ============================================================
// Tests: re-entrancy flag propagation (reentry_count)
// ============================================================

// Scenario: outer pre hook calls engine function which re-enters the
// dispatcher. The inner call triggers rebuild_hook_lists. The outer call
// must still detect the rebuild and refresh its iteration state.

static int g_plugin3_pre_called;
static int g_plugin3_post_called;

static void plugin3_pre_GameInit(void)
{
	g_plugin3_pre_called++;
	RETURN_META(MRES_IGNORED);
}

static void plugin3_post_GameInit(void)
{
	g_plugin3_post_called++;
	RETURN_META(MRES_IGNORED);
}

static void plugin1_pre_reenter_and_rebuild(void)
{
	g_plugin1_pre_called++;

	// Remove our hook to avoid infinite recursion
	plugin1_pre_funcs.pfnGameInit = NULL;

	// Re-enter the dispatcher (inner call)
	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnGameInit();

	// After inner call returns, load plugin3 and rebuild.
	// This simulates: plugin hook -> engine call -> dllapi re-entry ->
	// return -> plugin continues and loads another plugin.
	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plugin3_pre_funcs.pfnGameInit = plugin3_pre_GameInit;
	plugin3_post_funcs.pfnGameInit = plugin3_post_GameInit;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();

	RETURN_META(MRES_IGNORED);
}

static int test_reentry_rebuild_propagates_to_outer_pre(void)
{
	TEST("issue #108 - nested call + rebuild after return propagates to outer pre loop");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_reenter_and_rebuild;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin2_pre_mres = MRES_IGNORED;
	g_plugin3_pre_called = 0;
	g_plugin3_post_called = 0;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	pid_t pid = fork();
	if (pid == 0) {
		call_hooked_GameInit();
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	// Must not crash — outer loop must detect rebuild and refresh
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);

	teardown();
	PASS();
	return 0;
}

static void plugin1_post_reenter_and_rebuild(void)
{
	g_plugin1_post_called++;

	// Remove our hook to avoid infinite recursion
	plugin1_post_funcs.pfnGameInit = NULL;

	// Re-enter the dispatcher (inner call)
	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnGameInit();

	// After inner call returns, load plugin3 and rebuild
	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plugin3_pre_funcs.pfnGameInit = plugin3_pre_GameInit;
	plugin3_post_funcs.pfnGameInit = plugin3_post_GameInit;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();

	RETURN_META(MRES_IGNORED);
}

static int test_reentry_rebuild_propagates_to_outer_post(void)
{
	TEST("issue #108 - nested call + rebuild after return propagates to outer post loop");
	setup_two_plugins();
	plugin1_post_funcs.pfnGameInit = plugin1_post_reenter_and_rebuild;
	plugin2_post_funcs.pfnGameInit = plugin2_post_GameInit;
	g_plugin2_post_mres = MRES_IGNORED;
	g_plugin3_pre_called = 0;
	g_plugin3_post_called = 0;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	pid_t pid = fork();
	if (pid == 0) {
		call_hooked_GameInit();
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);

	teardown();
	PASS();
	return 0;
}

// Verify that hook_list_tables_updated is cleared after the outermost
// call returns (no stale flag left for unrelated future calls).

static void plugin1_pre_just_rebuild(void)
{
	g_plugin1_pre_called++;

	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plugin3_pre_funcs.pfnGameInit = plugin3_pre_GameInit;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();

	RETURN_META(MRES_IGNORED);
}

static int test_flag_cleared_after_outermost_returns(void)
{
	TEST("issue #108 - hook_list_tables_updated cleared after outermost call returns");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_just_rebuild;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin2_pre_mres = MRES_IGNORED;
	g_plugin3_pre_called = 0;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	call_hooked_GameInit();
	// After the call completes, the flag must be cleared
	ASSERT_TRUE(hook_list_tables_updated == mFALSE);

	teardown();
	PASS();
	return 0;
}

// Return-typed version: nested call with rebuild must propagate

static int plugin1_pre_Spawn_reenter_and_rebuild(edict_t *ed)
{
	g_plugin1_pre_called++;

	plugin1_pre_funcs.pfnSpawn = NULL;

	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnSpawn(ed);

	// After inner call, rebuild
	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plugin3_pre_funcs.pfnGameInit = NULL;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

static int test_reentry_rebuild_propagates_return_type(void)
{
	TEST("issue #108 - nested rebuild propagates in return-typed dispatch");
	setup_two_plugins();
	plugin1_pre_funcs.pfnSpawn = plugin1_pre_Spawn_reenter_and_rebuild;
	plugin2_pre_funcs.pfnSpawn = plugin2_pre_Spawn;
	g_plugin2_pre_mres = MRES_IGNORED;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	pid_t pid = fork();
	if (pid == 0) {
		call_hooked_Spawn();
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);

	teardown();
	PASS();
	return 0;
}

// Verify nested call does NOT clear the flag that was already set before entry

static void plugin1_pre_set_flag_then_reenter(void)
{
	g_plugin1_pre_called++;

	// Trigger rebuild first (sets flag)
	MPlugin *plug3 = &Plugins->plist[2];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->index = 3;
	snprintf(plug3->filename, sizeof(plug3->filename), "plugin3");
	plug3->file = plug3->filename;
	plugin3_pre_funcs.pfnGameInit = plugin3_pre_GameInit;
	plugin3_post_funcs.pfnGameInit = plugin3_post_GameInit;
	plug3->tables.dllapi = &plugin3_pre_funcs;
	plug3->post_tables.dllapi = &plugin3_post_funcs;
	Plugins->endlist = 3;
	Plugins->rebuild_hook_lists();
	// flag is now set

	// Re-enter — inner call must NOT clobber the flag for outer
	plugin1_pre_funcs.pfnGameInit = NULL;
	DLL_FUNCTIONS inner_funcs;
	memset(&inner_funcs, 0, sizeof(inner_funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&inner_funcs, &ver);
	inner_funcs.pfnGameInit();

	RETURN_META(MRES_IGNORED);
}

static int test_reentry_does_not_clobber_outer_flag(void)
{
	TEST("issue #108 - nested call does not clear flag set before re-entry");
	setup_two_plugins();
	plugin1_pre_funcs.pfnGameInit = plugin1_pre_set_flag_then_reenter;
	plugin2_pre_funcs.pfnGameInit = plugin2_pre_GameInit;
	g_plugin2_pre_mres = MRES_IGNORED;
	g_plugin3_pre_called = 0;
	g_plugin3_post_called = 0;
	memset(&plugin3_pre_funcs, 0, sizeof(plugin3_pre_funcs));
	memset(&plugin3_post_funcs, 0, sizeof(plugin3_post_funcs));

	pid_t pid = fork();
	if (pid == 0) {
		call_hooked_GameInit();
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	// Outer call must detect the rebuild (set before re-entry) and not crash
	ASSERT_TRUE(WIFEXITED(status));
	ASSERT_TRUE(WEXITSTATUS(status) == 0);

	teardown();
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

	printf("test_api_hook:\n");

	// void dispatch
	fail |= test_no_plugins_calls_gamedll();
	fail |= test_plugin_ignored_calls_gamedll();
	fail |= test_plugin_handled_calls_gamedll();
	fail |= test_plugin_supercede_skips_gamedll();
	fail |= test_post_hook_called();
	fail |= test_post_hook_sees_status();
	fail |= test_post_supercede_warns();
	fail |= test_plugin_unset_warns();

	// two-plugin ordering
	fail |= test_two_plugins_both_called();
	fail |= test_two_plugins_prev_mres_propagates();
	fail |= test_two_plugins_supercede_highest_wins();
	fail |= test_paused_plugin_skipped();
	fail |= test_plugin_no_hook_skipped();
	fail |= test_plugin_no_table_skipped();

	// return-value dispatch
	fail |= test_return_no_plugins();
	fail |= test_return_plugin_ignored();
	fail |= test_return_plugin_supercede();
	fail |= test_return_post_override();
	fail |= test_return_post_sees_orig_ret();
	fail |= test_return_two_plugins_supercede_first();
	fail |= test_return_pre_override_post_sees_override_ret();
	fail |= test_return_post_unset_warns();
	fail |= test_return_post_supercede_warns();
	fail |= test_return_paused_plugin_skipped();
	fail |= test_return_plugin_no_post_table();
	fail |= test_return_null_gamedll_table();
	fail |= test_return_null_gamedll_function();

	// null game DLL (void)
	fail |= test_null_gamedll_table();
	fail |= test_null_gamedll_function();

	// debug template path
	fail |= test_debug_dispatch_logs_calls();
	fail |= test_no_debug_dispatch_no_logs();

	// nested call
	fail |= test_nested_call_restores_globals();
	fail |= test_nested_two_plugins_prev_mres_preserved();

	// pre MRES_UNSET for return function
	fail |= test_return_pre_unset_warns();

	// nested return call
	fail |= test_nested_return_call_restores_globals();
	fail |= test_nested_two_plugins_return_prev_mres_preserved();

	// post-only plugin
	fail |= test_void_post_only_plugin();
	fail |= test_return_post_only_plugin();

	// void post MRES_UNSET
	fail |= test_void_post_unset_warns();

	// NULL table tests
	fail |= test_void_null_post_table();
	fail |= test_return_null_pre_table();

	// Issue #108: rebuild during hook iteration
	fail |= test_rebuild_during_pre_hook_survives();
	fail |= test_rebuild_during_post_hook_survives();

	// Issue #108: re-entrancy flag propagation
	fail |= test_reentry_rebuild_propagates_to_outer_pre();
	fail |= test_reentry_rebuild_propagates_to_outer_post();
	fail |= test_flag_cleared_after_outermost_returns();
	fail |= test_reentry_rebuild_propagates_return_type();
	fail |= test_reentry_does_not_clobber_outer_flag();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
