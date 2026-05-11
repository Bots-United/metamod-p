//
// metamod-p - tests for mutil.cpp
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#include <extdll.h>

#include "metamod.h"
#include "mutil.h"
#include "mreg.h"
#include "mlist.h"

#include "engine_mock.h"
#include "test_common.h"

extern mutil_funcs_t MetaUtilFunctions;

static plugin_info_t test_plinfo = {
	META_INTERFACE_VERSION,
	"TestPlugin",
	"1.0",
	"2024/01/01",
	"Test",
	"http://test",
	"TEST",
	PT_ANYTIME,
	PT_ANYTIME
};

static plid_t test_plid = &test_plinfo;

// ============================================================
// LogConsole tests
// ============================================================

static int test_log_console(void)
{
	TEST("mutil_LogConsole - logs to server print");
	mock_reset();
	MetaUtilFunctions.pfnLogConsole(test_plid, "hello %s", "world");
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "hello world");
	PASS();
	return 0;
}

static int test_log_console_long(void)
{
	TEST("mutil_LogConsole - long message gets newline replaced");
	mock_reset();
	char longmsg[4096];
	memset(longmsg, 'A', sizeof(longmsg) - 1);
	longmsg[sizeof(longmsg) - 1] = '\0';
	MetaUtilFunctions.pfnLogConsole(test_plid, "%s", longmsg);
	const char *out = mock_get_server_print_msg(0);
	ASSERT_TRUE(strlen(out) > 0);
	PASS();
	return 0;
}

// ============================================================
// LogMessage / LogError tests
// ============================================================

static int test_log_message(void)
{
	TEST("mutil_LogMessage - logs with plugin tag");
	mock_reset();
	MetaUtilFunctions.pfnLogMessage(test_plid, "test message %d", 42);
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "TEST");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "test message 42");
	PASS();
	return 0;
}

static int test_log_error(void)
{
	TEST("mutil_LogError - logs with ERROR prefix");
	mock_reset();
	MetaUtilFunctions.pfnLogError(test_plid, "bad thing");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "ERROR");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "bad thing");
	PASS();
	return 0;
}

// ============================================================
// LogDeveloper tests
// ============================================================

static int test_log_developer_off(void)
{
	TEST("mutil_LogDeveloper - developer=0 skips logging");
	mock_reset();
	mock_set_cvar_float("developer", 0.0f);
	MetaUtilFunctions.pfnLogDeveloper(test_plid, "should not appear");
	ASSERT_INT(mock_get_alert_count(), 0);
	PASS();
	return 0;
}

static int test_log_developer_on(void)
{
	TEST("mutil_LogDeveloper - developer=1 logs message");
	mock_reset();
	mock_set_cvar_float("developer", 1.0f);
	MetaUtilFunctions.pfnLogDeveloper(test_plid, "dev info");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "dev: dev info");
	PASS();
	return 0;
}

// ============================================================
// CenterSay tests
// ============================================================

static int test_center_say(void)
{
	TEST("mutil_CenterSay - broadcasts to all clients");
	mock_reset();
	gpGlobals->maxClients = 2;
	MetaUtilFunctions.pfnCenterSay(test_plid, "center %s", "msg");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "center msg");
	PASS();
	return 0;
}

static int test_center_say_parms(void)
{
	TEST("mutil_CenterSayParms - with custom parms");
	mock_reset();
	gpGlobals->maxClients = 1;
	hudtextparms_t parms;
	memset(&parms, 0, sizeof(parms));
	parms.x = 0.5f;
	parms.y = 0.5f;
	MetaUtilFunctions.pfnCenterSayParms(test_plid, parms, "parms test");
	ASSERT_STR_CONTAINS(mock_get_alert_msg(0), "parms test");
	PASS();
	return 0;
}

// ============================================================
// GetUserMsgID / GetUserMsgName tests
// ============================================================

static MRegMsgList test_reg_msgs;

static int test_get_user_msg_id_found(void)
{
	TEST("mutil_GetUserMsgID - finds registered message");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	test_reg_msgs.add("TestMsg", 64, 16);
	int size = 0;
	int id = MetaUtilFunctions.pfnGetUserMsgID(test_plid, "TestMsg", &size);
	ASSERT_INT(id, 64);
	ASSERT_INT(size, 16);
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_id_not_found(void)
{
	TEST("mutil_GetUserMsgID - unknown message returns 0");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	int id = MetaUtilFunctions.pfnGetUserMsgID(test_plid, "NoSuchMsg", NULL);
	ASSERT_INT(id, 0);
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_found(void)
{
	TEST("mutil_GetUserMsgName - finds registered message by id");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	test_reg_msgs.add("AnotherMsg", 65, 8);
	int size = 0;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 65, &size);
	ASSERT_PTR_NOT_NULL(name);
	ASSERT_STR(name, "AnotherMsg");
	ASSERT_INT(size, 8);
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_not_found(void)
{
	TEST("mutil_GetUserMsgName - unknown id returns NULL");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 999, NULL);
	ASSERT_PTR_NULL(name);
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_builtin_tempentity(void)
{
	TEST("mutil_GetUserMsgName - SVC_TEMPENTITY returns guess");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	int size = 0;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 23, &size);
	ASSERT_PTR_NOT_NULL(name);
	ASSERT_STR(name, "tempentity?");
	ASSERT_INT(size, -1);
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_builtin_intermission(void)
{
	TEST("mutil_GetUserMsgName - SVC_INTERMISSION returns guess");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 30, NULL);
	ASSERT_STR(name, "intermission?");
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_builtin_cdtrack(void)
{
	TEST("mutil_GetUserMsgName - SVC_CDTRACK returns guess");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 32, NULL);
	ASSERT_STR(name, "cdtrack?");
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_builtin_weaponanim(void)
{
	TEST("mutil_GetUserMsgName - SVC_WEAPONANIM returns guess");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 35, NULL);
	ASSERT_STR(name, "weaponanim?");
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_builtin_roomtype(void)
{
	TEST("mutil_GetUserMsgName - SVC_ROOMTYPE returns guess");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 37, NULL);
	ASSERT_STR(name, "roomtype?");
	RegMsgs = NULL;
	PASS();
	return 0;
}

static int test_get_user_msg_name_builtin_director(void)
{
	TEST("mutil_GetUserMsgName - SVC_DIRECTOR returns guess");
	mock_reset();
	RegMsgs = &test_reg_msgs;
	const char *name = MetaUtilFunctions.pfnGetUserMsgName(test_plid, 51, NULL);
	ASSERT_STR(name, "director?");
	RegMsgs = NULL;
	PASS();
	return 0;
}

// ============================================================
// GetGameInfo tests
// ============================================================

static int test_get_game_info_name(void)
{
	TEST("mutil_GetGameInfo - GINFO_NAME returns game name");
	mock_reset();
	STRNCPY(GameDLL.name, "cstrike", sizeof(GameDLL.name));
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, GINFO_NAME);
	ASSERT_STR(result, "cstrike");
	PASS();
	return 0;
}

static int test_get_game_info_desc(void)
{
	TEST("mutil_GetGameInfo - GINFO_DESC returns description");
	mock_reset();
	GameDLL.desc = "Counter-Strike";
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, GINFO_DESC);
	ASSERT_STR(result, "Counter-Strike");
	PASS();
	return 0;
}

static int test_get_game_info_gamedir(void)
{
	TEST("mutil_GetGameInfo - GINFO_GAMEDIR returns gamedir");
	mock_reset();
	STRNCPY(GameDLL.gamedir, "/opt/hlds/cstrike", sizeof(GameDLL.gamedir));
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, GINFO_GAMEDIR);
	ASSERT_STR(result, "/opt/hlds/cstrike");
	PASS();
	return 0;
}

static int test_get_game_info_dll_fullpath(void)
{
	TEST("mutil_GetGameInfo - GINFO_DLL_FULLPATH returns pathname");
	mock_reset();
	STRNCPY(GameDLL.pathname, "/opt/hlds/cstrike/dlls/cs.so", sizeof(GameDLL.pathname));
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, GINFO_DLL_FULLPATH);
	ASSERT_STR(result, "/opt/hlds/cstrike/dlls/cs.so");
	PASS();
	return 0;
}

static int test_get_game_info_dll_filename(void)
{
	TEST("mutil_GetGameInfo - GINFO_DLL_FILENAME returns file");
	mock_reset();
	GameDLL.file = "cs.so";
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, GINFO_DLL_FILENAME);
	ASSERT_STR(result, "cs.so");
	PASS();
	return 0;
}

static int test_get_game_info_realdll(void)
{
	TEST("mutil_GetGameInfo - GINFO_REALDLL_FULLPATH returns real_pathname");
	mock_reset();
	STRNCPY(GameDLL.real_pathname, "/opt/hlds/cstrike/dlls/cs_real.so", sizeof(GameDLL.real_pathname));
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, GINFO_REALDLL_FULLPATH);
	ASSERT_STR(result, "/opt/hlds/cstrike/dlls/cs_real.so");
	PASS();
	return 0;
}

static int test_get_game_info_invalid(void)
{
	TEST("mutil_GetGameInfo - invalid type returns NULL");
	mock_reset();
	const char *result = MetaUtilFunctions.pfnGetGameInfo(test_plid, (ginfo_t)999);
	ASSERT_PTR_NULL(result);
	PASS();
	return 0;
}

// ============================================================
// LoadMetaPlugin / UnloadMetaPlugin tests
// ============================================================

static MPluginList test_plugin_list("plugins.ini");

static int test_load_plugin_null_fname(void)
{
	TEST("mutil_LoadMetaPlugin - NULL fname returns ME_ARGUMENT");
	mock_reset();
	Plugins = &test_plugin_list;
	int ret = MetaUtilFunctions.pfnLoadPlugin(test_plid, NULL, PT_ANYTIME, NULL);
	ASSERT_INT(ret, ME_ARGUMENT);
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_unload_plugin_null_fname(void)
{
	TEST("mutil_UnloadMetaPlugin - NULL fname returns ME_ARGUMENT");
	mock_reset();
	Plugins = &test_plugin_list;
	int ret = MetaUtilFunctions.pfnUnloadPlugin(test_plid, NULL, PT_ANYTIME, PNL_NULL);
	ASSERT_INT(ret, ME_ARGUMENT);
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_unload_plugin_by_handle_null(void)
{
	TEST("mutil_UnloadMetaPluginByHandle - NULL handle returns ME_ARGUMENT");
	mock_reset();
	Plugins = &test_plugin_list;
	int ret = MetaUtilFunctions.pfnUnloadPluginByHandle(test_plid, NULL, PT_ANYTIME, PNL_NULL);
	ASSERT_INT(ret, ME_ARGUMENT);
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_unload_plugin_by_handle_not_found(void)
{
	TEST("mutil_UnloadMetaPluginByHandle - unknown handle returns ME_NOTFOUND");
	mock_reset();
	Plugins = &test_plugin_list;
	int ret = MetaUtilFunctions.pfnUnloadPluginByHandle(test_plid, (void *)0xDEAD, PT_ANYTIME, PNL_NULL);
	ASSERT_INT(ret, ME_NOTFOUND);
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_unload_plugin_not_found(void)
{
	TEST("mutil_UnloadMetaPlugin - unknown plugin name");
	mock_reset();
	Plugins = &test_plugin_list;
	int ret = MetaUtilFunctions.pfnUnloadPlugin(test_plid, "nonexistent_plugin", PT_ANYTIME, PNL_NULL);
	ASSERT_TRUE(ret != 0);
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_unload_plugin_by_index_not_found(void)
{
	TEST("mutil_UnloadMetaPlugin - unknown plugin index");
	mock_reset();
	Plugins = &test_plugin_list;
	int ret = MetaUtilFunctions.pfnUnloadPlugin(test_plid, "1", PT_ANYTIME, PNL_NULL);
	ASSERT_TRUE(ret != 0);
	Plugins = NULL;
	PASS();
	return 0;
}

// ============================================================
// IsQueryingClientCvar / MakeRequestID / GetHookTables tests
// ============================================================

static int test_is_querying_client_cvar(void)
{
	TEST("mutil_IsQueryingClientCvar - returns NULL for unknown player");
	mock_reset();
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	const char *result = MetaUtilFunctions.pfnIsQueryingClientCvar(test_plid, &ed);
	ASSERT_PTR_NULL(result);
	PASS();
	return 0;
}

static int test_make_request_id(void)
{
	TEST("mutil_MakeRequestID - returns unique IDs");
	mock_reset();
	int id1 = MetaUtilFunctions.pfnMakeRequestID(test_plid);
	int id2 = MetaUtilFunctions.pfnMakeRequestID(test_plid);
	ASSERT_TRUE(id1 != id2);
	ASSERT_TRUE(id1 > 0);
	ASSERT_TRUE(id2 > 0);
	PASS();
	return 0;
}

static int test_get_hook_tables(void)
{
	TEST("mutil_GetHookTables - returns pointers to hook tables");
	mock_reset();
	enginefuncs_t *peng = NULL;
	DLL_FUNCTIONS *pdll = NULL;
	NEW_DLL_FUNCTIONS *pnewdll = NULL;
	MetaUtilFunctions.pfnGetHookTables(test_plid, &peng, &pdll, &pnewdll);
	ASSERT_PTR_NOT_NULL(peng);
	PASS();
	return 0;
}

static int test_get_hook_tables_null_args(void)
{
	TEST("mutil_GetHookTables - handles NULL output pointers");
	mock_reset();
	MetaUtilFunctions.pfnGetHookTables(test_plid, NULL, NULL, NULL);
	PASS();
	return 0;
}

// ============================================================
// GetPluginPath tests
// ============================================================

static int test_get_plugin_path_not_found(void)
{
	TEST("mutil_GetPluginPath - unknown plugin returns NULL");
	mock_reset();
	Plugins = &test_plugin_list;
	const char *path = MetaUtilFunctions.pfnGetPluginPath(test_plid);
	ASSERT_PTR_NULL(path);
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_get_plugin_path_found(void)
{
	TEST("mutil_GetPluginPath - found plugin returns pathname");
	mock_reset();
	Plugins = &test_plugin_list;
	MPlugin *pl = &test_plugin_list.plist[0];
	memset(pl, 0, sizeof(*pl));
	pl->index = 1;
	pl->status = PL_RUNNING;
	pl->info = &test_plinfo;
	STRNCPY(pl->pathname, "/tmp/dlls/test_i386.so", sizeof(pl->pathname));
	test_plugin_list.endlist = 1;
	const char *path = MetaUtilFunctions.pfnGetPluginPath(test_plid);
	ASSERT_PTR_NOT_NULL(path);
	ASSERT_TRUE(strcmp(path, "/tmp/dlls/test_i386.so") == 0);
	memset(pl, 0, sizeof(*pl));
	test_plugin_list.endlist = 0;
	Plugins = NULL;
	PASS();
	return 0;
}

// ============================================================
// CallGameEntity tests
// ============================================================

static int test_call_game_entity_not_found(void)
{
	TEST("mutil_CallGameEntity - unknown entity returns false");
	mock_reset();
	GameDLL.handle = NULL;
	entvars_t ev;
	memset(&ev, 0, sizeof(ev));
	qboolean ret = MetaUtilFunctions.pfnCallGameEntity(test_plid, "nonexistent_entity_xyz", &ev);
	ASSERT_INT(ret, false);
	PASS();
	return 0;
}

static int test_call_game_entity_found(void)
{
	TEST("mutil_CallGameEntity - found entity returns true");
	mock_reset();
	void *h = dlopen("./fake_gamedll.so", RTLD_NOW);
	if (!h) { printf("SKIP (no fake_gamedll.so)\n"); tests_run--; return 0; }
	GameDLL.handle = (DLHANDLE)h;
	STRNCPY(GameDLL.name, "fake_gamedll.so", sizeof(GameDLL.name));
	entvars_t ev;
	memset(&ev, 0, sizeof(ev));
	qboolean ret = MetaUtilFunctions.pfnCallGameEntity(test_plid, "GiveFnptrsToDll", &ev);
	ASSERT_INT(ret, true);
	dlclose(h);
	GameDLL.handle = NULL;
	PASS();
	return 0;
}

// ============================================================
// MetaUtilFunctions table validation
// ============================================================

static int test_meta_util_functions_table(void)
{
	TEST("MetaUtilFunctions - all function pointers non-NULL");
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnLogConsole);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnLogMessage);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnLogError);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnLogDeveloper);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnCenterSay);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnCenterSayParms);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnCenterSayVarargs);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnCallGameEntity);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnGetUserMsgID);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnGetUserMsgName);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnGetPluginPath);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnGetGameInfo);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnLoadPlugin);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnUnloadPlugin);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnUnloadPluginByHandle);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnIsQueryingClientCvar);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnMakeRequestID);
	ASSERT_PTR_NOT_NULL(MetaUtilFunctions.pfnGetHookTables);
	PASS();
	return 0;
}

// ============================================================
// LoadMetaPlugin / UnloadMetaPlugin with real plugin
// ============================================================

static MRegCmdList test_reg_cmds_mu;
static MRegCvarList test_reg_cvars_mu;
static MRegMsgList test_reg_msgs_mu;

static void setup_mu_globals(void)
{
	mock_reset();
	RegCmds = &test_reg_cmds_mu;
	RegCvars = &test_reg_cvars_mu;
	RegMsgs = &test_reg_msgs_mu;
	STRNCPY(GameDLL.gamedir, "/tmp/test_mutil_gd", sizeof(GameDLL.gamedir));
}

static void teardown_mu_globals(void)
{
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
	memset(&GameDLL, 0, sizeof(GameDLL));
	Plugins = NULL;
}

struct HeapPluginListMU {
	MPluginList *ptr;
	HeapPluginListMU(const char *ini) : ptr(new MPluginList(ini)) {}
	~HeapPluginListMU() { delete ptr; }
};

static int test_load_plugin_success(void)
{
	TEST("mutil_LoadMetaPlugin - successful load returns 0");
	setup_mu_globals();
	system("mkdir -p /tmp/test_mutil_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mutil_gd/dlls/mu_load.so");

	HeapPluginListMU list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info;
	memset(&ldr_info, 0, sizeof(ldr_info));
	ldr_info.name = "MULoader";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "mu_loader.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "MULoader", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/mu_loader.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = Plugins->add(&t_loader);
	loader->info = &ldr_info;

	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	void *handle = NULL;
	int ret = MetaUtilFunctions.pfnLoadPlugin((plid_t)&ldr_info, "dlls/mu_load.so", PT_ANYTIME, &handle);
	ASSERT_INT(ret, 0);
	ASSERT_PTR_NOT_NULL(handle);

	// Clean up loaded plugin
	MPlugin *loaded = Plugins->find((DLHANDLE)handle);
	if (loaded) {
		loaded->free_api_pointers();
		if (loaded->handle) { DLCLOSE(loaded->handle); loaded->handle = NULL; }
		loaded->status = PL_EMPTY;
	}

	unlink("/tmp/test_mutil_gd/dlls/mu_load.so");
	teardown_mu_globals();
	PASS();
	return 0;
}

static int test_load_plugin_fail(void)
{
	TEST("mutil_LoadMetaPlugin - failed load sets handle to NULL");
	setup_mu_globals();
	system("mkdir -p /tmp/test_mutil_gd/dlls");

	HeapPluginListMU list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info;
	memset(&ldr_info, 0, sizeof(ldr_info));
	ldr_info.name = "MULoader2";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "mu_loader2.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "MULoader2", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/mu_loader2.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = Plugins->add(&t_loader);
	loader->info = &ldr_info;

	void *handle = (void *)0xBEEF;
	int ret = MetaUtilFunctions.pfnLoadPlugin((plid_t)&ldr_info, "dlls/nonexistent_mu.so", PT_ANYTIME, &handle);
	ASSERT_TRUE(ret != 0);
	ASSERT_PTR_NULL(handle);

	teardown_mu_globals();
	PASS();
	return 0;
}

static int test_unload_plugin_by_name(void)
{
	TEST("mutil_UnloadMetaPlugin - unload by index succeeds");
	setup_mu_globals();
	system("mkdir -p /tmp/test_mutil_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mutil_gd/dlls/mu_unload.so");

	HeapPluginListMU list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info;
	memset(&ldr_info, 0, sizeof(ldr_info));
	ldr_info.name = "MUUnloader";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "mu_loader3.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "MUUnloader", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/mu_loader3.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = Plugins->add(&t_loader);
	loader->info = &ldr_info;

	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	void *handle = NULL;
	int ret = MetaUtilFunctions.pfnLoadPlugin((plid_t)&ldr_info, "dlls/mu_unload.so", PT_ANYTIME, &handle);
	ASSERT_INT(ret, 0);

	// Unload by index — the loaded plugin is at index 2 (loader at 1)
	ret = MetaUtilFunctions.pfnUnloadPlugin((plid_t)&ldr_info, "2", PT_ANYTIME, PNL_PLUGIN);
	ASSERT_INT(ret, 0);

	unlink("/tmp/test_mutil_gd/dlls/mu_unload.so");
	teardown_mu_globals();
	PASS();
	return 0;
}

static int test_unload_plugin_by_handle_found(void)
{
	TEST("mutil_UnloadMetaPluginByHandle - unload by handle succeeds");
	setup_mu_globals();
	system("mkdir -p /tmp/test_mutil_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mutil_gd/dlls/mu_byh.so");

	HeapPluginListMU list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info;
	memset(&ldr_info, 0, sizeof(ldr_info));
	ldr_info.name = "MUHandleUnloader";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "mu_loader4.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "MUHandleUnloader", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/mu_loader4.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = Plugins->add(&t_loader);
	loader->info = &ldr_info;

	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	void *handle = NULL;
	int ret = MetaUtilFunctions.pfnLoadPlugin((plid_t)&ldr_info, "dlls/mu_byh.so", PT_ANYTIME, &handle);
	ASSERT_INT(ret, 0);
	ASSERT_PTR_NOT_NULL(handle);

	ret = MetaUtilFunctions.pfnUnloadPluginByHandle((plid_t)&ldr_info, handle, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_INT(ret, 0);

	unlink("/tmp/test_mutil_gd/dlls/mu_byh.so");
	teardown_mu_globals();
	PASS();
	return 0;
}

static int test_unload_plugin_fail_not_allowed(void)
{
	TEST("mutil_UnloadMetaPlugin - not unloadable returns meta_errno");
	setup_mu_globals();
	system("mkdir -p /tmp/test_mutil_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mutil_gd/dlls/mu_nounl.so");

	HeapPluginListMU list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info;
	memset(&ldr_info, 0, sizeof(ldr_info));
	ldr_info.name = "MUNoUnloader";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "mu_loader5.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "MUNoUnloader", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/mu_loader5.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = Plugins->add(&t_loader);
	loader->info = &ldr_info;

	setenv("FAKE_MM_UNLOADABLE", "1", 1);
	unsetenv("FAKE_MM_ATTACH_FAIL");

	void *handle = NULL;
	int ret = MetaUtilFunctions.pfnLoadPlugin((plid_t)&ldr_info, "dlls/mu_nounl.so", PT_ANYTIME, &handle);
	ASSERT_INT(ret, 0);

	ret = MetaUtilFunctions.pfnUnloadPlugin((plid_t)&ldr_info, "2", PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret != 0);

	// Clean up: plugin is still loaded since unload was refused
	MPlugin *loaded = Plugins->find(2);
	if (loaded) {
		loaded->free_api_pointers();
		if (loaded->handle) { DLCLOSE(loaded->handle); loaded->handle = NULL; }
		loaded->status = PL_EMPTY;
	}

	unsetenv("FAKE_MM_UNLOADABLE");
	unlink("/tmp/test_mutil_gd/dlls/mu_nounl.so");
	teardown_mu_globals();
	PASS();
	return 0;
}

static int test_unload_plugin_by_handle_fail_not_allowed(void)
{
	TEST("mutil_UnloadMetaPluginByHandle - not unloadable returns meta_errno");
	setup_mu_globals();
	system("mkdir -p /tmp/test_mutil_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mutil_gd/dlls/mu_nounlh.so");

	HeapPluginListMU list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info;
	memset(&ldr_info, 0, sizeof(ldr_info));
	ldr_info.name = "MUNoUnloaderH";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "mu_loader6.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "MUNoUnloaderH", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/mu_loader6.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = Plugins->add(&t_loader);
	loader->info = &ldr_info;

	setenv("FAKE_MM_UNLOADABLE", "1", 1);
	unsetenv("FAKE_MM_ATTACH_FAIL");

	void *handle = NULL;
	int ret = MetaUtilFunctions.pfnLoadPlugin((plid_t)&ldr_info, "dlls/mu_nounlh.so", PT_ANYTIME, &handle);
	ASSERT_INT(ret, 0);
	ASSERT_PTR_NOT_NULL(handle);

	ret = MetaUtilFunctions.pfnUnloadPluginByHandle((plid_t)&ldr_info, handle, PT_ANYTIME, PNL_PLUGIN);
	ASSERT_TRUE(ret != 0);

	// Clean up: plugin is still loaded since unload was refused
	MPlugin *loaded = Plugins->find((DLHANDLE)handle);
	if (loaded) {
		loaded->free_api_pointers();
		if (loaded->handle) { DLCLOSE(loaded->handle); loaded->handle = NULL; }
		loaded->status = PL_EMPTY;
	}

	unsetenv("FAKE_MM_UNLOADABLE");
	unlink("/tmp/test_mutil_gd/dlls/mu_nounlh.so");
	teardown_mu_globals();
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
	system("rm -rf /tmp/test_mutil_gd");

	printf("test_mutil:\n");

	fail |= test_log_console();
	fail |= test_log_console_long();
	fail |= test_log_message();
	fail |= test_log_error();
	fail |= test_log_developer_off();
	fail |= test_log_developer_on();
	fail |= test_center_say();
	fail |= test_center_say_parms();

	fail |= test_get_user_msg_id_found();
	fail |= test_get_user_msg_id_not_found();
	fail |= test_get_user_msg_name_found();
	fail |= test_get_user_msg_name_not_found();
	fail |= test_get_user_msg_name_builtin_tempentity();
	fail |= test_get_user_msg_name_builtin_intermission();
	fail |= test_get_user_msg_name_builtin_cdtrack();
	fail |= test_get_user_msg_name_builtin_weaponanim();
	fail |= test_get_user_msg_name_builtin_roomtype();
	fail |= test_get_user_msg_name_builtin_director();

	fail |= test_get_game_info_name();
	fail |= test_get_game_info_desc();
	fail |= test_get_game_info_gamedir();
	fail |= test_get_game_info_dll_fullpath();
	fail |= test_get_game_info_dll_filename();
	fail |= test_get_game_info_realdll();
	fail |= test_get_game_info_invalid();

	fail |= test_load_plugin_null_fname();
	fail |= test_unload_plugin_null_fname();
	fail |= test_unload_plugin_by_handle_null();
	fail |= test_unload_plugin_by_handle_not_found();
	fail |= test_unload_plugin_not_found();
	fail |= test_unload_plugin_by_index_not_found();

	fail |= test_is_querying_client_cvar();
	fail |= test_make_request_id();
	fail |= test_get_hook_tables();
	fail |= test_get_hook_tables_null_args();
	fail |= test_get_plugin_path_not_found();
	fail |= test_get_plugin_path_found();
	fail |= test_call_game_entity_not_found();
	fail |= test_call_game_entity_found();
	fail |= test_meta_util_functions_table();

	fail |= test_load_plugin_success();
	fail |= test_load_plugin_fail();
	fail |= test_unload_plugin_by_name();
	fail |= test_unload_plugin_by_handle_found();
	fail |= test_unload_plugin_fail_not_allowed();
	fail |= test_unload_plugin_by_handle_fail_not_allowed();

	system("rm -rf /tmp/test_mutil_gd");
	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
