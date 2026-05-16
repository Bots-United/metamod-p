//
// metamod-p - tests for dllapi.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "dllapi.h"
#include "mlist.h"
#include "mreg.h"
#include "conf_meta.h"

#include "meta_eiface.h"

#include "engine_mock.h"
#include "test_common.h"


static MPluginList test_plugins("plugins.ini");
static MRegCmdList test_reg_cmds;
static MRegCvarList test_reg_cvars;
static MRegMsgList test_reg_msgs;
static MConfig test_config;
static option_t test_options[] = {
	{ NULL, CF_NONE, NULL, NULL }
};

static int g_game_init_called;
static int g_spawn_called;
static int g_think_called;
static int g_server_deactivate_called;
static int g_start_frame_called;

static void mock_game_init(void) { g_game_init_called++; }
static int mock_spawn(edict_t *) { g_spawn_called++; return 0; }
static void mock_think(edict_t *) { g_think_called++; }
static void mock_server_deactivate(void) { g_server_deactivate_called++; }
static void mock_start_frame(void) { g_start_frame_called++; }

static DLL_FUNCTIONS mock_dll_funcs;

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
	metamod_not_loaded = 0;

	memset(&mock_dll_funcs, 0, sizeof(mock_dll_funcs));
	mock_dll_funcs.pfnGameInit = mock_game_init;
	mock_dll_funcs.pfnSpawn = mock_spawn;
	mock_dll_funcs.pfnThink = mock_think;
	mock_dll_funcs.pfnServerDeactivate = mock_server_deactivate;
	mock_dll_funcs.pfnStartFrame = mock_start_frame;
	GameDLL_funcs.dllapi_table = &mock_dll_funcs;

	g_game_init_called = 0;
	g_spawn_called = 0;
	g_think_called = 0;
	g_server_deactivate_called = 0;
	g_start_frame_called = 0;
}

static void teardown_globals(void)
{
	Plugins = NULL;
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
	Config = NULL;
	GameDLL_funcs.dllapi_table = NULL;
}

// ============================================================
// GetEntityAPI / GetEntityAPI2 tests
// ============================================================

static int test_get_entity_api_null(void)
{
	TEST("GetEntityAPI - NULL table returns FALSE");
	setup_globals();
	int ret = GetEntityAPI(NULL, INTERFACE_VERSION);
	ASSERT_TRUE(ret == FALSE);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_entity_api_bad_version(void)
{
	TEST("GetEntityAPI - wrong version returns FALSE");
	setup_globals();
	DLL_FUNCTIONS funcs;
	int ret = GetEntityAPI(&funcs, 999);
	ASSERT_TRUE(ret == FALSE);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_entity_api_success(void)
{
	TEST("GetEntityAPI - valid call succeeds");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ret = GetEntityAPI(&funcs, INTERFACE_VERSION);
	ASSERT_TRUE(ret == TRUE);
	ASSERT_TRUE(funcs.pfnGameInit != NULL);
	ASSERT_TRUE(funcs.pfnSpawn != NULL);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_entity_api2_null(void)
{
	TEST("GetEntityAPI2 - NULL table returns FALSE");
	setup_globals();
	int ver = INTERFACE_VERSION;
	int ret = GetEntityAPI2(NULL, &ver);
	ASSERT_TRUE(ret == FALSE);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_entity_api2_bad_version(void)
{
	TEST("GetEntityAPI2 - wrong version returns FALSE and sets version");
	setup_globals();
	DLL_FUNCTIONS funcs;
	int ver = 999;
	int ret = GetEntityAPI2(&funcs, &ver);
	ASSERT_TRUE(ret == FALSE);
	ASSERT_TRUE(ver == INTERFACE_VERSION);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_entity_api2_success(void)
{
	TEST("GetEntityAPI2 - valid call succeeds");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	int ret = GetEntityAPI2(&funcs, &ver);
	ASSERT_TRUE(ret == TRUE);
	ASSERT_TRUE(funcs.pfnGameInit != NULL);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_entity_api_not_loaded(void)
{
	TEST("GetEntityAPI - metamod_not_loaded returns FALSE");
	setup_globals();
	metamod_not_loaded = 1;
	DLL_FUNCTIONS funcs;
	int ret = GetEntityAPI(&funcs, INTERFACE_VERSION);
	ASSERT_TRUE(ret == FALSE);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// GetNewDLLFunctions tests
// ============================================================

static int test_get_new_dll_functions_null(void)
{
	TEST("GetNewDLLFunctions - NULL table returns FALSE");
	setup_globals();
	int ver = NEW_DLL_FUNCTIONS_VERSION;
	int ret = GetNewDLLFunctions(NULL, &ver);
	ASSERT_TRUE(ret == FALSE);
	teardown_globals();
	PASS();
	return 0;
}

static int test_get_new_dll_functions_bad_version(void)
{
	TEST("GetNewDLLFunctions - wrong version returns FALSE");
	setup_globals();
	NEW_DLL_FUNCTIONS funcs;
	int ver = 999;
	int ret = GetNewDLLFunctions(&funcs, &ver);
	ASSERT_TRUE(ret == FALSE);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// Hook call-through tests (no plugins, calls game DLL directly)
// ============================================================

static int test_hook_game_init(void)
{
	TEST("hook - GameDLLInit calls through to game DLL");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	funcs.pfnGameInit();
	ASSERT_TRUE(g_game_init_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_spawn(void)
{
	TEST("hook - DispatchSpawn calls through to game DLL");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	funcs.pfnSpawn(&ed);
	ASSERT_TRUE(g_spawn_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_think(void)
{
	TEST("hook - DispatchThink calls through to game DLL");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	funcs.pfnThink(&ed);
	ASSERT_TRUE(g_think_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_server_deactivate(void)
{
	TEST("hook - ServerDeactivate calls through to game DLL");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	funcs.pfnServerDeactivate();
	ASSERT_TRUE(g_server_deactivate_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_start_frame(void)
{
	TEST("hook - StartFrame calls through to game DLL");
	setup_globals();
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	funcs.pfnStartFrame();
	ASSERT_TRUE(g_start_frame_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_null_game_func(void)
{
	TEST("hook - NULL game DLL function handled gracefully");
	setup_globals();
	mock_dll_funcs.pfnStartFrame = NULL;
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	funcs.pfnStartFrame();
	ASSERT_TRUE(1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_null_game_table(void)
{
	TEST("hook - NULL game DLL table handled gracefully");
	setup_globals();
	GameDLL_funcs.dllapi_table = NULL;
	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);
	funcs.pfnGameInit();
	ASSERT_TRUE(g_game_init_called == 0);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// Comprehensive hook call-through tests for all DLL functions
// ============================================================

static int g_call_count;
static void stub_void_v(void) { g_call_count++; }
static void stub_void_p(edict_t *) { g_call_count++; }
static void stub_void_2p(edict_t *, edict_t *) { g_call_count++; }
static void stub_void_p_kvd(edict_t *, KeyValueData *) { g_call_count++; }
static void stub_void_p_sr(edict_t *, SAVERESTOREDATA *) { g_call_count++; }
static int stub_int_p_sr_i(edict_t *, SAVERESTOREDATA *, int) { g_call_count++; return 0; }
static void stub_void_sr_s_p_td_i(SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int) { g_call_count++; }
static void stub_void_sr(SAVERESTOREDATA *) { g_call_count++; }
static qboolean stub_connect(edict_t *, const char *, const char *, char [128]) { g_call_count++; return TRUE; }
static void stub_void_p_s(edict_t *, char *) { g_call_count++; }
static void stub_activate(edict_t *, int, int) { g_call_count++; }
static const char *stub_get_desc(void) { g_call_count++; return "test"; }
static void stub_customization(edict_t *, customization_t *) { g_call_count++; }
static void stub_sys_error(const char *) { g_call_count++; }
static void stub_pm_move(struct playermove_s *, int) { g_call_count++; }
static void stub_pm_init(struct playermove_s *) { g_call_count++; }
static char stub_pm_findtex(char *) { g_call_count++; return 'C'; }
static void stub_setup_vis(edict_t *, edict_t *, unsigned char **, unsigned char **) { g_call_count++; }
static void stub_update_cl(const struct edict_s *, int, struct clientdata_s *) { g_call_count++; }
static int stub_addfullpack(struct entity_state_s *, int, edict_t *, edict_t *, int, int, unsigned char *) { g_call_count++; return 0; }
static void stub_create_bl(int, int, struct entity_state_s *, struct edict_s *, int, vec3_t, vec3_t) { g_call_count++; }
static int stub_get_wpn(struct edict_s *, struct weapon_data_s *) { g_call_count++; return 0; }
static void stub_cmdstart(const edict_t *, const struct usercmd_s *, unsigned int) { g_call_count++; }
static void stub_cmdend(const edict_t *) { g_call_count++; }
static int stub_connless(const struct netadr_s *, const char *, char *, int *) { g_call_count++; return 0; }
static int stub_gethull(int, float *, float *) { g_call_count++; return 0; }
static int stub_inconsistent(const edict_t *, const char *, char *) { g_call_count++; return 0; }
static int stub_allow_lag(void) { g_call_count++; return 0; }
static void stub_free_ent(edict_t *) { g_call_count++; }
static int stub_should_collide(edict_t *, edict_t *) { g_call_count++; return 0; }
static void stub_cvar_value(const edict_t *, const char *) { g_call_count++; }
static void stub_cvar_value2(const edict_t *, int, const char *, const char *) { g_call_count++; }

static int test_hook_all_dllapi_void(void)
{
	TEST("hook - all void DLL API functions call through");
	setup_globals();

	mock_dll_funcs.pfnGameInit = stub_void_v;
	mock_dll_funcs.pfnSpawn = mock_spawn;
	mock_dll_funcs.pfnThink = stub_void_p;
	mock_dll_funcs.pfnUse = stub_void_2p;
	mock_dll_funcs.pfnTouch = stub_void_2p;
	mock_dll_funcs.pfnBlocked = stub_void_2p;
	mock_dll_funcs.pfnKeyValue = stub_void_p_kvd;
	mock_dll_funcs.pfnSave = stub_void_p_sr;
	mock_dll_funcs.pfnRestore = stub_int_p_sr_i;
	mock_dll_funcs.pfnSetAbsBox = stub_void_p;
	mock_dll_funcs.pfnSaveWriteFields = stub_void_sr_s_p_td_i;
	mock_dll_funcs.pfnSaveReadFields = stub_void_sr_s_p_td_i;
	mock_dll_funcs.pfnSaveGlobalState = stub_void_sr;
	mock_dll_funcs.pfnRestoreGlobalState = stub_void_sr;
	mock_dll_funcs.pfnResetGlobalState = stub_void_v;
	mock_dll_funcs.pfnClientConnect = stub_connect;
	mock_dll_funcs.pfnClientDisconnect = stub_void_p;
	mock_dll_funcs.pfnClientKill = stub_void_p;
	mock_dll_funcs.pfnClientPutInServer = stub_void_p;
	mock_dll_funcs.pfnClientCommand = stub_void_p;
	mock_dll_funcs.pfnClientUserInfoChanged = stub_void_p_s;
	mock_dll_funcs.pfnServerActivate = stub_activate;
	mock_dll_funcs.pfnServerDeactivate = stub_void_v;
	mock_dll_funcs.pfnPlayerPreThink = stub_void_p;
	mock_dll_funcs.pfnPlayerPostThink = stub_void_p;
	mock_dll_funcs.pfnStartFrame = stub_void_v;
	mock_dll_funcs.pfnParmsNewLevel = stub_void_v;
	mock_dll_funcs.pfnParmsChangeLevel = stub_void_v;
	mock_dll_funcs.pfnGetGameDescription = stub_get_desc;
	mock_dll_funcs.pfnPlayerCustomization = stub_customization;
	mock_dll_funcs.pfnSpectatorConnect = stub_void_p;
	mock_dll_funcs.pfnSpectatorDisconnect = stub_void_p;
	mock_dll_funcs.pfnSpectatorThink = stub_void_p;
	mock_dll_funcs.pfnSys_Error = stub_sys_error;
	mock_dll_funcs.pfnPM_Move = stub_pm_move;
	mock_dll_funcs.pfnPM_Init = stub_pm_init;
	mock_dll_funcs.pfnPM_FindTextureType = stub_pm_findtex;
	mock_dll_funcs.pfnSetupVisibility = stub_setup_vis;
	mock_dll_funcs.pfnUpdateClientData = stub_update_cl;
	mock_dll_funcs.pfnAddToFullPack = stub_addfullpack;
	mock_dll_funcs.pfnCreateBaseline = stub_create_bl;
	mock_dll_funcs.pfnRegisterEncoders = stub_void_v;
	mock_dll_funcs.pfnGetWeaponData = stub_get_wpn;
	mock_dll_funcs.pfnCmdStart = stub_cmdstart;
	mock_dll_funcs.pfnCmdEnd = stub_cmdend;
	mock_dll_funcs.pfnConnectionlessPacket = stub_connless;
	mock_dll_funcs.pfnGetHullBounds = stub_gethull;
	mock_dll_funcs.pfnCreateInstancedBaselines = stub_void_v;
	mock_dll_funcs.pfnInconsistentFile = stub_inconsistent;
	mock_dll_funcs.pfnAllowLagCompensation = stub_allow_lag;

	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	GetEntityAPI2(&funcs, &ver);

	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	SAVERESTOREDATA sr;
	memset(&sr, 0, sizeof(sr));
	KeyValueData kvd;
	memset(&kvd, 0, sizeof(kvd));
	char reason[128] = "";
	float vec[3] = {0};

	g_call_count = 0;
	funcs.pfnUse(&ed, &ed); ASSERT_INT(g_call_count, 1);
	funcs.pfnTouch(&ed, &ed); ASSERT_INT(g_call_count, 2);
	funcs.pfnBlocked(&ed, &ed); ASSERT_INT(g_call_count, 3);
	funcs.pfnKeyValue(&ed, &kvd); ASSERT_INT(g_call_count, 4);
	funcs.pfnSave(&ed, &sr); ASSERT_INT(g_call_count, 5);
	funcs.pfnRestore(&ed, &sr, 0); ASSERT_INT(g_call_count, 6);
	funcs.pfnSetAbsBox(&ed); ASSERT_INT(g_call_count, 7);
	funcs.pfnSaveWriteFields(&sr, "test", NULL, NULL, 0); ASSERT_INT(g_call_count, 8);
	funcs.pfnSaveReadFields(&sr, "test", NULL, NULL, 0); ASSERT_INT(g_call_count, 9);
	funcs.pfnSaveGlobalState(&sr); ASSERT_INT(g_call_count, 10);
	funcs.pfnRestoreGlobalState(&sr); ASSERT_INT(g_call_count, 11);
	funcs.pfnResetGlobalState(); ASSERT_INT(g_call_count, 12);
	funcs.pfnClientConnect(&ed, "name", "addr", reason); ASSERT_INT(g_call_count, 13);
	funcs.pfnClientDisconnect(&ed); ASSERT_INT(g_call_count, 14);
	funcs.pfnClientKill(&ed); ASSERT_INT(g_call_count, 15);
	funcs.pfnClientPutInServer(&ed); ASSERT_INT(g_call_count, 16);
	funcs.pfnClientCommand(&ed); ASSERT_INT(g_call_count, 17);
	funcs.pfnClientUserInfoChanged(&ed, (char *)""); ASSERT_INT(g_call_count, 18);
	funcs.pfnServerActivate(&ed, 1, 32); ASSERT_INT(g_call_count, 19);
	funcs.pfnPlayerPreThink(&ed); ASSERT_INT(g_call_count, 20);
	funcs.pfnPlayerPostThink(&ed); ASSERT_INT(g_call_count, 21);
	funcs.pfnParmsNewLevel(); ASSERT_INT(g_call_count, 22);
	funcs.pfnParmsChangeLevel(); ASSERT_INT(g_call_count, 23);
	funcs.pfnGetGameDescription(); ASSERT_INT(g_call_count, 24);
	funcs.pfnPlayerCustomization(&ed, NULL); ASSERT_INT(g_call_count, 25);
	funcs.pfnSpectatorConnect(&ed); ASSERT_INT(g_call_count, 26);
	funcs.pfnSpectatorDisconnect(&ed); ASSERT_INT(g_call_count, 27);
	funcs.pfnSpectatorThink(&ed); ASSERT_INT(g_call_count, 28);
	funcs.pfnSys_Error("test"); ASSERT_INT(g_call_count, 29);
	funcs.pfnPM_Move(NULL, 0); ASSERT_INT(g_call_count, 30);
	funcs.pfnPM_Init(NULL); ASSERT_INT(g_call_count, 31);
	funcs.pfnPM_FindTextureType((char *)"test"); ASSERT_INT(g_call_count, 32);
	funcs.pfnSetupVisibility(&ed, &ed, NULL, NULL); ASSERT_INT(g_call_count, 33);
	funcs.pfnUpdateClientData(&ed, 0, NULL); ASSERT_INT(g_call_count, 34);
	funcs.pfnAddToFullPack(NULL, 0, &ed, &ed, 0, 0, NULL); ASSERT_INT(g_call_count, 35);
	funcs.pfnCreateBaseline(0, 0, NULL, &ed, 0, vec, vec); ASSERT_INT(g_call_count, 36);
	funcs.pfnRegisterEncoders(); ASSERT_INT(g_call_count, 37);
	funcs.pfnGetWeaponData(&ed, NULL); ASSERT_INT(g_call_count, 38);
	funcs.pfnCmdStart(&ed, NULL, 0); ASSERT_INT(g_call_count, 39);
	funcs.pfnCmdEnd(&ed); ASSERT_INT(g_call_count, 40);
	funcs.pfnConnectionlessPacket(NULL, "", NULL, NULL); ASSERT_INT(g_call_count, 41);
	funcs.pfnGetHullBounds(0, NULL, NULL); ASSERT_INT(g_call_count, 42);
	funcs.pfnCreateInstancedBaselines(); ASSERT_INT(g_call_count, 43);
	funcs.pfnInconsistentFile(&ed, "test", NULL); ASSERT_INT(g_call_count, 44);
	funcs.pfnAllowLagCompensation(); ASSERT_INT(g_call_count, 45);

	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_new_dll_functions(void)
{
	TEST("hook - NEW_DLL_FUNCTIONS call through to game DLL");
	setup_globals();
	meta_new_dll_functions_t::test_set_version(3);

	NEW_DLL_FUNCTIONS mock_new_funcs;
	memset(&mock_new_funcs, 0, sizeof(mock_new_funcs));
	mock_new_funcs.pfnOnFreeEntPrivateData = stub_free_ent;
	mock_new_funcs.pfnGameShutdown = stub_void_v;
	mock_new_funcs.pfnShouldCollide = stub_should_collide;
	mock_new_funcs.pfnCvarValue = stub_cvar_value;
	mock_new_funcs.pfnCvarValue2 = stub_cvar_value2;
	GameDLL_funcs.newapi_table = &mock_new_funcs;

	NEW_DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = NEW_DLL_FUNCTIONS_VERSION;
	int ret = GetNewDLLFunctions(&funcs, &ver);
	ASSERT_TRUE(ret == TRUE);

	edict_t ed;
	memset(&ed, 0, sizeof(ed));

	g_call_count = 0;
	funcs.pfnOnFreeEntPrivateData(&ed); ASSERT_INT(g_call_count, 1);
	funcs.pfnGameShutdown(); ASSERT_INT(g_call_count, 2);
	funcs.pfnShouldCollide(&ed, &ed); ASSERT_INT(g_call_count, 3);
	funcs.pfnCvarValue(&ed, "val"); ASSERT_INT(g_call_count, 4);
	funcs.pfnCvarValue2(&ed, 1, "cv", "val"); ASSERT_INT(g_call_count, 5);

	GameDLL_funcs.newapi_table = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_client_command_meta(void)
{
	TEST("hook - mm_ClientCommand dispatches 'meta' to client_meta");
	setup_globals();
	test_config.clientmeta = 1;

	mock_dll_funcs.pfnClientCommand = stub_void_p;
	GameDLL_funcs.dllapi_table = &mock_dll_funcs;

	DLL_FUNCTIONS funcs;
	memset(&funcs, 0, sizeof(funcs));
	int ver = INTERFACE_VERSION;
	int ret = GetEntityAPI2(&funcs, &ver);
	ASSERT_TRUE(ret == TRUE);

	const char *argv[] = {"meta", "version"};
	mock_set_cmd_args(2, argv, "meta version");

	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	g_call_count = 0;
	funcs.pfnClientCommand(&ed);
	ASSERT_INT(g_call_count, 1);
	ASSERT_TRUE(mock_get_client_print_count() > 0);

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

	printf("test_dllapi:\n");

	fail |= test_get_entity_api_null();
	fail |= test_get_entity_api_bad_version();
	fail |= test_get_entity_api_success();
	fail |= test_get_entity_api2_null();
	fail |= test_get_entity_api2_bad_version();
	fail |= test_get_entity_api2_success();
	fail |= test_get_entity_api_not_loaded();
	fail |= test_get_new_dll_functions_null();
	fail |= test_get_new_dll_functions_bad_version();
	fail |= test_hook_game_init();
	fail |= test_hook_spawn();
	fail |= test_hook_think();
	fail |= test_hook_server_deactivate();
	fail |= test_hook_start_frame();
	fail |= test_hook_null_game_func();
	fail |= test_hook_null_game_table();
	fail |= test_hook_all_dllapi_void();
	fail |= test_hook_new_dll_functions();
	fail |= test_hook_client_command_meta();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
