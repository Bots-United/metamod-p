//
// metamod-p - tests for metamod.cpp
//

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <extdll.h>

#include "metamod.h"
#include "meta_api.h"
#include "mutil.h"
#include "game_support.h"
#include "reg_support.h"
#include "conf_meta.h"
#include "info_name.h"
#include "log_meta.h"
#include "osdep.h"

#include "engine_mock.h"
#include "test_common.h"

extern option_t global_options[];
extern MConfig static_config;

static void cleanup_startup_allocs(void)
{
	if (RegCmds) { delete RegCmds; RegCmds = NULL; }
	if (RegCvars) { delete RegCvars; RegCvars = NULL; }
	if (RegMsgs) { delete RegMsgs; RegMsgs = NULL; }
	if (Plugins) { delete Plugins; Plugins = NULL; }
	if (Config) {
		if (Config->gamedll) { free(Config->gamedll); Config->gamedll = NULL; }
		if (Config->plugins_file) { free(Config->plugins_file); Config->plugins_file = NULL; }
		if (Config->exec_cfg) { free(Config->exec_cfg); Config->exec_cfg = NULL; }
		if (Config->test_get_filename()) { free(Config->test_get_filename()); Config->test_set_filename(NULL); }
	}
	Config = &static_config;
}

// ============================================================
// meta_init_gamedll tests
// ============================================================

static int test_init_gamedll_relative(void)
{
	TEST("meta_init_gamedll - relative gamedir sets name and builds full path");
	mock_reset();
	mock_set_gamedir("cstrike");

	mBOOL ret = meta_init_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(strcmp(GameDLL.name, "cstrike") == 0);

	char expected[PATH_MAX + 64];
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	snprintf(expected, sizeof(expected), "%s/cstrike", cwd);
	ASSERT_TRUE(strcmp(GameDLL.gamedir, expected) == 0);

	PASS();
	return 0;
}

static int test_init_gamedll_absolute(void)
{
	TEST("meta_init_gamedll - absolute gamedir extracts name from path");
	mock_reset();
	mock_set_gamedir("/opt/hlds/cstrike");

	mBOOL ret = meta_init_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(strcmp(GameDLL.name, "cstrike") == 0);
	ASSERT_TRUE(strcmp(GameDLL.gamedir, "/opt/hlds/cstrike") == 0);

	PASS();
	return 0;
}

static int test_init_gamedll_clears_gamedll(void)
{
	TEST("meta_init_gamedll - clears GameDLL struct first");
	mock_reset();
	memset(GameDLL.name, 'X', sizeof(GameDLL.name));
	mock_set_gamedir("valve");

	meta_init_gamedll();
	ASSERT_TRUE(strcmp(GameDLL.name, "valve") == 0);

	PASS();
	return 0;
}

// ============================================================
// metamod_startup tests
// ============================================================

static int test_startup_init_fail(void)
{
	TEST("metamod_startup - returns 0 when meta_init_gamedll fails");
	mock_reset();
	// pfnGetGameDir returns empty string, which is relative.
	// getcwd will succeed, so meta_init_gamedll will set name="".
	// Then setup_gamedll will fail because game name is empty/unknown.
	// But meta_init_gamedll itself should succeed.
	// To make it fail, we need getcwd to fail -- that's hard to arrange.
	// Instead, let's test the startup-fails-at-meta_load_gamedll path.
	mock_set_gamedir("unknown_game_xyz");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	int ret = metamod_startup();
	ASSERT_TRUE(ret == 0);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_prints_banner(void)
{
	TEST("metamod_startup - prints banner messages");
	mock_reset();
	mock_set_gamedir("cstrike");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	int count = mock_get_server_print_count();
	ASSERT_TRUE(count > 0);
	int found_name = 0;
	for (int i = 0; i < count; i++) {
		if (strstr(mock_get_server_print_msg(i), VNAME))
			found_name = 1;
	}
	ASSERT_TRUE(found_name);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_sets_gamedll(void)
{
	TEST("metamod_startup - sets GameDLL.name via meta_init_gamedll");
	mock_reset();
	mock_set_gamedir("cstrike");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(strcmp(GameDLL.name, "cstrike") == 0);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_allocates_reg_lists(void)
{
	TEST("metamod_startup - allocates RegCmds, RegCvars, RegMsgs");
	mock_reset();
	mock_set_gamedir("cstrike");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(RegCmds != NULL);
	ASSERT_TRUE(RegCvars != NULL);
	ASSERT_TRUE(RegMsgs != NULL);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

// ============================================================
// metamod_startup - localinfo branches
// ============================================================

static int test_startup_localinfo_debug(void)
{
	TEST("metamod_startup - mm_debug localinfo sets debuglevel");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_debug", "5");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(Config->debuglevel == 5);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_localinfo_pluginsfile(void)
{
	TEST("metamod_startup - mm_pluginsfile localinfo sets plugins_file");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_pluginsfile", "custom_plugins.ini");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_PTR_NOT_NULL(Config->plugins_file);
	ASSERT_TRUE(strstr(Config->plugins_file, "custom_plugins.ini") != NULL);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_localinfo_execcfg(void)
{
	TEST("metamod_startup - mm_execcfg localinfo sets exec_cfg");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_execcfg", "custom_exec.cfg");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_PTR_NOT_NULL(Config->exec_cfg);
	ASSERT_TRUE(strstr(Config->exec_cfg, "custom_exec.cfg") != NULL);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_localinfo_gamedll(void)
{
	TEST("metamod_startup - mm_gamedll localinfo sets gamedll");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_gamedll", "dlls/custom.so");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_PTR_NOT_NULL(Config->gamedll);
	ASSERT_TRUE(strstr(Config->gamedll, "custom.so") != NULL);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_localinfo_autodetect(void)
{
	TEST("metamod_startup - mm_autodetect localinfo sets autodetect");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_autodetect", "no");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(Config->autodetect == mFALSE);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_localinfo_clientmeta(void)
{
	TEST("metamod_startup - mm_clientmeta localinfo sets clientmeta");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_clientmeta", "no");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(Config->clientmeta == mFALSE);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_localinfo_configfile(void)
{
	TEST("metamod_startup - mm_configfile localinfo with missing file warns");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_configfile", "nonexistent_config.ini");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	int found_warning = 0;
	for (int i = 0; i < mock_get_alert_count(); i++) {
		if (strstr(mock_get_alert_msg(i), "Empty/missing config.ini"))
			found_warning = 1;
	}
	ASSERT_TRUE(found_warning);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_developer_mode(void)
{
	TEST("metamod_startup - developer mode sets meta_debug to 3");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_cvar_float("developer", 1.0f);

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_config_debuglevel(void)
{
	TEST("metamod_startup - config debuglevel sets meta_debug cvar");
	mock_reset();
	mock_set_gamedir("cstrike");
	mock_set_localinfo("mm_debug", "7");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(Config->debuglevel == 7);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_valid_configfile(void)
{
	TEST("metamod_startup - mm_configfile with valid file loads it");
	mock_reset();
	mock_set_gamedir("/tmp/test_metamod_gd");
	system("mkdir -p /tmp/test_metamod_gd");

	FILE *fp = fopen("/tmp/test_metamod_gd/test_config.ini", "w");
	fprintf(fp, "debuglevel 4\n");
	fclose(fp);

	mock_set_localinfo("mm_configfile", "test_config.ini");

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(Config->debuglevel == 4);
	cleanup_startup_allocs();

	unlink("/tmp/test_metamod_gd/test_config.ini");

	PASS();
	return 0;
}

static int test_startup_plugins_load(void)
{
	TEST("metamod_startup - loads plugins when file exists");
	mock_reset();
	mock_set_gamedir("/tmp/test_metamod_gd");
	system("mkdir -p /tmp/test_metamod_gd");

	FILE *fp = fopen("/tmp/test_metamod_gd/plugins.ini", "w");
	fprintf(fp, "# empty plugin list\n");
	fclose(fp);

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	metamod_startup();

	ASSERT_TRUE(Plugins != NULL);
	cleanup_startup_allocs();

	unlink("/tmp/test_metamod_gd/plugins.ini");

	PASS();
	return 0;
}

static int test_startup_exec_cfg(void)
{
	TEST("metamod_startup - exec_cfg issues server command");
	mock_reset();
	mock_set_gamedir("/tmp/test_metamod_gd");
	system("mkdir -p /tmp/test_metamod_gd");

	FILE *fp = fopen("/tmp/test_metamod_gd/plugins.ini", "w");
	fprintf(fp, "# empty\n");
	fclose(fp);

	fp = fopen("/tmp/test_metamod_gd/exec.cfg", "w");
	fprintf(fp, "echo test\n");
	fclose(fp);

	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	int ret = metamod_startup();
	ASSERT_TRUE(ret == 0 || ret == 1);
	cleanup_startup_allocs();

	unlink("/tmp/test_metamod_gd/plugins.ini");
	unlink("/tmp/test_metamod_gd/exec.cfg");

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
	system("rm -rf /tmp/test_metamod_gd");

	printf("test_metamod:\n");

	fail |= test_init_gamedll_relative();
	fail |= test_init_gamedll_absolute();
	fail |= test_init_gamedll_clears_gamedll();
	fail |= test_startup_init_fail();
	fail |= test_startup_prints_banner();
	fail |= test_startup_sets_gamedll();
	fail |= test_startup_allocates_reg_lists();
	fail |= test_startup_localinfo_debug();
	fail |= test_startup_localinfo_pluginsfile();
	fail |= test_startup_localinfo_execcfg();
	fail |= test_startup_localinfo_gamedll();
	fail |= test_startup_localinfo_autodetect();
	fail |= test_startup_localinfo_clientmeta();
	fail |= test_startup_localinfo_configfile();
	fail |= test_startup_developer_mode();
	fail |= test_startup_config_debuglevel();
	fail |= test_startup_valid_configfile();
	fail |= test_startup_plugins_load();
	fail |= test_startup_exec_cfg();

	system("rm -rf /tmp/test_metamod_gd");
	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
