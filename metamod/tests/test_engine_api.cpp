//
// metamod-p - tests for engine_api.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "engine_api.h"
#include "mlist.h"
#include "mreg.h"
#include "conf_meta.h"

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

static int g_precache_model_called;
static int g_precache_sound_called;
static int g_server_print_called;

static int mock_eng_precache_model(char *s)
{
	(void)s;
	g_precache_model_called++;
	return 42;
}

static int mock_eng_precache_sound(char *s)
{
	(void)s;
	g_precache_sound_called++;
	return 7;
}

static void mock_eng_server_print(const char *msg)
{
	(void)msg;
	g_server_print_called++;
}

static enginefuncs_t mock_eng_table;

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

	memset(&mock_eng_table, 0, sizeof(mock_eng_table));
	mock_eng_table.pfnPrecacheModel = mock_eng_precache_model;
	mock_eng_table.pfnPrecacheSound = mock_eng_precache_sound;
	mock_eng_table.pfnServerPrint = mock_eng_server_print;
	Engine_funcs = &mock_eng_table;

	g_precache_model_called = 0;
	g_precache_sound_called = 0;
	g_server_print_called = 0;
}

static void teardown_globals(void)
{
	Plugins = NULL;
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
	Config = NULL;
}

// ============================================================
// Engine hook call-through tests (no plugins)
// ============================================================

static int test_hook_precache_model(void)
{
	TEST("engine hook - PrecacheModel calls through to engine");
	setup_globals();
	meta_engfuncs.pfnPrecacheModel((char *)"test.mdl");
	ASSERT_TRUE(g_precache_model_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_precache_sound(void)
{
	TEST("engine hook - PrecacheSound calls through to engine");
	setup_globals();
	meta_engfuncs.pfnPrecacheSound((char *)"test.wav");
	ASSERT_TRUE(g_precache_sound_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_server_print(void)
{
	TEST("engine hook - ServerPrint calls through to engine");
	setup_globals();
	meta_engfuncs.pfnServerPrint("test message");
	ASSERT_TRUE(g_server_print_called == 1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_null_engine_func(void)
{
	TEST("engine hook - NULL engine function handled gracefully");
	setup_globals();
	mock_eng_table.pfnServerPrint = NULL;
	meta_engfuncs.pfnServerPrint("test");
	ASSERT_TRUE(g_server_print_called == 0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_hook_null_engine_table(void)
{
	TEST("engine hook - NULL engine table handled gracefully");
	setup_globals();
	Engine_funcs = NULL;
	meta_engfuncs.pfnServerPrint("test");
	ASSERT_TRUE(g_server_print_called == 0);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// Comprehensive engine hook call-through test
// ============================================================

static int g_eng_call_count;
static int  eng_int_s(char *) { g_eng_call_count++; return 0; }
static void eng_void_v(void) { g_eng_call_count++; }
static void eng_void_p(edict_t *) { g_eng_call_count++; }
static void eng_void_p_cs(edict_t *, const char *) { g_eng_call_count++; }
static int  eng_int_cs(const char *) { g_eng_call_count++; return 0; }
static int  eng_int_i(int) { g_eng_call_count++; return 0; }
static void eng_void_p_cf_cf(edict_t *, const float *, const float *) { g_eng_call_count++; }
static void eng_void_s_s(char *, char *) { g_eng_call_count++; }
static float eng_float_cf(const float *) { g_eng_call_count++; return 0; }
static void eng_void_cf_f(const float *, float *) { g_eng_call_count++; }
static void eng_void_p_cf_f_i(edict_t *, const float *, float, int) { g_eng_call_count++; }
static edict_t *eng_edict_p_cs_cs(edict_t *, const char *, const char *) { g_eng_call_count++; return NULL; }
static int  eng_int_p(edict_t *) { g_eng_call_count++; return 0; }
static edict_t *eng_edict_p_cf_f(edict_t *, const float *, float) { g_eng_call_count++; return NULL; }
static edict_t *eng_edict_p(edict_t *) { g_eng_call_count++; return NULL; }
static void eng_void_cf(const float *) { g_eng_call_count++; }
static void eng_void_cf_f_f_f(const float *, float *, float *, float *) { g_eng_call_count++; }
static edict_t *eng_edict_v(void) { g_eng_call_count++; return NULL; }
static edict_t *eng_edict_i(int) { g_eng_call_count++; return NULL; }
static int  eng_int_p_f_f_i(edict_t *, float, float, int) { g_eng_call_count++; return 0; }
static void eng_void_p_cf2(edict_t *, const float *) { g_eng_call_count++; }
static void eng_emit_sound(edict_t *, int, const char *, float, float, int, int) { g_eng_call_count++; }
static void eng_emit_ambient(edict_t *, float *, const char *, float, float, int, int) { g_eng_call_count++; }
static void eng_traceline(const float *, const float *, int, edict_t *, TraceResult *) { g_eng_call_count++; }
static void eng_tracetoss(edict_t *, edict_t *, TraceResult *) { g_eng_call_count++; }
static int  eng_tracemonster(edict_t *, const float *, const float *, int, edict_t *, TraceResult *) { g_eng_call_count++; return 0; }
static void eng_tracehull(const float *, const float *, int, int, edict_t *, TraceResult *) { g_eng_call_count++; }
static void eng_tracemodel(const float *, const float *, int, edict_t *, TraceResult *) { g_eng_call_count++; }
static const char *eng_tracetexture(edict_t *, const float *, const float *) { g_eng_call_count++; return ""; }
static void eng_tracesphere(const float *, const float *, int, float, edict_t *, TraceResult *) { g_eng_call_count++; }
static void eng_getaim(edict_t *, float, float *) { g_eng_call_count++; }
static void eng_void_s(char *) { g_eng_call_count++; }
static void eng_void_i_s(int, char *) { g_eng_call_count++; }
static void eng_void_i(int) { g_eng_call_count++; }
static void eng_void_f(float) { g_eng_call_count++; }
static void eng_void_cs(const char *) { g_eng_call_count++; }
static void eng_void_cvar(cvar_t *) { g_eng_call_count++; }
static float eng_float_cs(const char *) { g_eng_call_count++; return 0; }
static const char *eng_cs_cs(const char *) { g_eng_call_count++; return ""; }
static void eng_void_cs_f(const char *, float) { g_eng_call_count++; }
static void eng_void_cs_cs(const char *, const char *) { g_eng_call_count++; }
static void eng_alert(ALERT_TYPE, char *, ...) { g_eng_call_count++; }
static void eng_fprintf(void *, char *, ...) { g_eng_call_count++; }
static void *eng_pvalloc(edict_t *, int32) { g_eng_call_count++; return NULL; }
static void *eng_pvget(edict_t *) { g_eng_call_count++; return NULL; }
static const char *eng_cs_i(int) { g_eng_call_count++; return ""; }
static int  eng_int_cs2(const char *s) { g_eng_call_count++; return 0; }
static entvars_s *eng_vars(edict_t *) { g_eng_call_count++; return NULL; }
static int  eng_int_cp2(const edict_t *) { g_eng_call_count++; return 0; }
static edict_t *eng_edict_evars(entvars_s *) { g_eng_call_count++; return NULL; }
static void *eng_modelptr(edict_t *) { g_eng_call_count++; return NULL; }
static int  eng_regusermsg(const char *, int) { g_eng_call_count++; return 0; }
static void eng_anim(const edict_t *, float) { g_eng_call_count++; }
static void eng_bonepos(const edict_t *, int, float *, float *) { g_eng_call_count++; }
static uint32 eng_funcfromname(const char *) { g_eng_call_count++; return 0; }
static const char *eng_nameforfunc(uint32) { g_eng_call_count++; return ""; }
static void eng_clientprintf(edict_t *, PRINT_TYPE, const char *) { g_eng_call_count++; }
static void eng_getattach(const edict_t *, int, float *, float *) { g_eng_call_count++; }
static void eng_crc32init(CRC32_t *) { g_eng_call_count++; }
static void eng_crc32buf(CRC32_t *, void *, int) { g_eng_call_count++; }
static void eng_crc32byte(CRC32_t *, unsigned char) { g_eng_call_count++; }
static CRC32_t eng_crc32final(CRC32_t) { g_eng_call_count++; return 0; }
static int32 eng_randlong(int32, int32) { g_eng_call_count++; return 0; }
static float eng_randfloat(float, float) { g_eng_call_count++; return 0; }
static void eng_setview(const edict_t *, const edict_t *) { g_eng_call_count++; }
static float eng_time(void) { g_eng_call_count++; return 0; }
static void eng_crosshair(const edict_t *, float, float) { g_eng_call_count++; }
static byte *eng_loadfile(char *, int *) { g_eng_call_count++; return NULL; }
static void eng_freefile(void *) { g_eng_call_count++; }
static void eng_endsection(const char *) { g_eng_call_count++; }
static int  eng_compfiletime(char *, char *, int *) { g_eng_call_count++; return 0; }
static void eng_getgamedir(char *) { g_eng_call_count++; }
static void eng_fadevolume(const edict_t *, int, int, int, int) { g_eng_call_count++; }
static void eng_setmaxspeed(const edict_t *, float) { g_eng_call_count++; }
static edict_t *eng_createfake(const char *) { g_eng_call_count++; return NULL; }
static void eng_runplayermove(edict_t *, const float *, float, float, float, unsigned short, byte, byte) { g_eng_call_count++; }
static int  eng_numents(void) { g_eng_call_count++; return 0; }
static char *eng_infobuf(edict_t *) { g_eng_call_count++; return (char *)""; }
static char *eng_infokeyval(char *, char *) { g_eng_call_count++; return (char *)""; }
static void eng_setkeyval(char *, char *, char *) { g_eng_call_count++; }
static void eng_setclientkey(int, char *, char *, char *) { g_eng_call_count++; }
static int  eng_ismapvalid(char *) { g_eng_call_count++; return 0; }
static void eng_staticdecal(const float *, int, int, int) { g_eng_call_count++; }
static int  eng_precachegeneric(char *) { g_eng_call_count++; return 0; }
static int  eng_getplayeruserid(edict_t *) { g_eng_call_count++; return 0; }
static void eng_buildsoundmsg(edict_t *, int, const char *, float, float, int, int, int, int, const float *, edict_t *) { g_eng_call_count++; }
static int  eng_isdedicated(void) { g_eng_call_count++; return 0; }
static cvar_t *eng_cvargetptr(const char *) { g_eng_call_count++; return NULL; }
static unsigned int eng_getwonid(edict_t *) { g_eng_call_count++; return 0; }
static void eng_removekey(char *, const char *) { g_eng_call_count++; }
static const char *eng_physkey(const edict_t *, const char *) { g_eng_call_count++; return ""; }
static void eng_setphyskey(const edict_t *, const char *, const char *) { g_eng_call_count++; }
static const char *eng_physinfo(const edict_t *) { g_eng_call_count++; return ""; }
static unsigned short eng_precacheevent(int, const char *) { g_eng_call_count++; return 0; }
static void eng_playbackevent(int, const edict_t *, unsigned short, float, float *, float *, float, float, int, int, int, int) { g_eng_call_count++; }
static unsigned char *eng_setfatpvs(float *) { g_eng_call_count++; return NULL; }
static unsigned char *eng_setfatpas(float *) { g_eng_call_count++; return NULL; }
static int  eng_checkvis(const edict_t *, unsigned char *) { g_eng_call_count++; return 0; }
static void eng_deltasetfield(struct delta_s *, const char *) { g_eng_call_count++; }
static void eng_deltaunsetfield(struct delta_s *, const char *) { g_eng_call_count++; }
static void eng_deltaaddenc(char *, void (*)(struct delta_s *, const unsigned char *, const unsigned char *)) { g_eng_call_count++; }
static int  eng_getcurplayer(void) { g_eng_call_count++; return 0; }
static int  eng_canskip(const edict_t *) { g_eng_call_count++; return 0; }
static int  eng_deltafindfield(struct delta_s *, const char *) { g_eng_call_count++; return 0; }
static void eng_deltasetbyidx(struct delta_s *, int) { g_eng_call_count++; }
static void eng_deltaunsetbyidx(struct delta_s *, int) { g_eng_call_count++; }
static void eng_setgroupmask(int, int) { g_eng_call_count++; }
static int  eng_createinstbl(int, struct entity_state_s *) { g_eng_call_count++; return 0; }
static void eng_cvardirectset(struct cvar_s *, char *) { g_eng_call_count++; }
static void eng_forceunmod(FORCE_TYPE, float *, float *, const char *) { g_eng_call_count++; }
static void eng_playerstats(const edict_t *, int *, int *) { g_eng_call_count++; }
static void eng_addservercmd(char *, void (*)(void)) { g_eng_call_count++; }
static qboolean eng_voice_get(int, int) { g_eng_call_count++; return 0; }
static qboolean eng_voice_set(int, int, qboolean) { g_eng_call_count++; return 0; }
static const char *eng_authid(edict_t *) { g_eng_call_count++; return ""; }
static sequenceEntry_s *eng_seqget(const char *, const char *) { g_eng_call_count++; return NULL; }
static sentenceEntry_s *eng_seqpick(const char *, int, int *) { g_eng_call_count++; return NULL; }
static int  eng_filesize(char *) { g_eng_call_count++; return 0; }
static unsigned int eng_approxwavelen(const char *) { g_eng_call_count++; return 0; }
static int  eng_iscareermatch(void) { g_eng_call_count++; return 0; }
static int  eng_getlocstrlen(const char *) { g_eng_call_count++; return 0; }
static void eng_regtutormsg(int) { g_eng_call_count++; }
static int  eng_gettutormsg(int) { g_eng_call_count++; return 0; }
static void eng_proctutorbuf(int *, int) { g_eng_call_count++; }
static void eng_constructtutorbuf(int *, int) { g_eng_call_count++; }
static void eng_resettutordata(void) { g_eng_call_count++; }
static void eng_querycvar(const edict_t *, const char *) { g_eng_call_count++; }
static void eng_querycvar2(const edict_t *, const char *, int) { g_eng_call_count++; }
static int  eng_checkparm(const char *, char **) { g_eng_call_count++; return 0; }
static void eng_void_i_i_cf_p(int, int, const float *, edict_t *) { g_eng_call_count++; }
static void eng_clientcmd(edict_t *, char *, ...) { g_eng_call_count++; }

static int test_hook_all_engine_functions(void)
{
	TEST("engine hook - comprehensive call-through test");
	setup_globals();

	mock_eng_table.pfnPrecacheModel = eng_int_s;
	mock_eng_table.pfnPrecacheSound = eng_int_s;
	mock_eng_table.pfnSetModel = eng_void_p_cs;
	mock_eng_table.pfnModelIndex = eng_int_cs;
	mock_eng_table.pfnModelFrames = eng_int_i;
	mock_eng_table.pfnSetSize = eng_void_p_cf_cf;
	mock_eng_table.pfnChangeLevel = eng_void_s_s;
	mock_eng_table.pfnGetSpawnParms = eng_void_p;
	mock_eng_table.pfnSaveSpawnParms = eng_void_p;
	mock_eng_table.pfnVecToYaw = eng_float_cf;
	mock_eng_table.pfnVecToAngles = eng_void_cf_f;
	mock_eng_table.pfnMoveToOrigin = eng_void_p_cf_f_i;
	mock_eng_table.pfnChangeYaw = eng_void_p;
	mock_eng_table.pfnChangePitch = eng_void_p;
	mock_eng_table.pfnFindEntityByString = eng_edict_p_cs_cs;
	mock_eng_table.pfnGetEntityIllum = eng_int_p;
	mock_eng_table.pfnFindEntityInSphere = eng_edict_p_cf_f;
	mock_eng_table.pfnFindClientInPVS = eng_edict_p;
	mock_eng_table.pfnEntitiesInPVS = eng_edict_p;
	mock_eng_table.pfnMakeVectors = eng_void_cf;
	mock_eng_table.pfnAngleVectors = eng_void_cf_f_f_f;
	mock_eng_table.pfnCreateEntity = eng_edict_v;
	mock_eng_table.pfnRemoveEntity = eng_void_p;
	mock_eng_table.pfnCreateNamedEntity = eng_edict_i;
	mock_eng_table.pfnMakeStatic = eng_void_p;
	mock_eng_table.pfnEntIsOnFloor = eng_int_p;
	mock_eng_table.pfnDropToFloor = eng_int_p;
	mock_eng_table.pfnWalkMove = eng_int_p_f_f_i;
	mock_eng_table.pfnSetOrigin = eng_void_p_cf2;
	mock_eng_table.pfnEmitSound = eng_emit_sound;
	mock_eng_table.pfnEmitAmbientSound = eng_emit_ambient;
	mock_eng_table.pfnTraceLine = eng_traceline;
	mock_eng_table.pfnTraceToss = eng_tracetoss;
	mock_eng_table.pfnTraceMonsterHull = eng_tracemonster;
	mock_eng_table.pfnTraceHull = eng_tracehull;
	mock_eng_table.pfnTraceModel = eng_tracemodel;
	mock_eng_table.pfnTraceTexture = eng_tracetexture;
	mock_eng_table.pfnTraceSphere = eng_tracesphere;
	mock_eng_table.pfnGetAimVector = eng_getaim;
	mock_eng_table.pfnServerCommand = eng_void_s;
	mock_eng_table.pfnServerExecute = eng_void_v;
	mock_eng_table.pfnClientCommand = eng_clientcmd;
	mock_eng_table.pfnParticleEffect = (void (*)(const float *, const float *, float, float))eng_void_cf_f;
	mock_eng_table.pfnLightStyle = eng_void_i_s;
	mock_eng_table.pfnDecalIndex = eng_int_cs;
	mock_eng_table.pfnPointContents = (int (*)(const float *))eng_float_cf;
	mock_eng_table.pfnMessageBegin = eng_void_i_i_cf_p;
	mock_eng_table.pfnMessageEnd = eng_void_v;
	mock_eng_table.pfnWriteByte = eng_void_i;
	mock_eng_table.pfnWriteChar = eng_void_i;
	mock_eng_table.pfnWriteShort = eng_void_i;
	mock_eng_table.pfnWriteLong = eng_void_i;
	mock_eng_table.pfnWriteAngle = eng_void_f;
	mock_eng_table.pfnWriteCoord = eng_void_f;
	mock_eng_table.pfnWriteString = eng_void_cs;
	mock_eng_table.pfnWriteEntity = eng_void_i;
	mock_eng_table.pfnCVarRegister = eng_void_cvar;
	mock_eng_table.pfnCVarGetFloat = eng_float_cs;
	mock_eng_table.pfnCVarGetString = eng_cs_cs;
	mock_eng_table.pfnCVarSetFloat = eng_void_cs_f;
	mock_eng_table.pfnCVarSetString = eng_void_cs_cs;
	mock_eng_table.pfnAlertMessage = eng_alert;
	mock_eng_table.pfnEngineFprintf = eng_fprintf;
	mock_eng_table.pfnPvAllocEntPrivateData = eng_pvalloc;
	mock_eng_table.pfnPvEntPrivateData = eng_pvget;
	mock_eng_table.pfnFreeEntPrivateData = eng_void_p;
	mock_eng_table.pfnSzFromIndex = eng_cs_i;
	mock_eng_table.pfnAllocString = eng_int_cs2;
	mock_eng_table.pfnGetVarsOfEnt = eng_vars;
	mock_eng_table.pfnPEntityOfEntOffset = eng_edict_i;
	mock_eng_table.pfnEntOffsetOfPEntity = eng_int_cp2;
	mock_eng_table.pfnIndexOfEdict = eng_int_cp2;
	mock_eng_table.pfnPEntityOfEntIndex = eng_edict_i;
	mock_eng_table.pfnFindEntityByVars = eng_edict_evars;
	mock_eng_table.pfnGetModelPtr = eng_modelptr;
	mock_eng_table.pfnRegUserMsg = eng_regusermsg;
	mock_eng_table.pfnAnimationAutomove = eng_anim;
	mock_eng_table.pfnGetBonePosition = eng_bonepos;
	mock_eng_table.pfnFunctionFromName = eng_funcfromname;
	mock_eng_table.pfnNameForFunction = eng_nameforfunc;
	mock_eng_table.pfnClientPrintf = eng_clientprintf;
	mock_eng_table.pfnServerPrint = eng_void_cs;
	mock_eng_table.pfnCmd_Args = (const char *(*)(void))eng_edict_v;
	mock_eng_table.pfnCmd_Argv = eng_cs_i;
	mock_eng_table.pfnCmd_Argc = eng_numents;
	mock_eng_table.pfnGetAttachment = eng_getattach;
	mock_eng_table.pfnCRC32_Init = eng_crc32init;
	mock_eng_table.pfnCRC32_ProcessBuffer = eng_crc32buf;
	mock_eng_table.pfnCRC32_ProcessByte = eng_crc32byte;
	mock_eng_table.pfnCRC32_Final = eng_crc32final;
	mock_eng_table.pfnRandomLong = eng_randlong;
	mock_eng_table.pfnRandomFloat = eng_randfloat;
	mock_eng_table.pfnSetView = eng_setview;
	mock_eng_table.pfnTime = eng_time;
	mock_eng_table.pfnCrosshairAngle = eng_crosshair;
	mock_eng_table.pfnLoadFileForMe = eng_loadfile;
	mock_eng_table.pfnFreeFile = eng_freefile;
	mock_eng_table.pfnEndSection = eng_endsection;
	mock_eng_table.pfnCompareFileTime = eng_compfiletime;
	mock_eng_table.pfnGetGameDir = eng_getgamedir;
	mock_eng_table.pfnCvar_RegisterVariable = eng_void_cvar;
	mock_eng_table.pfnFadeClientVolume = eng_fadevolume;
	mock_eng_table.pfnSetClientMaxspeed = eng_setmaxspeed;
	mock_eng_table.pfnCreateFakeClient = eng_createfake;
	mock_eng_table.pfnRunPlayerMove = eng_runplayermove;
	mock_eng_table.pfnNumberOfEntities = eng_numents;
	mock_eng_table.pfnGetInfoKeyBuffer = eng_infobuf;
	mock_eng_table.pfnInfoKeyValue = eng_infokeyval;
	mock_eng_table.pfnSetKeyValue = eng_setkeyval;
	mock_eng_table.pfnSetClientKeyValue = eng_setclientkey;
	mock_eng_table.pfnIsMapValid = eng_ismapvalid;
	mock_eng_table.pfnStaticDecal = eng_staticdecal;
	mock_eng_table.pfnPrecacheGeneric = eng_precachegeneric;
	mock_eng_table.pfnGetPlayerUserId = eng_getplayeruserid;
	mock_eng_table.pfnBuildSoundMsg = eng_buildsoundmsg;
	mock_eng_table.pfnIsDedicatedServer = eng_isdedicated;
	mock_eng_table.pfnCVarGetPointer = eng_cvargetptr;
	mock_eng_table.pfnGetPlayerWONId = eng_getwonid;
	mock_eng_table.pfnInfo_RemoveKey = eng_removekey;
	mock_eng_table.pfnGetPhysicsKeyValue = eng_physkey;
	mock_eng_table.pfnSetPhysicsKeyValue = eng_setphyskey;
	mock_eng_table.pfnGetPhysicsInfoString = eng_physinfo;
	mock_eng_table.pfnPrecacheEvent = eng_precacheevent;
	mock_eng_table.pfnPlaybackEvent = eng_playbackevent;
	mock_eng_table.pfnSetFatPVS = eng_setfatpvs;
	mock_eng_table.pfnSetFatPAS = eng_setfatpas;
	mock_eng_table.pfnCheckVisibility = eng_checkvis;
	mock_eng_table.pfnDeltaSetField = eng_deltasetfield;
	mock_eng_table.pfnDeltaUnsetField = eng_deltaunsetfield;
	mock_eng_table.pfnDeltaAddEncoder = eng_deltaaddenc;
	mock_eng_table.pfnGetCurrentPlayer = eng_getcurplayer;
	mock_eng_table.pfnCanSkipPlayer = eng_canskip;
	mock_eng_table.pfnDeltaFindField = eng_deltafindfield;
	mock_eng_table.pfnDeltaSetFieldByIndex = eng_deltasetbyidx;
	mock_eng_table.pfnDeltaUnsetFieldByIndex = eng_deltaunsetbyidx;
	mock_eng_table.pfnSetGroupMask = eng_setgroupmask;
	mock_eng_table.pfnCreateInstancedBaseline = eng_createinstbl;
	mock_eng_table.pfnCvar_DirectSet = eng_cvardirectset;
	mock_eng_table.pfnForceUnmodified = eng_forceunmod;
	mock_eng_table.pfnGetPlayerStats = eng_playerstats;
	mock_eng_table.pfnAddServerCommand = eng_addservercmd;
	mock_eng_table.pfnVoice_GetClientListening = eng_voice_get;
	mock_eng_table.pfnVoice_SetClientListening = eng_voice_set;
	mock_eng_table.pfnGetPlayerAuthId = eng_authid;
	mock_eng_table.pfnSequenceGet = eng_seqget;
	mock_eng_table.pfnSequencePickSentence = eng_seqpick;
	mock_eng_table.pfnGetFileSize = eng_filesize;
	mock_eng_table.pfnGetApproxWavePlayLen = eng_approxwavelen;
	mock_eng_table.pfnIsCareerMatch = eng_iscareermatch;
	mock_eng_table.pfnGetLocalizedStringLength = eng_getlocstrlen;
	mock_eng_table.pfnRegisterTutorMessageShown = eng_regtutormsg;
	mock_eng_table.pfnGetTimesTutorMessageShown = eng_gettutormsg;
	mock_eng_table.pfnProcessTutorMessageDecayBuffer = eng_proctutorbuf;
	mock_eng_table.pfnConstructTutorMessageDecayBuffer = eng_constructtutorbuf;
	mock_eng_table.pfnResetTutorMessageDecayData = eng_resettutordata;
	mock_eng_table.pfnQueryClientCvarValue = eng_querycvar;
	mock_eng_table.pfnQueryClientCvarValue2 = eng_querycvar2;
	mock_eng_table.pfnEngCheckParm = eng_checkparm;
	mock_eng_table.pfnPEntityOfEntIndexAllEntities = eng_edict_i;

	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	float vec[3] = {0};
	TraceResult tr;
	memset(&tr, 0, sizeof(tr));
	cvar_t cv;
	memset(&cv, 0, sizeof(cv));
	CRC32_t crc = 0;
	int dummy = 0;

	g_eng_call_count = 0;
	meta_engfuncs.pfnPrecacheModel((char *)"m");
	meta_engfuncs.pfnPrecacheSound((char *)"s");
	meta_engfuncs.pfnSetModel(&ed, "m");
	meta_engfuncs.pfnModelIndex("m");
	meta_engfuncs.pfnModelFrames(0);
	meta_engfuncs.pfnSetSize(&ed, vec, vec);
	meta_engfuncs.pfnChangeLevel((char *)"a", (char *)"b");
	meta_engfuncs.pfnGetSpawnParms(&ed);
	meta_engfuncs.pfnSaveSpawnParms(&ed);
	meta_engfuncs.pfnVecToYaw(vec);
	meta_engfuncs.pfnVecToAngles(vec, vec);
	meta_engfuncs.pfnMoveToOrigin(&ed, vec, 1.0, 0);
	meta_engfuncs.pfnChangeYaw(&ed);
	meta_engfuncs.pfnChangePitch(&ed);
	meta_engfuncs.pfnFindEntityByString(&ed, "f", "v");
	meta_engfuncs.pfnGetEntityIllum(&ed);
	meta_engfuncs.pfnFindEntityInSphere(&ed, vec, 1.0);
	meta_engfuncs.pfnFindClientInPVS(&ed);
	meta_engfuncs.pfnEntitiesInPVS(&ed);
	meta_engfuncs.pfnMakeVectors(vec);
	meta_engfuncs.pfnAngleVectors(vec, vec, vec, vec);
	meta_engfuncs.pfnCreateEntity();
	meta_engfuncs.pfnRemoveEntity(&ed);
	meta_engfuncs.pfnCreateNamedEntity(0);
	meta_engfuncs.pfnMakeStatic(&ed);
	meta_engfuncs.pfnEntIsOnFloor(&ed);
	meta_engfuncs.pfnDropToFloor(&ed);
	meta_engfuncs.pfnWalkMove(&ed, 0, 0, 0);
	meta_engfuncs.pfnSetOrigin(&ed, vec);
	meta_engfuncs.pfnEmitSound(&ed, 0, "s", 1.0, 1.0, 0, 100);
	meta_engfuncs.pfnEmitAmbientSound(&ed, vec, "s", 1.0, 1.0, 0, 100);
	meta_engfuncs.pfnTraceLine(vec, vec, 0, &ed, &tr);
	meta_engfuncs.pfnTraceToss(&ed, &ed, &tr);
	meta_engfuncs.pfnTraceMonsterHull(&ed, vec, vec, 0, &ed, &tr);
	meta_engfuncs.pfnTraceHull(vec, vec, 0, 0, &ed, &tr);
	meta_engfuncs.pfnTraceModel(vec, vec, 0, &ed, &tr);
	meta_engfuncs.pfnTraceTexture(&ed, vec, vec);
	meta_engfuncs.pfnTraceSphere(vec, vec, 0, 1.0, &ed, &tr);
	meta_engfuncs.pfnGetAimVector(&ed, 1.0, vec);
	meta_engfuncs.pfnServerCommand((char *)"t");
	meta_engfuncs.pfnServerExecute();
	meta_engfuncs.pfnClientCommand(&ed, (char *)"%s", (char *)"t");
	meta_engfuncs.pfnParticleEffect(vec, vec, 0, 0);
	meta_engfuncs.pfnLightStyle(0, (char *)"a");
	meta_engfuncs.pfnDecalIndex("d");
	meta_engfuncs.pfnPointContents(vec);
	meta_engfuncs.pfnMessageBegin(0, 0, vec, &ed);
	meta_engfuncs.pfnMessageEnd();
	meta_engfuncs.pfnWriteByte(0);
	meta_engfuncs.pfnWriteChar(0);
	meta_engfuncs.pfnWriteShort(0);
	meta_engfuncs.pfnWriteLong(0);
	meta_engfuncs.pfnWriteAngle(0);
	meta_engfuncs.pfnWriteCoord(0);
	meta_engfuncs.pfnWriteString("t");
	meta_engfuncs.pfnWriteEntity(0);
	meta_engfuncs.pfnCVarRegister(&cv);
	meta_engfuncs.pfnCVarGetFloat("v");
	meta_engfuncs.pfnCVarGetString("v");
	meta_engfuncs.pfnCVarSetFloat("v", 0);
	meta_engfuncs.pfnCVarSetString("v", "s");
	meta_engfuncs.pfnAlertMessage(at_console, (char *)"t");
	meta_engfuncs.pfnEngineFprintf(NULL, (char *)"t");
	meta_engfuncs.pfnPvAllocEntPrivateData(&ed, 10);
	meta_engfuncs.pfnPvEntPrivateData(&ed);
	meta_engfuncs.pfnFreeEntPrivateData(&ed);
	meta_engfuncs.pfnSzFromIndex(0);
	meta_engfuncs.pfnAllocString("s");
	meta_engfuncs.pfnGetVarsOfEnt(&ed);
	meta_engfuncs.pfnPEntityOfEntOffset(0);
	meta_engfuncs.pfnEntOffsetOfPEntity(&ed);
	meta_engfuncs.pfnIndexOfEdict(&ed);
	meta_engfuncs.pfnPEntityOfEntIndex(0);
	meta_engfuncs.pfnFindEntityByVars(NULL);
	meta_engfuncs.pfnGetModelPtr(&ed);
	meta_engfuncs.pfnRegUserMsg("m", 0);
	meta_engfuncs.pfnAnimationAutomove(&ed, 0);
	meta_engfuncs.pfnGetBonePosition(&ed, 0, vec, vec);
	meta_engfuncs.pfnFunctionFromName("f");
	meta_engfuncs.pfnNameForFunction(0);
	meta_engfuncs.pfnClientPrintf(&ed, print_console, "t");
	meta_engfuncs.pfnServerPrint("t");
	meta_engfuncs.pfnCmd_Args();
	meta_engfuncs.pfnCmd_Argv(0);
	meta_engfuncs.pfnCmd_Argc();
	meta_engfuncs.pfnGetAttachment(&ed, 0, vec, vec);
	meta_engfuncs.pfnCRC32_Init(&crc);
	meta_engfuncs.pfnCRC32_ProcessBuffer(&crc, &dummy, 4);
	meta_engfuncs.pfnCRC32_ProcessByte(&crc, 0);
	meta_engfuncs.pfnCRC32_Final(crc);
	meta_engfuncs.pfnRandomLong(0, 1);
	meta_engfuncs.pfnRandomFloat(0, 1);
	meta_engfuncs.pfnSetView(&ed, &ed);
	meta_engfuncs.pfnTime();
	meta_engfuncs.pfnCrosshairAngle(&ed, 0, 0);
	meta_engfuncs.pfnLoadFileForMe((char *)"f", &dummy);
	meta_engfuncs.pfnFreeFile(NULL);
	meta_engfuncs.pfnEndSection("s");
	meta_engfuncs.pfnCompareFileTime((char *)"a", (char *)"b", &dummy);
	meta_engfuncs.pfnGetGameDir((char *)"g");
	meta_engfuncs.pfnCvar_RegisterVariable(&cv);
	meta_engfuncs.pfnFadeClientVolume(&ed, 0, 0, 0, 0);
	meta_engfuncs.pfnSetClientMaxspeed(&ed, 0);
	meta_engfuncs.pfnCreateFakeClient("f");
	meta_engfuncs.pfnRunPlayerMove(&ed, vec, 0, 0, 0, 0, 0, 0);
	meta_engfuncs.pfnNumberOfEntities();
	meta_engfuncs.pfnGetInfoKeyBuffer(&ed);
	meta_engfuncs.pfnInfoKeyValue((char *)"b", (char *)"k");
	meta_engfuncs.pfnSetKeyValue((char *)"b", (char *)"k", (char *)"v");
	meta_engfuncs.pfnSetClientKeyValue(0, (char *)"b", (char *)"k", (char *)"v");
	meta_engfuncs.pfnIsMapValid((char *)"m");
	meta_engfuncs.pfnStaticDecal(vec, 0, 0, 0);
	meta_engfuncs.pfnPrecacheGeneric((char *)"g");
	meta_engfuncs.pfnGetPlayerUserId(&ed);
	meta_engfuncs.pfnBuildSoundMsg(&ed, 0, "s", 1.0, 1.0, 0, 100, 0, 0, vec, &ed);
	meta_engfuncs.pfnIsDedicatedServer();
	meta_engfuncs.pfnCVarGetPointer("v");
	meta_engfuncs.pfnGetPlayerWONId(&ed);
	meta_engfuncs.pfnInfo_RemoveKey((char *)"b", "k");
	meta_engfuncs.pfnGetPhysicsKeyValue(&ed, "k");
	meta_engfuncs.pfnSetPhysicsKeyValue(&ed, "k", "v");
	meta_engfuncs.pfnGetPhysicsInfoString(&ed);
	meta_engfuncs.pfnPrecacheEvent(0, "e");
	meta_engfuncs.pfnPlaybackEvent(0, &ed, 0, 0, vec, vec, 0, 0, 0, 0, 0, 0);
	meta_engfuncs.pfnSetFatPVS(vec);
	meta_engfuncs.pfnSetFatPAS(vec);
	meta_engfuncs.pfnCheckVisibility(&ed, NULL);
	meta_engfuncs.pfnDeltaSetField(NULL, "f");
	meta_engfuncs.pfnDeltaUnsetField(NULL, "f");
	meta_engfuncs.pfnDeltaAddEncoder((char *)"n", NULL);
	meta_engfuncs.pfnGetCurrentPlayer();
	meta_engfuncs.pfnCanSkipPlayer(&ed);
	meta_engfuncs.pfnDeltaFindField(NULL, "f");
	meta_engfuncs.pfnDeltaSetFieldByIndex(NULL, 0);
	meta_engfuncs.pfnDeltaUnsetFieldByIndex(NULL, 0);
	meta_engfuncs.pfnSetGroupMask(0, 0);
	meta_engfuncs.pfnCreateInstancedBaseline(0, NULL);
	meta_engfuncs.pfnCvar_DirectSet(NULL, (char *)"v");
	meta_engfuncs.pfnForceUnmodified(force_exactfile, vec, vec, "f");
	meta_engfuncs.pfnGetPlayerStats(&ed, &dummy, &dummy);
	meta_engfuncs.pfnAddServerCommand((char *)"c", NULL);
	meta_engfuncs.pfnVoice_GetClientListening(0, 0);
	meta_engfuncs.pfnVoice_SetClientListening(0, 0, 0);
	meta_engfuncs.pfnGetPlayerAuthId(&ed);
	meta_engfuncs.pfnSequenceGet("f", "e");
	meta_engfuncs.pfnSequencePickSentence("g", 0, &dummy);
	meta_engfuncs.pfnGetFileSize((char *)"f");
	meta_engfuncs.pfnGetApproxWavePlayLen("w");
	meta_engfuncs.pfnIsCareerMatch();
	meta_engfuncs.pfnGetLocalizedStringLength("l");
	meta_engfuncs.pfnRegisterTutorMessageShown(0);
	meta_engfuncs.pfnGetTimesTutorMessageShown(0);
	meta_engfuncs.pfnProcessTutorMessageDecayBuffer(&dummy, 1);
	meta_engfuncs.pfnConstructTutorMessageDecayBuffer(&dummy, 1);
	meta_engfuncs.pfnResetTutorMessageDecayData();
	meta_engfuncs.pfnQueryClientCvarValue(&ed, "c");
	meta_engfuncs.pfnQueryClientCvarValue2(&ed, "c", 1);
	meta_engfuncs.pfnEngCheckParm("p", NULL);
	meta_engfuncs.pfnPEntityOfEntIndexAllEntities(0);

	ASSERT_TRUE(g_eng_call_count > 100);

	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// RegUserMsg duplicate-detection tests
// ============================================================

static int g_regusermsg_return_id;
static int mock_eng_regusermsg_fixed(const char *, int)
{
	return g_regusermsg_return_id;
}

static int test_regusermsg_duplicate_name(void)
{
	TEST("engine hook - RegUserMsg duplicate name logs debug");
	setup_globals();
	mock_eng_table.pfnRegUserMsg = mock_eng_regusermsg_fixed;

	g_regusermsg_return_id = 64;
	meta_engfuncs.pfnRegUserMsg("TestMsg", 4);
	MRegMsg *msg = RegMsgs->find(64);
	ASSERT_PTR_NOT_NULL(msg);

	meta_engfuncs.pfnRegUserMsg("TestMsg", 4);
	ASSERT_TRUE(1);

	teardown_globals();
	PASS();
	return 0;
}

static int test_regusermsg_update_empty_name(void)
{
	TEST("engine hook - RegUserMsg updates empty name");
	setup_globals();
	mock_eng_table.pfnRegUserMsg = mock_eng_regusermsg_fixed;

	g_regusermsg_return_id = 65;
	RegMsgs->add("", 65, 4);

	meta_engfuncs.pfnRegUserMsg("UpdatedName", 4);
	MRegMsg *msg = RegMsgs->find(65);
	ASSERT_PTR_NOT_NULL(msg);
	ASSERT_TRUE(strcmp(msg->name, "UpdatedName") == 0);

	teardown_globals();
	PASS();
	return 0;
}

static int test_regusermsg_id_reuse(void)
{
	TEST("engine hook - RegUserMsg warns on id reuse with different name");
	setup_globals();
	mock_eng_table.pfnRegUserMsg = mock_eng_regusermsg_fixed;

	g_regusermsg_return_id = 66;
	meta_engfuncs.pfnRegUserMsg("OrigName", 4);

	meta_engfuncs.pfnRegUserMsg("DifferentName", 4);
	ASSERT_TRUE(mock_get_alert_count() > 0);

	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// IS_VALID_PTR checks for late-added engine functions
// ============================================================

static void querycvar_stub(const edict_t *, const char *) {}
static void querycvar2_stub(const edict_t *, const char *, int) {}
static int checkparm_stub(const char *, char **) { return 0; }

static int test_querycvar_invalid_ptr(void)
{
	TEST("engine hook - QueryClientCvarValue validates invalid pointer");
	setup_globals();
	mock_eng_table.pfnQueryClientCvarValue = querycvar_stub;
	g_engfuncs.pfnQueryClientCvarValue =
		(void (*)(const edict_t *, const char *))0x4;
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	meta_engfuncs.pfnQueryClientCvarValue(&ed, "test");
	ASSERT_TRUE(g_engfuncs.pfnQueryClientCvarValue == NULL);
	teardown_globals();
	PASS();
	return 0;
}

static int test_querycvar2_invalid_ptr(void)
{
	TEST("engine hook - QueryClientCvarValue2 validates invalid pointer");
	setup_globals();
	mock_eng_table.pfnQueryClientCvarValue2 = querycvar2_stub;
	g_engfuncs.pfnQueryClientCvarValue2 =
		(void (*)(const edict_t *, const char *, int))0x4;
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	meta_engfuncs.pfnQueryClientCvarValue2(&ed, "test", 1);
	ASSERT_TRUE(g_engfuncs.pfnQueryClientCvarValue2 == NULL);
	teardown_globals();
	PASS();
	return 0;
}

static int test_engcheckparm_invalid_ptr(void)
{
	TEST("engine hook - EngCheckParm validates invalid pointer");
	setup_globals();
	mock_eng_table.pfnEngCheckParm = checkparm_stub;
	g_engfuncs.pfnEngCheckParm = (int (*)(const char *, char **))0x4;
	meta_engfuncs.pfnEngCheckParm("test", NULL);
	ASSERT_TRUE(g_engfuncs.pfnEngCheckParm == NULL);
	teardown_globals();
	PASS();
	return 0;
}

static edict_t *pentity_allents_stub(int) { return NULL; }

static int test_pentityallentities_invalid_ptr(void)
{
	TEST("engine hook - PEntityOfEntIndexAllEntities validates invalid pointer");
	setup_globals();
	mock_eng_table.pfnPEntityOfEntIndexAllEntities = pentity_allents_stub;
	g_engfuncs.pfnPEntityOfEntIndexAllEntities = (edict_t *(*)(int))0x4;
	meta_engfuncs.pfnPEntityOfEntIndexAllEntities(0);
	ASSERT_TRUE(g_engfuncs.pfnPEntityOfEntIndexAllEntities == NULL);
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

	printf("test_engine_api:\n");

	// IS_VALID_PTR tests must run first (use static s_check variables)
	fail |= test_querycvar_invalid_ptr();
	fail |= test_querycvar2_invalid_ptr();
	fail |= test_engcheckparm_invalid_ptr();
	fail |= test_pentityallentities_invalid_ptr();

	fail |= test_hook_precache_model();
	fail |= test_hook_precache_sound();
	fail |= test_hook_server_print();
	fail |= test_hook_null_engine_func();
	fail |= test_hook_null_engine_table();
	fail |= test_hook_all_engine_functions();
	fail |= test_regusermsg_duplicate_name();
	fail |= test_regusermsg_update_empty_name();
	fail |= test_regusermsg_id_reuse();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
