//
// metamod-p - tests for commands_meta.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "commands_meta.h"
#include "mreg.h"
#include "mlist.h"
#include "conf_meta.h"

#include "engine_mock.h"
#include "test_common.h"

// Shared test objects
static MPluginList test_plugins("plugins.ini");
static MRegCmdList test_reg_cmds;
static MRegCvarList test_reg_cvars;
static MRegMsgList test_reg_msgs;
static MConfig test_config;
static option_t test_options[] = {
	{ NULL, CF_NONE, NULL, NULL }
};

static void setup_globals(void)
{
	mock_reset();
	Plugins = &test_plugins;
	RegCmds = &test_reg_cmds;
	RegCvars = &test_reg_cvars;
	RegMsgs = &test_reg_msgs;
	memset(&test_config, 0, sizeof(test_config));
	test_config.init(test_options);
	Config = &test_config;
}

static void teardown_globals(void)
{
	Plugins = NULL;
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
	Config = NULL;
}

// Helper to set CMD_ARGV
static const char *argv_buf[10];
static void set_argv(const char *a0, const char *a1 = NULL,
                     const char *a2 = NULL, const char *a3 = NULL)
{
	int argc = 0;
	argv_buf[argc++] = a0;
	if (a1) argv_buf[argc++] = a1;
	if (a2) argv_buf[argc++] = a2;
	if (a3) argv_buf[argc++] = a3;
	mock_set_cmd_args(argc, argv_buf, a1 ? a1 : "");
}

// ============================================================
// svr_meta dispatch tests
// ============================================================

static int test_svr_meta_version(void)
{
	TEST("svr_meta - 'version' command");
	setup_globals();
	set_argv("meta", "version");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Metamod");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_gpl(void)
{
	TEST("svr_meta - 'gpl' command");
	setup_globals();
	set_argv("meta", "gpl");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Metamod");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_game(void)
{
	TEST("svr_meta - 'game' command");
	setup_globals();
	STRNCPY(GameDLL.name, "cstrike", sizeof(GameDLL.name));
	set_argv("meta", "game");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "GameDLL info");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_config(void)
{
	TEST("svr_meta - 'config' command");
	setup_globals();
	set_argv("meta", "config");
	svr_meta();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_list(void)
{
	TEST("svr_meta - 'list' command");
	setup_globals();
	set_argv("meta", "list");
	svr_meta();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_cmds(void)
{
	TEST("svr_meta - 'cmds' command");
	setup_globals();
	set_argv("meta", "cmds");
	svr_meta();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_cvars(void)
{
	TEST("svr_meta - 'cvars' command");
	setup_globals();
	set_argv("meta", "cvars");
	svr_meta();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_refresh(void)
{
	TEST("svr_meta - 'refresh' command");
	setup_globals();
	set_argv("meta", "refresh");
	svr_meta();
	ASSERT_TRUE(mock_get_alert_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_unrecognized(void)
{
	TEST("svr_meta - unrecognized command shows usage");
	setup_globals();
	set_argv("meta", "bogus_cmd");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unrecognized");
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// svr_meta - plugin commands (no plugin loaded)
// ============================================================

static int test_svr_meta_load_usage(void)
{
	TEST("svr_meta - 'load' with too few args shows usage");
	setup_globals();
	set_argv("meta", "load");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta load");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_pause_not_found(void)
{
	TEST("svr_meta - 'pause' unknown plugin");
	setup_globals();
	set_argv("meta", "pause", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_unpause_not_found(void)
{
	TEST("svr_meta - 'unpause' unknown plugin");
	setup_globals();
	set_argv("meta", "unpause", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_unload_not_found(void)
{
	TEST("svr_meta - 'unload' unknown plugin");
	setup_globals();
	set_argv("meta", "unload", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_force_unload_not_found(void)
{
	TEST("svr_meta - 'force_unload' unknown plugin");
	setup_globals();
	set_argv("meta", "force_unload", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_reload_not_found(void)
{
	TEST("svr_meta - 'reload' unknown plugin");
	setup_globals();
	set_argv("meta", "reload", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_retry_not_found(void)
{
	TEST("svr_meta - 'retry' unknown plugin");
	setup_globals();
	set_argv("meta", "retry", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_clear_not_found(void)
{
	TEST("svr_meta - 'clear' unknown plugin");
	setup_globals();
	set_argv("meta", "clear", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

static int test_svr_meta_info_not_found(void)
{
	TEST("svr_meta - 'info' unknown plugin");
	setup_globals();
	set_argv("meta", "info", "nonexistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find");
	teardown_globals();
	PASS();
	return 0;
}

// usage when argc < 3
static int test_svr_meta_pause_usage(void)
{
	TEST("svr_meta - 'pause' with no args shows usage");
	setup_globals();
	set_argv("meta", "pause");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta pause");
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_meta_version - argc checks
// ============================================================

static int test_cmd_meta_version_usage(void)
{
	TEST("cmd_meta_version - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "version", "extra");
	cmd_meta_version();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta version");
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_meta_game_usage(void)
{
	TEST("cmd_meta_game - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "game", "extra");
	cmd_meta_game();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta game");
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_meta_refresh_usage(void)
{
	TEST("cmd_meta_refresh - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "refresh", "extra");
	cmd_meta_refresh();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta refresh");
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// client_meta tests
// ============================================================

static int test_client_meta_version(void)
{
	TEST("client_meta - 'version' command");
	setup_globals();
	set_argv("meta", "version");
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	client_meta(&ed);
	ASSERT_TRUE(mock_get_client_print_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_client_meta_list(void)
{
	TEST("client_meta - 'list' command");
	setup_globals();
	set_argv("meta", "list");
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	client_meta(&ed);
	ASSERT_TRUE(mock_get_client_print_count() > 0 || mock_get_alert_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_client_meta_aybabtu(void)
{
	TEST("client_meta - 'aybabtu' easter egg");
	setup_globals();
	set_argv("meta", "aybabtu");
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	client_meta(&ed);
	ASSERT_STR_CONTAINS(mock_get_client_print_msg(0), "All Your Base");
	teardown_globals();
	PASS();
	return 0;
}

static int test_client_meta_unknown(void)
{
	TEST("client_meta - unknown command shows usage");
	setup_globals();
	set_argv("meta", "bogus");
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	client_meta(&ed);
	ASSERT_STR_CONTAINS(mock_get_client_print_msg(0), "Unrecognized");
	teardown_globals();
	PASS();
	return 0;
}

static int test_client_meta_version_usage(void)
{
	TEST("client_meta_version - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "version", "extra");
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	client_meta(&ed);
	ASSERT_STR_CONTAINS(mock_get_client_print_msg(0), "usage: meta version");
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_meta_list_usage(void)
{
	TEST("cmd_meta_pluginlist - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "list", "extra");
	cmd_meta_pluginlist();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta list");
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_meta_cmds_usage(void)
{
	TEST("cmd_meta_cmdlist - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "cmds", "extra");
	cmd_meta_cmdlist();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta cmds");
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_meta_cvars_usage(void)
{
	TEST("cmd_meta_cvarlist - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "cvars", "extra");
	cmd_meta_cvarlist();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta cvars");
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_meta_config_usage(void)
{
	TEST("cmd_meta_config - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "config", "extra");
	cmd_meta_config();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage: meta cvars");
	teardown_globals();
	PASS();
	return 0;
}

static int test_client_meta_list_usage(void)
{
	TEST("client_meta_pluginlist - wrong argc shows usage");
	setup_globals();
	set_argv("meta", "list", "extra");
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	client_meta_pluginlist(&ed);
	ASSERT_STR_CONTAINS(mock_get_client_print_msg(0), "usage: meta list");
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_doplug tests with loaded plugin
// ============================================================

static plugin_info_t fake_info = {
	(char *)"5:13",
	(char *)"Test Plugin",
	(char *)"1.0",
	(char *)"2024/01/01",
	(char *)"Test Author",
	(char *)"http://example.com",
	(char *)"TEST",
	PT_ANYPAUSE,
	PT_ANYPAUSE,
};

static void setup_fake_plugin(void)
{
	MPlugin *pl = &test_plugins.plist[0];
	memset(pl, 0, sizeof(*pl));
	pl->index = 1;
	STRNCPY(pl->desc, "TestPlugin", sizeof(pl->desc));
	STRNCPY(pl->filename, "dlls/test_i386.so", sizeof(pl->filename));
	pl->file = pl->filename + 5;
	STRNCPY(pl->pathname, "/tmp/dlls/test_i386.so", sizeof(pl->pathname));
	pl->status = PL_RUNNING;
	pl->action = PA_NONE;
	pl->source = PS_INI;
	pl->info = &fake_info;
	pl->handle = NULL;
	test_plugins.endlist = 1;
}

static void teardown_fake_plugin(void)
{
	memset(&test_plugins.plist[0], 0, sizeof(test_plugins.plist[0]));
	test_plugins.endlist = 0;
}

static int test_cmd_doplug_pause(void)
{
	TEST("cmd_doplug - PC_PAUSE pauses running plugin");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "pause", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Paused plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_unpause(void)
{
	TEST("cmd_doplug - PC_UNPAUSE unpauses paused plugin");
	setup_globals();
	setup_fake_plugin();
	test_plugins.plist[0].status = PL_PAUSED;
	set_argv("meta", "unpause", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unpaused plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_pause_fail(void)
{
	TEST("cmd_doplug - PC_PAUSE on paused plugin fails");
	setup_globals();
	setup_fake_plugin();
	test_plugins.plist[0].status = PL_PAUSED;
	set_argv("meta", "pause", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Pause failed");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_unpause_fail(void)
{
	TEST("cmd_doplug - PC_UNPAUSE on running plugin fails");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "unpause", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unpause failed");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_unload(void)
{
	TEST("cmd_doplug - PC_UNLOAD unloads plugin");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "unload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unloaded plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_force_unload(void)
{
	TEST("cmd_doplug - PC_FORCE_UNLOAD force-unloads plugin");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "force_unload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Forced unload plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_reload_fail(void)
{
	TEST("cmd_doplug - PC_RELOAD fails on plugin without file");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "reload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Reload");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_retry_fail(void)
{
	TEST("cmd_doplug - PC_RETRY fails with no pending action");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "retry", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Retry failed");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_clear(void)
{
	TEST("cmd_doplug - PC_CLEAR clears failed plugin");
	setup_globals();
	setup_fake_plugin();
	test_plugins.plist[0].status = PL_FAILED;
	set_argv("meta", "clear", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Cleared failed plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_clear_fail(void)
{
	TEST("cmd_doplug - PC_CLEAR fails on running plugin");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "clear", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Clear failed");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_info(void)
{
	TEST("cmd_doplug - PC_INFO shows plugin info");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "info", "TestPlugin");
	svr_meta();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_by_index(void)
{
	TEST("cmd_doplug - finds plugin by index number");
	setup_globals();
	setup_fake_plugin();
	test_plugins.plist[0].status = PL_PAUSED;
	set_argv("meta", "unpause", "1");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unpaused plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_require_found(void)
{
	TEST("cmd_doplug - PC_REQUIRE finds running plugin");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "require", "TestPlugin");
	svr_meta();
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_unload_delayed(void)
{
	TEST("cmd_doplug - PC_UNLOAD delayed for restricted plugin");
	setup_globals();
	setup_fake_plugin();
	fake_info.unloadable = PT_CHANGELEVEL;
	set_argv("meta", "unload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unload delayed");
	fake_info.unloadable = PT_ANYPAUSE;
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_unload_fail_notallowed(void)
{
	TEST("cmd_doplug - PC_UNLOAD not allowed for startup-only plugin");
	setup_globals();
	setup_fake_plugin();
	fake_info.unloadable = PT_STARTUP;
	set_argv("meta", "unload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Unload failed");
	fake_info.unloadable = PT_ANYPAUSE;
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_reload_delayed(void)
{
	TEST("cmd_doplug - PC_RELOAD delayed for restricted plugin");
	setup_globals();
	setup_fake_plugin();
	fake_info.unloadable = PT_CHANGELEVEL;
	set_argv("meta", "reload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Reload delayed");
	fake_info.unloadable = PT_ANYPAUSE;
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_reload_notallowed(void)
{
	TEST("cmd_doplug - PC_RELOAD not allowed for startup-only plugin");
	setup_globals();
	setup_fake_plugin();
	fake_info.unloadable = PT_STARTUP;
	set_argv("meta", "reload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Reload not allowed");
	fake_info.unloadable = PT_ANYPAUSE;
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_not_found(void)
{
	TEST("cmd_doplug - plugin not found shows error");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "info", "NonExistent");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Couldn't find plugin");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_doplug_usage(void)
{
	TEST("cmd_doplug - too few args shows usage");
	setup_globals();
	set_argv("meta", "pause");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "usage:");
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// meta_register_cmdcvar
// ============================================================

static int test_meta_register_cmdcvar(void)
{
	TEST("meta_register_cmdcvar - registers debug and version cvars");
	setup_globals();
	meta_register_cmdcvar();
	ASSERT_TRUE(1);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// meta load with args
// ============================================================

static int test_svr_meta_load_with_args(void)
{
	TEST("svr_meta - 'load' with args calls cmd_addload");
	setup_globals();
	STRNCPY(GameDLL.gamedir, "/tmp/test_cmds_gd", sizeof(GameDLL.gamedir));
	const char *argv[] = {"meta", "load", "nonexistent_plugin"};
	mock_set_cmd_args(3, argv, "load nonexistent_plugin");
	svr_meta();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_doplug not unique match
// ============================================================

static int test_cmd_doplug_not_unique(void)
{
	TEST("cmd_doplug - non-unique match shows error");
	setup_globals();
	MPlugin *pl0 = &test_plugins.plist[0];
	MPlugin *pl1 = &test_plugins.plist[1];
	memset(pl0, 0, sizeof(*pl0));
	memset(pl1, 0, sizeof(*pl1));
	pl0->index = 1;
	pl0->status = PL_RUNNING;
	pl0->action = PA_NONE;
	pl0->info = &fake_info;
	STRNCPY(pl0->desc, "TestPluginA", sizeof(pl0->desc));
	pl0->file = pl0->filename;
	STRNCPY(pl0->filename, "a.so", sizeof(pl0->filename));
	pl1->index = 2;
	pl1->status = PL_RUNNING;
	pl1->action = PA_NONE;
	pl1->info = &fake_info;
	STRNCPY(pl1->desc, "TestPluginB", sizeof(pl1->desc));
	pl1->file = pl1->filename;
	STRNCPY(pl1->filename, "b.so", sizeof(pl1->filename));
	test_plugins.endlist = 2;
	set_argv("meta", "info", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "unique");
	memset(pl0, 0, sizeof(*pl0));
	memset(pl1, 0, sizeof(*pl1));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_doplug force_unload failure
// ============================================================

static int test_cmd_doplug_force_unload_fail(void)
{
	TEST("cmd_doplug - PC_FORCE_UNLOAD fails on invalid plugin");
	setup_globals();
	setup_fake_plugin();
	test_plugins.plist[0].status = PL_FAILED;
	test_plugins.plist[0].file = NULL;
	set_argv("meta", "force_unload", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Forced unload failed");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_doplug retry success
// ============================================================

static int test_cmd_doplug_retry_success(void)
{
	TEST("cmd_doplug - PC_RETRY succeeds for PA_UNLOAD pending action");
	setup_globals();
	setup_fake_plugin();
	test_plugins.plist[0].action = PA_UNLOAD;
	set_argv("meta", "retry", "TestPlugin");
	svr_meta();
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Retry succeeded");
	teardown_fake_plugin();
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_doplug - unexpected pcmd value
// ============================================================

static int test_cmd_doplug_unexpected_pcmd(void)
{
	TEST("cmd_doplug - unexpected pcmd value warns");
	setup_globals();
	setup_fake_plugin();
	set_argv("meta", "info", "TestPlugin");
	// Call cmd_doplug directly with an invalid pcmd value
	cmd_doplug((PLUG_CMD)99);
	// Should trigger META_WARNING and META_CONS about command failed
	int found = 0;
	for (int i = 0; i < mock_get_alert_count(); i++)
		if (strstr(mock_get_alert_msg(i), "Unexpected plug_cmd"))
			found = 1;
	ASSERT_TRUE(found);
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Command failed");
	teardown_fake_plugin();
	teardown_globals();
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
	system("rm -rf /tmp/test_cmds_gd");

	printf("test_commands_meta:\n");

	fail |= test_svr_meta_version();
	fail |= test_svr_meta_gpl();
	fail |= test_svr_meta_game();
	fail |= test_svr_meta_config();
	fail |= test_svr_meta_list();
	fail |= test_svr_meta_cmds();
	fail |= test_svr_meta_cvars();
	fail |= test_svr_meta_refresh();
	fail |= test_svr_meta_unrecognized();
	fail |= test_svr_meta_load_usage();
	fail |= test_svr_meta_pause_not_found();
	fail |= test_svr_meta_unpause_not_found();
	fail |= test_svr_meta_unload_not_found();
	fail |= test_svr_meta_force_unload_not_found();
	fail |= test_svr_meta_reload_not_found();
	fail |= test_svr_meta_retry_not_found();
	fail |= test_svr_meta_clear_not_found();
	fail |= test_svr_meta_info_not_found();
	fail |= test_svr_meta_pause_usage();
	fail |= test_cmd_meta_version_usage();
	fail |= test_cmd_meta_game_usage();
	fail |= test_cmd_meta_refresh_usage();
	fail |= test_client_meta_version();
	fail |= test_client_meta_list();
	fail |= test_client_meta_aybabtu();
	fail |= test_client_meta_unknown();
	fail |= test_client_meta_version_usage();
	fail |= test_cmd_meta_list_usage();
	fail |= test_cmd_meta_cmds_usage();
	fail |= test_cmd_meta_cvars_usage();
	fail |= test_cmd_meta_config_usage();
	fail |= test_client_meta_list_usage();
	fail |= test_meta_register_cmdcvar();

	fail |= test_cmd_doplug_pause();
	fail |= test_cmd_doplug_unpause();
	fail |= test_cmd_doplug_pause_fail();
	fail |= test_cmd_doplug_unpause_fail();
	fail |= test_cmd_doplug_unload();
	fail |= test_cmd_doplug_force_unload();
	fail |= test_cmd_doplug_reload_fail();
	fail |= test_cmd_doplug_retry_fail();
	fail |= test_cmd_doplug_clear();
	fail |= test_cmd_doplug_clear_fail();
	fail |= test_cmd_doplug_info();
	fail |= test_cmd_doplug_by_index();
	fail |= test_cmd_doplug_require_found();
	fail |= test_cmd_doplug_unload_delayed();
	fail |= test_cmd_doplug_unload_fail_notallowed();
	fail |= test_cmd_doplug_reload_delayed();
	fail |= test_cmd_doplug_reload_notallowed();
	fail |= test_cmd_doplug_not_found();
	fail |= test_cmd_doplug_usage();

	fail |= test_svr_meta_load_with_args();
	fail |= test_cmd_doplug_not_unique();
	fail |= test_cmd_doplug_force_unload_fail();
	fail |= test_cmd_doplug_retry_success();

	fail |= test_cmd_doplug_unexpected_pcmd();

	system("rm -rf /tmp/test_cmds_gd");
	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
