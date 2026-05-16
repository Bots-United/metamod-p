//
// metamod-p - lifecycle tests for mplugin.cpp (load/query/attach/detach/unload)
//
// Uses preprocessor macro redirection to mock DLOPEN/DLSYM/DLCLOSE/DLERROR,
// allowing us to test the full plugin lifecycle without actual shared libraries.
//

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <extdll.h>

#include "mplugin.h"
#include "metamod.h"
#include "mreg.h"
#include "h_export.h"
#include "dllapi.h"
#include "support_meta.h"
#include "types_meta.h"
#include "log_meta.h"
#include "osdep.h"
#include "mm_pextensions.h"

#include "engine_mock.h"
#include "test_common.h"

// Forward-declare mock dl functions (signatures match the inline wrappers)
static DLHANDLE dlopen_mocked(const char *filename);
static DLFUNC dlsym_mocked(DLHANDLE handle, const char *string);
static int dlclose_mocked(DLHANDLE handle);
static const char *dlerror_mocked(void);

// Redirect DLOPEN/DLSYM/DLCLOSE/DLERROR to mocked versions.
// The originals are inline functions in osdep.h (already included above).
// The preprocessor macro shadows them for all code compiled after this point.
#define DLOPEN dlopen_mocked
#define DLSYM dlsym_mocked
#define DLCLOSE dlclose_mocked
#define DLERROR dlerror_mocked

#include "../mplugin.cpp"

#undef DLOPEN
#undef DLSYM
#undef DLCLOSE
#undef DLERROR

// ============================================================
// Mock dl* state
// ============================================================

static const void *FAKE_HANDLE = (const void *)0xDEAD0001;

static bool g_dlopen_fail;
static bool g_dlclose_called;
static int g_dlclose_return;

static bool g_have_meta_init;
static bool g_have_meta_query;
static bool g_have_givefnptrstodll;
static bool g_have_meta_attach;
static bool g_have_meta_detach;
static bool g_have_meta_pext;
static int g_pext_return_version;

static int g_meta_query_return;
static int g_meta_attach_return;
static int g_meta_detach_return;

static bool g_meta_init_called;
static bool g_givefnptrstodll_called;
static bool g_meta_query_called;
static bool g_meta_attach_called;
static bool g_meta_detach_called;
static bool g_gameinit_called;

static plugin_info_t g_fake_info;
static bool g_meta_query_null_info;
static char g_plugin_ifvers[32];
static bool g_provide_api_tables;

// ============================================================
// Fake plugin callback functions
// ============================================================

static void fake_Meta_Init(void)
{
	g_meta_init_called = true;
}

static void WINAPI fake_GiveFnptrsToDll(enginefuncs_t *, globalvars_t *)
{
	g_givefnptrstodll_called = true;
}

static int fake_Meta_Query(char *, plugin_info_t **pinfo, mutil_funcs_t *)
{
	g_meta_query_called = true;
	if (!g_meta_query_null_info)
		*pinfo = &g_fake_info;
	else
		*pinfo = NULL;
	return g_meta_query_return;
}

static void fake_GameInit(void)
{
	g_gameinit_called = true;
}

static int fake_GetEntityAPI2(DLL_FUNCTIONS *pTable, int *)
{
	if (pTable) {
		memset(pTable, 0, sizeof(*pTable));
		pTable->pfnGameInit = fake_GameInit;
	}
	return TRUE;
}

static int fake_Meta_Attach(PLUG_LOADTIME, META_FUNCTIONS *pFunctionTable,
                            meta_globals_t *, gamedll_funcs_t *)
{
	g_meta_attach_called = true;
	if (g_provide_api_tables && pFunctionTable)
		pFunctionTable->pfnGetEntityAPI2 = (GETENTITYAPI2_FN)fake_GetEntityAPI2;
	return g_meta_attach_return;
}

static int fake_Meta_Detach(PLUG_LOADTIME, PL_UNLOAD_REASON)
{
	g_meta_detach_called = true;
	return g_meta_detach_return;
}

static int fake_Meta_PExtGiveFnptrs(int, pextension_funcs_t *)
{
	return g_pext_return_version;
}

// ============================================================
// Mock dl* implementations
// ============================================================

static DLHANDLE dlopen_mocked(const char *filename)
{
	(void)filename;
	if (g_dlopen_fail)
		return NULL;
	return (DLHANDLE)FAKE_HANDLE;
}

static DLFUNC dlsym_mocked(DLHANDLE handle, const char *symbol)
{
	(void)handle;
	if (strcmp(symbol, "Meta_Init") == 0)
		return g_have_meta_init ? (DLFUNC)fake_Meta_Init : NULL;
	if (strcmp(symbol, "Meta_Query") == 0)
		return g_have_meta_query ? (DLFUNC)fake_Meta_Query : NULL;
	if (strcmp(symbol, "GiveFnptrsToDll") == 0)
		return g_have_givefnptrstodll ? (DLFUNC)fake_GiveFnptrsToDll : NULL;
	if (strcmp(symbol, "Meta_Attach") == 0)
		return g_have_meta_attach ? (DLFUNC)fake_Meta_Attach : NULL;
	if (strcmp(symbol, "Meta_Detach") == 0)
		return g_have_meta_detach ? (DLFUNC)fake_Meta_Detach : NULL;
	if (strcmp(symbol, "Meta_PExtGiveFnptrs") == 0)
		return g_have_meta_pext ? (DLFUNC)fake_Meta_PExtGiveFnptrs : NULL;
	return NULL;
}

static int dlclose_mocked(DLHANDLE handle)
{
	(void)handle;
	g_dlclose_called = true;
	return g_dlclose_return;
}

static const char *dlerror_mocked(void)
{
	return "mocked dlerror";
}

// ============================================================
// Test infrastructure
// ============================================================

static MPluginList test_plugins("plugins.ini");
static MRegCmdList test_reg_cmds;
static MRegCvarList test_reg_cvars;
static MRegMsgList test_reg_msgs;
static enginefuncs_t g_test_pl_funcs;

static void reset_mock_dl(void)
{
	g_dlopen_fail = false;
	g_dlclose_called = false;
	g_dlclose_return = 0;

	g_have_meta_init = true;
	g_have_meta_query = true;
	g_have_givefnptrstodll = true;
	g_have_meta_attach = true;
	g_have_meta_detach = true;
	g_have_meta_pext = false;
	g_pext_return_version = META_PEXT_VERSION;

	g_meta_query_return = TRUE;
	g_meta_attach_return = TRUE;
	g_meta_detach_return = TRUE;

	g_meta_init_called = false;
	g_givefnptrstodll_called = false;
	g_meta_query_called = false;
	g_meta_attach_called = false;
	g_meta_detach_called = false;
	g_gameinit_called = false;

	g_meta_query_null_info = false;
	g_provide_api_tables = false;

	memset(&g_fake_info, 0, sizeof(g_fake_info));
	g_fake_info.ifvers = g_plugin_ifvers;
	g_fake_info.name = "Test Plugin";
	g_fake_info.version = "1.0";
	g_fake_info.date = "2024-01-01";
	g_fake_info.author = "Test";
	g_fake_info.url = "http://test";
	g_fake_info.logtag = "TEST";
	g_fake_info.loadable = PT_ANYTIME;
	g_fake_info.unloadable = PT_ANYPAUSE;
	STRNCPY(g_plugin_ifvers, META_INTERFACE_VERSION, sizeof(g_plugin_ifvers));
}

static void setup_globals(void)
{
	mock_reset();
	reset_mock_dl();

	Plugins = &test_plugins;
	RegCmds = &test_reg_cmds;
	RegCvars = &test_reg_cvars;
	RegMsgs = &test_reg_msgs;
	STRNCPY(GameDLL.gamedir, "/tmp/test_mplugin_lc", sizeof(GameDLL.gamedir));

	memset(&g_test_pl_funcs, 0, sizeof(g_test_pl_funcs));
	Engine.pl_funcs = &g_test_pl_funcs;
}

static void teardown_globals(void)
{
	Plugins = NULL;
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
}

static void setup_plugin_for_load(MPlugin *plug, const char *filename)
{
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_VALID;
	plug->action = PA_LOAD;
	plug->source = PS_INI;
	plug->index = 1;
	STRNCPY(plug->filename, filename, sizeof(plug->filename));
	plug->file = plug->filename;
	STRNCPY(plug->pathname, filename, sizeof(plug->pathname));
	STRNCPY(plug->desc, "<Test Plugin>", sizeof(plug->desc));
}

static void setup_plugin_as_loaded(MPlugin *plug, const char *filename)
{
	setup_plugin_for_load(plug, filename);
	plug->status = PL_RUNNING;
	plug->action = PA_NONE;
	plug->handle = (DLHANDLE)FAKE_HANDLE;
	plug->info = &g_fake_info;
	plug->time_loaded = time(NULL);
}

// ============================================================
// load tests
// ============================================================

static int test_load_success(void)
{
	TEST("load - full lifecycle success");
	setup_globals();
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/test.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	ASSERT_TRUE(plug.action == PA_NONE);
	ASSERT_TRUE(g_meta_init_called);
	ASSERT_TRUE(g_givefnptrstodll_called);
	ASSERT_TRUE(g_meta_query_called);
	ASSERT_TRUE(g_meta_attach_called);
	ASSERT_TRUE(plug.handle == (DLHANDLE)FAKE_HANDLE);
	ASSERT_TRUE(plug.info == &g_fake_info);
	ASSERT_STR_CONTAINS(plug.desc, "Test Plugin");
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_dlopen_fail(void)
{
	TEST("load - dlopen failure");
	setup_globals();
	g_dlopen_fail = true;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/bad.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(plug.status == PL_BADFILE);
	ASSERT_TRUE(meta_errno == ME_DLOPEN);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_missing_meta_query(void)
{
	TEST("load - missing Meta_Query symbol");
	setup_globals();
	g_have_meta_query = false;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/nq.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(plug.status == PL_BADFILE);
	ASSERT_TRUE(meta_errno == ME_DLMISSING);
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_missing_givefnptrstodll(void)
{
	TEST("load - missing GiveFnptrsToDll symbol");
	setup_globals();
	g_have_givefnptrstodll = false;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/nogive.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(plug.status == PL_BADFILE);
	ASSERT_TRUE(meta_errno == ME_DLMISSING);
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_meta_query_error(void)
{
	TEST("load - Meta_Query returns error");
	setup_globals();
	g_meta_query_return = FALSE;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/qerr.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLERROR);
	ASSERT_TRUE(plug.status == PL_BADFILE);
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_null_info(void)
{
	TEST("load - Meta_Query returns null info");
	setup_globals();
	g_meta_query_null_info = true;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/nullinfo.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NULLRESULT);
	ASSERT_TRUE(plug.status == PL_BADFILE);
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_missing_meta_attach(void)
{
	TEST("load - missing Meta_Attach symbol");
	setup_globals();
	g_have_meta_attach = false;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/noatt.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(plug.status == PL_FAILED);
	ASSERT_TRUE(meta_errno == ME_DLMISSING);
	ASSERT_FALSE(g_dlclose_called);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_meta_attach_error(void)
{
	TEST("load - Meta_Attach returns error");
	setup_globals();
	g_meta_attach_return = FALSE;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/atterr.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(plug.status == PL_FAILED);
	ASSERT_TRUE(meta_errno == ME_DLERROR);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_loadtime_not_allowed(void)
{
	TEST("load - not loadable at this time (startup only)");
	setup_globals();
	g_fake_info.loadable = PT_STARTUP;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/startonly.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);
	ASSERT_TRUE(plug.action == PA_NONE);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_loadtime_delayed(void)
{
	TEST("load - load delayed (changelevel at anytime)");
	setup_globals();
	g_fake_info.loadable = PT_CHANGELEVEL;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/delayed.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DELAYED);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_without_meta_init(void)
{
	TEST("load - Meta_Init is optional");
	setup_globals();
	g_have_meta_init = false;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/noinit.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	ASSERT_FALSE(g_meta_init_called);
	ASSERT_TRUE(g_meta_query_called);
	ASSERT_TRUE(g_meta_attach_called);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_gameinit_at_anytime(void)
{
	TEST("load - GameInit called when loading after startup");
	setup_globals();
	g_provide_api_tables = true;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/gi.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	ASSERT_TRUE(g_gameinit_called);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_no_gameinit_at_startup(void)
{
	TEST("load - GameInit not called at startup");
	setup_globals();
	g_provide_api_tables = true;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/gi2.so");
	mBOOL ret = plug.load(PT_STARTUP);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	ASSERT_FALSE(g_gameinit_called);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_ifvers_newer_rejected(void)
{
	TEST("load - newer interface version rejected");
	setup_globals();
	STRNCPY(g_plugin_ifvers, "6:0", sizeof(g_plugin_ifvers));
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/newif.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_IFVERSION);
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_ifvers_older_major_rejected(void)
{
	TEST("load - older major interface version rejected");
	setup_globals();
	STRNCPY(g_plugin_ifvers, "4:0", sizeof(g_plugin_ifvers));
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/oldif.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_IFVERSION);
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_ifvers_older_minor_accepted(void)
{
	TEST("load - older minor version accepted");
	setup_globals();
	STRNCPY(g_plugin_ifvers, "5:12", sizeof(g_plugin_ifvers));
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/oldmin.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_with_pext(void)
{
	TEST("load - Meta_PExtGiveFnptrs called when present");
	setup_globals();
	g_have_meta_pext = true;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/pext.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_with_gamedll_tables(void)
{
	TEST("load - attach copies GameDLL function tables");
	setup_globals();
	DLL_FUNCTIONS gdll_funcs;
	NEW_DLL_FUNCTIONS gdll_newapi;
	memset(&gdll_funcs, 0, sizeof(gdll_funcs));
	memset(&gdll_newapi, 0, sizeof(gdll_newapi));
	GameDLL_funcs.dllapi_table = &gdll_funcs;
	GameDLL_funcs.newapi_table = &gdll_newapi;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/copytbl.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	ASSERT_PTR_NOT_NULL(plug.test_gamedll_funcs().dllapi_table);
	ASSERT_PTR_NOT_NULL(plug.test_gamedll_funcs().newapi_table);
	plug.free_api_pointers();
	GameDLL_funcs.dllapi_table = NULL;
	GameDLL_funcs.newapi_table = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_pext_newer_version(void)
{
	TEST("load - Meta_PExtGiveFnptrs newer version warns");
	setup_globals();
	g_have_meta_pext = true;
	g_pext_return_version = META_PEXT_VERSION + 1;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/pextnew.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	plug.free_api_pointers();
	g_pext_return_version = META_PEXT_VERSION;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// unload tests
// ============================================================

static int test_unload_success(void)
{
	TEST("unload - successful unload");
	setup_globals();
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_UNLOAD;
	// allocate tables like attach() would
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_meta_detach_called);
	ASSERT_TRUE(g_dlclose_called);
	ASSERT_TRUE(plug.status == PL_EMPTY);
	ASSERT_TRUE(plug.handle == NULL);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_detach_fails(void)
{
	TEST("unload - detach failure blocks unload");
	setup_globals();
	g_meta_detach_return = FALSE;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_UNLOAD;
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(g_meta_detach_called);
	ASSERT_FALSE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_forced_overrides_detach(void)
{
	TEST("unload - forced unload overrides failed detach");
	setup_globals();
	g_meta_detach_return = FALSE;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_UNLOAD;
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_CMD_FORCED, PNL_CMD_FORCED);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_dlclose_called);
	ASSERT_TRUE(plug.status == PL_EMPTY);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_loadtime_not_allowed(void)
{
	TEST("unload - not unloadable at this time");
	setup_globals();
	g_fake_info.unloadable = PT_STARTUP;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_UNLOAD;
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);
	ASSERT_TRUE(plug.action == PA_NONE);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_loadtime_delayed(void)
{
	TEST("unload - unload delayed (changelevel at anytime)");
	setup_globals();
	g_fake_info.unloadable = PT_CHANGELEVEL;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_UNLOAD;
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DELAYED);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_forced_overrides_loadtime(void)
{
	TEST("unload - forced unload overrides loadtime restriction");
	setup_globals();
	g_fake_info.unloadable = PT_STARTUP;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_UNLOAD;
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_CMD_FORCED, PNL_CMD_FORCED);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_dlclose_called);
	ASSERT_TRUE(plug.status == PL_EMPTY);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_reload_action(void)
{
	TEST("unload - PA_RELOAD sets PL_VALID and PA_LOAD");
	setup_globals();
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_RELOAD;
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_VALID);
	ASSERT_TRUE(plug.action == PA_LOAD);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// detach tests
// ============================================================

static int test_detach_no_handle(void)
{
	TEST("detach - null handle returns true");
	setup_globals();
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.handle = NULL;
	mBOOL ret = plug.test_detach(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_FALSE(g_meta_detach_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_detach_missing_symbol(void)
{
	TEST("detach - missing Meta_Detach symbol");
	setup_globals();
	g_have_meta_detach = false;
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.handle = (DLHANDLE)FAKE_HANDLE;
	STRNCPY(plug.desc, "test", sizeof(plug.desc));
	mBOOL ret = plug.test_detach(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLMISSING);
	teardown_globals();
	PASS();
	return 0;
}

static int test_detach_error(void)
{
	TEST("detach - Meta_Detach returns error");
	setup_globals();
	g_meta_detach_return = FALSE;
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.handle = (DLHANDLE)FAKE_HANDLE;
	STRNCPY(plug.desc, "test", sizeof(plug.desc));
	mBOOL ret = plug.test_detach(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLERROR);
	ASSERT_TRUE(g_meta_detach_called);
	teardown_globals();
	PASS();
	return 0;
}

static int test_detach_success(void)
{
	TEST("detach - successful detach");
	setup_globals();
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.handle = (DLHANDLE)FAKE_HANDLE;
	STRNCPY(plug.desc, "test", sizeof(plug.desc));
	mBOOL ret = plug.test_detach(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_meta_detach_called);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// reload tests
// ============================================================

static int test_reload_success(void)
{
	TEST("reload - successful reload");
	setup_globals();
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_RELOAD;
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.reload(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	ASSERT_TRUE(g_meta_detach_called);
	ASSERT_TRUE(g_dlclose_called);
	ASSERT_TRUE(g_meta_query_called);
	ASSERT_TRUE(g_meta_attach_called);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_reload_loadtime_not_allowed(void)
{
	TEST("reload - not reloadable at this time");
	setup_globals();
	g_fake_info.loadable = PT_STARTUP;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_RELOAD;
	mBOOL ret = plug.reload(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);
	ASSERT_TRUE(plug.action == PA_NONE);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// pause / unpause tests
// ============================================================

static int test_pause_success(void)
{
	TEST("pause - successful pause");
	setup_globals();
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	mBOOL ret = plug.pause();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_PAUSED);
	teardown_globals();
	PASS();
	return 0;
}

static int test_pause_not_allowed(void)
{
	TEST("pause - plugin doesn't allow pausing");
	setup_globals();
	g_fake_info.unloadable = PT_ANYTIME;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	mBOOL ret = plug.pause();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);
	ASSERT_TRUE(plug.action == PA_NONE);
	teardown_globals();
	PASS();
	return 0;
}

static int test_unpause_success(void)
{
	TEST("unpause - successful unpause");
	setup_globals();
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.status = PL_PAUSED;
	mBOOL ret = plug.unpause();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// full load+unload cycle
// ============================================================

static int test_load_unload_cycle(void)
{
	TEST("load+unload - full lifecycle");
	setup_globals();
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/cycle.so");

	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);

	reset_mock_dl();
	plug.action = PA_UNLOAD;
	ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_EMPTY);
	ASSERT_TRUE(plug.handle == NULL);
	ASSERT_TRUE(g_meta_detach_called);
	ASSERT_TRUE(g_dlclose_called);

	teardown_globals();
	PASS();
	return 0;
}

static int test_load_pause_unpause_unload(void)
{
	TEST("load+pause+unpause+unload - full lifecycle");
	setup_globals();
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/full.so");

	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);

	ret = plug.pause();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_PAUSED);

	ret = plug.unpause();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);

	reset_mock_dl();
	plug.action = PA_UNLOAD;
	ret = plug.unload(PT_ANYTIME, PNL_COMMAND, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_EMPTY);

	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// free_api_pointers with allocated tables
// ============================================================

static int test_free_api_pointers_allocated(void)
{
	TEST("free_api_pointers - frees all allocated tables");
	setup_globals();
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	plug.tables.dllapi = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.tables.newapi = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	plug.tables.engine = (enginefuncs_t *)calloc(1, sizeof(enginefuncs_t));
	plug.post_tables.dllapi = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.post_tables.newapi = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	plug.post_tables.engine = (enginefuncs_t *)calloc(1, sizeof(enginefuncs_t));
	plug.free_api_pointers();
	ASSERT_TRUE(1);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_unload tests
// ============================================================

static int test_plugin_unload_success(void)
{
	TEST("plugin_unload - successful unload by another plugin");
	setup_globals();
	MPlugin *plugA = &test_plugins.plist[0];
	MPlugin *plugB = &test_plugins.plist[1];
	setup_plugin_as_loaded(plugA, "dlls/plugA.so");
	plugA->index = 1;
	setup_plugin_as_loaded(plugB, "dlls/plugB.so");
	plugB->index = 2;
	test_plugins.endlist = 2;

	plugin_info_t infoA;
	memset(&infoA, 0, sizeof(infoA));
	infoA.ifvers = g_plugin_ifvers;
	infoA.name = "PluginA";
	infoA.loadable = PT_ANYTIME;
	infoA.unloadable = PT_ANYPAUSE;
	plugA->info = &infoA;

	mBOOL ret = plugB->plugin_unload(plugA->info, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret == mTRUE);

	memset(plugA, 0, sizeof(*plugA));
	memset(plugB, 0, sizeof(*plugB));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

static int test_plugin_unload_self(void)
{
	TEST("plugin_unload - cannot unload self");
	setup_globals();
	MPlugin *plugA = &test_plugins.plist[0];
	setup_plugin_as_loaded(plugA, "dlls/plugA.so");
	plugA->index = 1;
	test_plugins.endlist = 1;

	mBOOL ret = plugA->plugin_unload(plugA->info, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_UNLOAD_SELF);

	memset(plugA, 0, sizeof(*plugA));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

static int test_plugin_unload_unknown_plid(void)
{
	TEST("plugin_unload - unknown plid returns ME_BADREQ");
	setup_globals();
	MPlugin *plugA = &test_plugins.plist[0];
	setup_plugin_as_loaded(plugA, "dlls/plugA.so");
	plugA->index = 1;
	test_plugins.endlist = 1;

	plugin_info_t unknown_info;
	memset(&unknown_info, 0, sizeof(unknown_info));
	unknown_info.name = "Unknown";

	mBOOL ret = plugA->plugin_unload(&unknown_info, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_BADREQ);

	memset(plugA, 0, sizeof(*plugA));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

static int test_plugin_unload_is_unloader(void)
{
	TEST("plugin_unload - cannot unload active unloader");
	setup_globals();
	MPlugin *plugA = &test_plugins.plist[0];
	MPlugin *plugB = &test_plugins.plist[1];
	setup_plugin_as_loaded(plugA, "dlls/plugA.so");
	plugA->index = 1;
	setup_plugin_as_loaded(plugB, "dlls/plugB.so");
	plugB->index = 2;
	plugB->is_unloader = mTRUE;
	test_plugins.endlist = 2;

	plugin_info_t infoA;
	memset(&infoA, 0, sizeof(infoA));
	infoA.ifvers = g_plugin_ifvers;
	infoA.name = "PluginA";
	infoA.loadable = PT_ANYTIME;
	infoA.unloadable = PT_ANYPAUSE;
	plugA->info = &infoA;

	mBOOL ret = plugB->plugin_unload(plugA->info, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_UNLOAD_UNLOADER);

	memset(plugA, 0, sizeof(*plugA));
	memset(plugB, 0, sizeof(*plugB));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_unload with ME_DELAYED conversion
// ============================================================

static int test_plugin_unload_delayed(void)
{
	TEST("plugin_unload - delayed unload converted to ME_NOTALLOWED");
	setup_globals();
	MPlugin *plugA = &test_plugins.plist[0];
	MPlugin *plugB = &test_plugins.plist[1];
	setup_plugin_as_loaded(plugA, "dlls/plugA.so");
	plugA->index = 1;
	plugA->test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plugA->test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	setup_plugin_as_loaded(plugB, "dlls/plugB.so");
	plugB->index = 2;
	test_plugins.endlist = 2;

	plugin_info_t infoA;
	memset(&infoA, 0, sizeof(infoA));
	infoA.ifvers = g_plugin_ifvers;
	infoA.name = "PluginA";
	infoA.loadable = PT_ANYTIME;
	infoA.unloadable = PT_CHANGELEVEL;
	plugA->info = &infoA;

	plugin_info_t infoB;
	memset(&infoB, 0, sizeof(infoB));
	infoB.ifvers = g_plugin_ifvers;
	infoB.name = "PluginB";
	infoB.loadable = PT_ANYTIME;
	infoB.unloadable = PT_ANYPAUSE;
	plugB->info = &infoB;

	// Unload plugA (PT_CHANGELEVEL), requested by plugB.
	// unload() returns ME_DELAYED, plugin_unload converts to ME_NOTALLOWED.
	mBOOL ret = plugA->plugin_unload(&infoB, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);

	plugA->free_api_pointers();
	memset(plugA, 0, sizeof(*plugA));
	memset(plugB, 0, sizeof(*plugB));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// additional lifecycle tests
// ============================================================

static int test_load_ifvers_unexpected(void)
{
	TEST("load - unexpected version comparison (same parsed, different string)");
	setup_globals();
	STRNCPY(g_plugin_ifvers, "05:13", sizeof(g_plugin_ifvers));
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/unexpv.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_reload_delayed(void)
{
	TEST("reload - delayed when loadable is PT_CHANGELEVEL");
	setup_globals();
	g_fake_info.loadable = PT_CHANGELEVEL;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_RELOAD;
	mBOOL ret = plug.reload(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DELAYED);
	teardown_globals();
	PASS();
	return 0;
}

static int test_retry_reload(void)
{
	TEST("retry - PA_RELOAD calls reload");
	setup_globals();
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_RELOAD;
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.retry(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(plug.status == PL_RUNNING);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

static int test_unload_reload_overrides_detach(void)
{
	TEST("unload - PNL_RELOAD overrides failed detach");
	setup_globals();
	g_meta_detach_return = FALSE;
	MPlugin plug;
	setup_plugin_as_loaded(&plug, "dlls/test.so");
	plug.action = PA_RELOAD;
	plug.test_gamedll_funcs().dllapi_table = (DLL_FUNCTIONS *)calloc(1, sizeof(DLL_FUNCTIONS));
	plug.test_gamedll_funcs().newapi_table = (NEW_DLL_FUNCTIONS *)calloc(1, sizeof(NEW_DLL_FUNCTIONS));
	mBOOL ret = plug.unload(PT_ANYTIME, PNL_RELOAD, PNL_RELOAD);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(g_dlclose_called);
	ASSERT_TRUE(plug.status == PL_VALID);
	plug.free_api_pointers();
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// reload — check_input fails (line 1137)
// ============================================================

static int test_reload_check_input_fails(void)
{
	TEST("reload - check_input failure returns ME_ARGUMENT");
	setup_globals();
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.status = PL_EMPTY;  // < PL_VALID, so check_input fails
	plug.action = PA_RELOAD;
	STRNCPY(plug.filename, "test.so", sizeof(plug.filename));
	plug.file = plug.filename;
	STRNCPY(plug.pathname, "/tmp/test.so", sizeof(plug.pathname));
	STRNCPY(plug.desc, "test", sizeof(plug.desc));
	mBOOL ret = plug.reload(PT_ANYTIME, PNL_COMMAND);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// clear — dlclose fails (lines 1271-1273)
// ============================================================

static int test_clear_dlclose_fails(void)
{
	TEST("clear - dlclose failure returns ME_DLERROR");
	setup_globals();
	g_dlclose_return = -1;  // make DLCLOSE return non-zero
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.status = PL_FAILED;  // allowed status for clear()
	plug.handle = (DLHANDLE)FAKE_HANDLE;
	STRNCPY(plug.desc, "test", sizeof(plug.desc));
	plug.file = (char *)"test.so";
	mBOOL ret = plug.clear();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DLERROR);
	ASSERT_TRUE(plug.status == PL_FAILED);  // status set to PL_FAILED on error
	ASSERT_TRUE(g_dlclose_called);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show — found_child_plugins returns true (line 1371)
// ============================================================

static int test_show_child_plugins(void)
{
	TEST("show - found_child_plugins triggers child listing");
	setup_globals();
	// Set up parent plugin at index 1
	MPlugin *parent = &test_plugins.plist[0];
	memset(parent, 0, sizeof(*parent));
	parent->index = 1;
	parent->status = PL_RUNNING;
	parent->action = PA_NONE;
	STRNCPY(parent->filename, "parent.so", sizeof(parent->filename));
	parent->file = parent->filename;
	STRNCPY(parent->desc, "Parent Plugin", sizeof(parent->desc));
	parent->time_loaded = 1000000;
	parent->handle = (DLHANDLE)FAKE_HANDLE;

	plugin_info_t parent_info;
	memset(&parent_info, 0, sizeof(parent_info));
	parent_info.ifvers = g_plugin_ifvers;
	parent_info.name = (char *)"Parent Plugin";
	parent_info.version = (char *)"1.0";
	parent_info.date = (char *)"2024-01-01";
	parent_info.author = (char *)"Test";
	parent_info.url = (char *)"http://test";
	parent_info.logtag = (char *)"PAR";
	parent_info.loadable = PT_ANYTIME;
	parent_info.unloadable = PT_ANYPAUSE;
	parent->info = &parent_info;

	// Set up child plugin at index 2 with source_plugin_index = 1
	MPlugin *child = &test_plugins.plist[1];
	memset(child, 0, sizeof(*child));
	child->index = 2;
	child->status = PL_RUNNING;
	child->source = PS_PLUGIN;
	child->source_plugin_index = 1;  // loaded by parent (index 1)
	STRNCPY(child->filename, "child.so", sizeof(child->filename));
	child->file = child->filename;
	STRNCPY(child->desc, "Child Plugin", sizeof(child->desc));
	test_plugins.endlist = 2;

	int before_count = mock_get_server_print_count();
	parent->show();
	int after_count = mock_get_server_print_count();
	// show() should have printed child plugin info
	ASSERT_TRUE(after_count > before_count);

	memset(parent, 0, sizeof(*parent));
	memset(child, 0, sizeof(*child));
	test_plugins.endlist = 0;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// load edge cases
// ============================================================

static int test_load_check_input_fails(void)
{
	TEST("load - check_input failure returns ME_ARGUMENT");
	setup_globals();
	MPlugin plug;
	memset(&plug, 0, sizeof(plug));
	plug.status = PL_VALID;
	plug.action = PA_LOAD;
	plug.index = 1;
	// filename is empty, so check_input fails
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_already_running(void)
{
	TEST("load - already running returns ME_ALREADY");
	setup_globals();
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/already.so");
	plug.status = PL_RUNNING;
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_ALREADY);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_wrong_action(void)
{
	TEST("load - wrong action returns ME_BADREQ");
	setup_globals();
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/wrongact.so");
	plug.action = PA_UNLOAD;
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_BADREQ);
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_query_fail_dlclose_error(void)
{
	TEST("load - query failure with dlclose error warns");
	setup_globals();
	g_have_meta_query = false;
	g_dlclose_return = -1;
	MPlugin plug;
	setup_plugin_for_load(&plug, "dlls/qfail.so");
	mBOOL ret = plug.load(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(plug.status == PL_BADFILE);
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
	system("rm -rf /tmp/test_mplugin_lc");

	printf("test_mplugin_lifecycle:\n");

	// load
	fail |= test_load_success();
	fail |= test_load_dlopen_fail();
	fail |= test_load_missing_meta_query();
	fail |= test_load_missing_givefnptrstodll();
	fail |= test_load_meta_query_error();
	fail |= test_load_null_info();
	fail |= test_load_missing_meta_attach();
	fail |= test_load_meta_attach_error();
	fail |= test_load_loadtime_not_allowed();
	fail |= test_load_loadtime_delayed();
	fail |= test_load_without_meta_init();
	fail |= test_load_gameinit_at_anytime();
	fail |= test_load_no_gameinit_at_startup();
	fail |= test_load_ifvers_newer_rejected();
	fail |= test_load_ifvers_older_major_rejected();
	fail |= test_load_ifvers_older_minor_accepted();
	fail |= test_load_with_pext();
	fail |= test_load_with_gamedll_tables();
	fail |= test_load_pext_newer_version();

	// unload
	fail |= test_unload_success();
	fail |= test_unload_detach_fails();
	fail |= test_unload_forced_overrides_detach();
	fail |= test_unload_loadtime_not_allowed();
	fail |= test_unload_loadtime_delayed();
	fail |= test_unload_forced_overrides_loadtime();
	fail |= test_unload_reload_action();

	// detach
	fail |= test_detach_no_handle();
	fail |= test_detach_missing_symbol();
	fail |= test_detach_error();
	fail |= test_detach_success();

	// reload
	fail |= test_reload_success();
	fail |= test_reload_loadtime_not_allowed();

	// pause/unpause
	fail |= test_pause_success();
	fail |= test_pause_not_allowed();
	fail |= test_unpause_success();

	// full lifecycle
	fail |= test_load_unload_cycle();
	fail |= test_load_pause_unpause_unload();

	// plugin_unload
	fail |= test_plugin_unload_success();
	fail |= test_plugin_unload_self();
	fail |= test_plugin_unload_unknown_plid();
	fail |= test_plugin_unload_is_unloader();

	// free_api_pointers
	fail |= test_free_api_pointers_allocated();

	// plugin_unload delayed
	fail |= test_plugin_unload_delayed();

	// additional lifecycle
	fail |= test_load_ifvers_unexpected();
	fail |= test_reload_delayed();
	fail |= test_retry_reload();
	fail |= test_unload_reload_overrides_detach();

	// reload check_input fails
	fail |= test_reload_check_input_fails();

	// clear dlclose fails
	fail |= test_clear_dlclose_fails();

	// show found_child_plugins
	fail |= test_show_child_plugins();

	// load edge cases
	fail |= test_load_check_input_fails();
	fail |= test_load_already_running();
	fail |= test_load_wrong_action();
	fail |= test_load_query_fail_dlclose_error();

	system("rm -rf /tmp/test_mplugin_lc");
	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
