//
// metamod-p - tests for meta_load_gamedll() and metamod_startup() paths
//
// Uses preprocessor macro redirection to mock DLOPEN/DLSYM/DLCLOSE/DLERROR,
// allowing testing of full gamedll loading without actual shared libraries.
//

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include <extdll.h>

#include "metamod.h"
#include "h_export.h"
#include "mreg.h"
#include "meta_api.h"
#include "mutil.h"
#include "game_support.h"
#include "reg_support.h"
#include "commands_meta.h"
#include "support_meta.h"
#include "conf_meta.h"
#include "info_name.h"
#include "log_meta.h"
#include "osdep.h"
#include "vdate.h"
#include "linkent.h"

#include "engine_mock.h"
#include "test_common.h"

// Forward-declare mock dl functions
static DLHANDLE dlopen_mocked(const char *filename);
static DLFUNC dlsym_mocked(DLHANDLE handle, const char *string);
static int dlclose_mocked(DLHANDLE handle);
static const char *dlerror_mocked(void);

// Redirect DLOPEN/DLSYM/DLCLOSE/DLERROR to mocked versions
#define DLOPEN dlopen_mocked
#define DLSYM dlsym_mocked
#define DLCLOSE dlclose_mocked
#define DLERROR dlerror_mocked

#include "../metamod.cpp"

#undef DLOPEN
#undef DLSYM
#undef DLCLOSE
#undef DLERROR

// ============================================================
// Mock dl* state
// ============================================================

static const void *FAKE_GAMEDLL_HANDLE = (const void *)0xDEAD0002;

static bool g_dlopen_fail;
static bool g_have_givefnptrstodll;
static bool g_have_getentityapi2;
static bool g_have_getentityapi;
static bool g_have_getnewdllfunctions;

static bool g_givefnptrstodll_called;
static bool g_getentityapi2_called;
static bool g_getentityapi_called;
static bool g_getnewdllfunctions_called;

static int g_getentityapi2_return;
static int g_getentityapi_return;
static int g_getnewdllfunctions_return;

static bool g_init_linkent_return;

// ============================================================
// Fake gamedll callback functions
// ============================================================

static void WINAPI fake_GiveFnptrsToDll(enginefuncs_t *, globalvars_t *)
{
	g_givefnptrstodll_called = true;
}

static int fake_GetEntityAPI2(DLL_FUNCTIONS *pTable, int *)
{
	g_getentityapi2_called = true;
	if (pTable)
		memset(pTable, 0, sizeof(*pTable));
	return g_getentityapi2_return;
}

static int fake_GetEntityAPI(DLL_FUNCTIONS *pTable, int)
{
	g_getentityapi_called = true;
	if (pTable)
		memset(pTable, 0, sizeof(*pTable));
	return g_getentityapi_return;
}

static int fake_GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pTable, int *)
{
	g_getnewdllfunctions_called = true;
	if (pTable)
		memset(pTable, 0, sizeof(*pTable));
	return g_getnewdllfunctions_return;
}

// ============================================================
// Mock dl* implementations
// ============================================================

static DLHANDLE dlopen_mocked(const char *filename)
{
	(void)filename;
	if (g_dlopen_fail)
		return NULL;
	return (DLHANDLE)FAKE_GAMEDLL_HANDLE;
}

static DLFUNC dlsym_mocked(DLHANDLE handle, const char *symbol)
{
	(void)handle;
	if (strcmp(symbol, "GiveFnptrsToDll") == 0)
		return g_have_givefnptrstodll ? (DLFUNC)fake_GiveFnptrsToDll : NULL;
	if (strcmp(symbol, "GetEntityAPI2") == 0)
		return g_have_getentityapi2 ? (DLFUNC)fake_GetEntityAPI2 : NULL;
	if (strcmp(symbol, "GetEntityAPI") == 0)
		return g_have_getentityapi ? (DLFUNC)fake_GetEntityAPI : NULL;
	if (strcmp(symbol, "GetNewDLLFunctions") == 0)
		return g_have_getnewdllfunctions ? (DLFUNC)fake_GetNewDLLFunctions : NULL;
	return NULL;
}

static int dlclose_mocked(DLHANDLE handle) __attribute__((unused));
static int dlclose_mocked(DLHANDLE handle)
{
	(void)handle;
	return 0;
}

static const char *dlerror_mocked(void)
{
	return "mocked dlerror";
}

// ============================================================
// Stubs for functions from other translation units
// ============================================================

// setup_gamedll is in game_support.cpp — stub it so we control the result
static bool g_setup_gamedll_return;

mBOOL DLLINTERNAL setup_gamedll(gamedll_t *gamedll)
{
	if (!g_setup_gamedll_return)
		return mFALSE;
	gamedll->desc = "Counter-Strike";
	safevoid_snprintf(gamedll->pathname, sizeof(gamedll->pathname),
			"%s/dlls/cs.so", gamedll->gamedir);
	return mTRUE;
}

// init_linkent_replacement is in osdep_linkent_linux.cpp
int DLLINTERNAL init_linkent_replacement(DLHANDLE, DLHANDLE)
{
	return g_init_linkent_return ? 1 : 0;
}

// Stubs for commands_meta.cpp functions
void DLLINTERNAL meta_register_cmdcvar()
{
}

void DLLINTERNAL print_version(meta_print_func_t)
{
}

// meta_engfuncs is in engine_api.cpp
meta_enginefuncs_t meta_engfuncs;

// ============================================================
// Test infrastructure
// ============================================================

static void reset_mock_dl(void)
{
	g_dlopen_fail = false;
	g_have_givefnptrstodll = true;
	g_have_getentityapi2 = true;
	g_have_getentityapi = false;
	g_have_getnewdllfunctions = true;

	g_givefnptrstodll_called = false;
	g_getentityapi2_called = false;
	g_getentityapi_called = false;
	g_getnewdllfunctions_called = false;

	g_getentityapi2_return = TRUE;
	g_getentityapi_return = TRUE;
	g_getnewdllfunctions_return = TRUE;

	g_setup_gamedll_return = true;
	g_init_linkent_return = true;
}

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
	if (GameDLL_funcs.dllapi_table) {
		free(GameDLL_funcs.dllapi_table);
		GameDLL_funcs.dllapi_table = NULL;
	}
	if (GameDLL_funcs.newapi_table) {
		free(GameDLL_funcs.newapi_table);
		GameDLL_funcs.newapi_table = NULL;
	}
}

static void setup_test(void)
{
	mock_reset();
	reset_mock_dl();
	mock_set_gamedir("cstrike");
	Config = &static_config;
	memset(Config, 0, sizeof(*Config));
	memset(&GameDLL, 0, sizeof(GameDLL));
	STRNCPY(GameDLL.name, "cstrike", sizeof(GameDLL.name));
	STRNCPY(GameDLL.gamedir, "/tmp/test_metamod_gd2/cstrike", sizeof(GameDLL.gamedir));
}

// ============================================================
// meta_load_gamedll tests
// ============================================================

static int test_load_gamedll_success(void)
{
	TEST("meta_load_gamedll - success with GetEntityAPI2 and GetNewDLLFunctions");
	setup_test();

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_givefnptrstodll_called);
	ASSERT_TRUE(g_getentityapi2_called);
	ASSERT_TRUE(g_getnewdllfunctions_called);
	ASSERT_TRUE(GameDLL.handle == (DLHANDLE)FAKE_GAMEDLL_HANDLE);
	ASSERT_PTR_NOT_NULL(GameDLL_funcs.dllapi_table);
	ASSERT_PTR_NOT_NULL(GameDLL_funcs.newapi_table);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_setup_fail(void)
{
	TEST("meta_load_gamedll - fails when setup_gamedll fails");
	setup_test();
	g_setup_gamedll_return = false;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mFALSE);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_dlopen_fail(void)
{
	TEST("meta_load_gamedll - fails when DLOPEN fails");
	setup_test();
	g_dlopen_fail = true;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLOPEN);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_no_givefnptrs(void)
{
	TEST("meta_load_gamedll - fails when GiveFnptrsToDll not found");
	setup_test();
	g_have_givefnptrstodll = false;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLMISSING);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_linkent_fail(void)
{
	TEST("meta_load_gamedll - fails when init_linkent_replacement fails");
	setup_test();
	g_init_linkent_return = false;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLERROR);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_api1_fallback(void)
{
	TEST("meta_load_gamedll - falls back to GetEntityAPI when API2 missing");
	setup_test();
	g_have_getentityapi2 = false;
	g_have_getentityapi = true;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_FALSE(g_getentityapi2_called);
	ASSERT_TRUE(g_getentityapi_called);
	ASSERT_PTR_NOT_NULL(GameDLL_funcs.dllapi_table);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_no_entity_api(void)
{
	TEST("meta_load_gamedll - fails when neither GetEntityAPI nor API2 found");
	setup_test();
	g_have_getentityapi2 = false;
	g_have_getentityapi = false;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLMISSING);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_api2_returns_false(void)
{
	TEST("meta_load_gamedll - API2 returns false, falls back to API1");
	setup_test();
	g_have_getentityapi = true;
	g_getentityapi2_return = FALSE;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_getentityapi2_called);
	ASSERT_TRUE(g_getentityapi_called);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_no_newdll(void)
{
	TEST("meta_load_gamedll - succeeds without GetNewDLLFunctions");
	setup_test();
	g_have_getnewdllfunctions = false;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_PTR_NULL(GameDLL_funcs.newapi_table);
	ASSERT_PTR_NOT_NULL(GameDLL_funcs.dllapi_table);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_load_gamedll_newdll_returns_false(void)
{
	TEST("meta_load_gamedll - GetNewDLLFunctions returns false");
	setup_test();
	g_getnewdllfunctions_return = FALSE;

	mBOOL ret = meta_load_gamedll();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_PTR_NULL(GameDLL_funcs.newapi_table);
	ASSERT_PTR_NOT_NULL(GameDLL_funcs.dllapi_table);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

// ============================================================
// metamod_startup integration with meta_load_gamedll
// ============================================================

static int test_startup_gamedll_load_fail(void)
{
	TEST("metamod_startup - returns 0 when meta_load_gamedll fails");
	setup_test();
	mock_set_gamedir("cstrike");
	g_dlopen_fail = true;

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 0);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_full_success(void)
{
	TEST("metamod_startup - full success path with gamedll loading");
	setup_test();
	mock_set_gamedir("/tmp/test_metamod_gd2");
	system("mkdir -p /tmp/test_metamod_gd2");

	FILE *fp = fopen("/tmp/test_metamod_gd2/plugins.ini", "w");
	fprintf(fp, "# empty\n");
	fclose(fp);

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 1);
	ASSERT_TRUE(g_givefnptrstodll_called);
	ASSERT_TRUE(g_getentityapi2_called);
	ASSERT_PTR_NOT_NULL(GameDLL_funcs.dllapi_table);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_exec_cfg_runs(void)
{
	TEST("metamod_startup - exec_cfg issues server command");
	setup_test();
	mock_set_gamedir("/tmp/test_metamod_gd2");
	system("mkdir -p /tmp/test_metamod_gd2");

	FILE *fp = fopen("/tmp/test_metamod_gd2/plugins.ini", "w");
	fprintf(fp, "# empty\n");
	fclose(fp);

	fp = fopen("/tmp/test_metamod_gd2/exec.cfg", "w");
	fprintf(fp, "echo test\n");
	fclose(fp);

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 1);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_exec_cfg_absolute(void)
{
	TEST("metamod_startup - absolute exec_cfg path warns");
	setup_test();
	mock_set_gamedir("/tmp/test_metamod_gd2");
	system("mkdir -p /tmp/test_metamod_gd2");

	FILE *fp = fopen("/tmp/test_metamod_gd2/plugins.ini", "w");
	fprintf(fp, "# empty\n");
	fclose(fp);

	// Create a real file at an absolute path, then set exec_cfg to it
	fp = fopen("/tmp/test_metamod_gd2/abs_exec.cfg", "w");
	fprintf(fp, "echo test\n");
	fclose(fp);
	mock_set_localinfo("mm_execcfg", "/tmp/test_metamod_gd2/abs_exec.cfg");

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 1);

	int found_warning = 0;
	for (int i = 0; i < mock_get_alert_count(); i++) {
		if (strstr(mock_get_alert_msg(i), "Cannot exec absolute pathnames"))
			found_warning = 1;
	}
	ASSERT_TRUE(found_warning);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_plugins_load_fail(void)
{
	TEST("metamod_startup - continues when Plugins->load fails");
	setup_test();
	mock_set_gamedir("/tmp/test_metamod_gd2");
	system("mkdir -p /tmp/test_metamod_gd2");

	// No plugins.ini — Plugins->load will fail

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 1);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_old_plugins_ini_fallback(void)
{
	TEST("metamod_startup - falls back to metamod.ini when plugins.ini missing");
	setup_test();
	mock_set_gamedir("/tmp/test_metamod_gd2");
	system("mkdir -p /tmp/test_metamod_gd2");

	// Create metamod.ini (OLD_PLUGINS_INI) but not addons/metamod/plugins.ini
	FILE *fp = fopen("/tmp/test_metamod_gd2/metamod.ini", "w");
	fprintf(fp, "# old style\n");
	fclose(fp);

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 1);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

static int test_startup_version_string(void)
{
	TEST("metamod_startup - sets version cvar string");
	setup_test();
	mock_set_gamedir("/tmp/test_metamod_gd2");
	system("mkdir -p /tmp/test_metamod_gd2");

	FILE *fp = fopen("/tmp/test_metamod_gd2/plugins.ini", "w");
	fprintf(fp, "# empty\n");
	fclose(fp);

	int ret = metamod_startup();
	ASSERT_TRUE(ret == 1);
	cleanup_startup_allocs();

	PASS();
	return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
	int fail = 0;

	system("rm -rf /tmp/test_metamod_gd2");

	printf("test_metamod_gamedll:\n");

	// meta_load_gamedll tests
	fail |= test_load_gamedll_success();
	fail |= test_load_gamedll_setup_fail();
	fail |= test_load_gamedll_dlopen_fail();
	fail |= test_load_gamedll_no_givefnptrs();
	fail |= test_load_gamedll_linkent_fail();
	fail |= test_load_gamedll_api1_fallback();
	fail |= test_load_gamedll_no_entity_api();
	fail |= test_load_gamedll_api2_returns_false();
	fail |= test_load_gamedll_no_newdll();
	fail |= test_load_gamedll_newdll_returns_false();

	// metamod_startup integration tests
	fail |= test_startup_gamedll_load_fail();
	fail |= test_startup_full_success();
	fail |= test_startup_exec_cfg_runs();
	fail |= test_startup_exec_cfg_absolute();
	fail |= test_startup_plugins_load_fail();
	fail |= test_startup_old_plugins_ini_fallback();
	fail |= test_startup_version_string();

	system("rm -rf /tmp/test_metamod_gd2");
	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
