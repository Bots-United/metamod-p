//
// metamod-p - engine mock for unit testing
//
// engine_mock.cpp
//
// Provides stub implementations of engine, metamod globals and functions
// needed to link metamod source files for testing.
//

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <new>

#include <extdll.h>

#include "metamod.h"
#include "log_meta.h"
#include "engine_t.h"

#include "api_info.h"
#include "api_hook.h"
#include "mutil.h"
#include "engine_api.h"
#include "commands_meta.h"

#include "engine_mock.h"

// ============================================================
// Engine globals
// All weak so that real .o files (h_export.o, metamod.o, etc.)
// can override when linked into a test.
// ============================================================

static globalvars_t mock_globalvars;
__attribute__((weak)) globalvars_t *gpGlobals = &mock_globalvars;

__attribute__((weak)) HL_enginefuncs_t g_engfuncs;
__attribute__((weak)) engine_t Engine;
__attribute__((weak)) enginefuncs_t *Engine_funcs = NULL;
__attribute__((weak)) meta_enginefuncs_t g_plugin_engfuncs;

// ============================================================
// Metamod globals
// ============================================================

__attribute__((weak)) DLHANDLE metamod_handle = NULL;
__attribute__((weak)) cvar_t meta_version = {(char *)"metamod_version", (char *)"test", 0, 0, NULL};

__attribute__((weak)) gamedll_t GameDLL;
__attribute__((weak)) gamedll_funcs_t GameDLL_funcs;

__attribute__((weak)) MConfig *Config = NULL;
__attribute__((weak)) MPluginList *Plugins = NULL;
__attribute__((weak)) MRegCmdList *RegCmds = NULL;
__attribute__((weak)) MRegCvarList *RegCvars = NULL;
__attribute__((weak)) MRegMsgList *RegMsgs = NULL;

__attribute__((weak)) meta_globals_t PublicMetaGlobals;
__attribute__((weak)) meta_globals_t PrivateMetaGlobals;

__attribute__((weak)) DLL_FUNCTIONS *g_pHookedDllFunctions = NULL;
__attribute__((weak)) NEW_DLL_FUNCTIONS *g_pHookedNewDllFunctions = NULL;

__attribute__((weak)) int metamod_not_loaded = 0;

__attribute__((weak)) MPlayerList g_Players;

__attribute__((weak)) int requestid_counter = 0;

// ============================================================
// Logging globals
// ============================================================

__attribute__((weak)) cvar_t meta_debug = {(char *)"meta_debug", (char *)"0", 0, 0, NULL};
__attribute__((weak)) int meta_debug_value = 0;

// ============================================================
// support_meta globals
// ============================================================

__attribute__((weak)) META_ERRNO meta_errno;

// ============================================================
// Capture buffers for verifying engine output in tests
// ============================================================

static char mock_alert_msgs[MOCK_MAX_MSGS][MOCK_MSG_LEN];
static int mock_alert_count;

static char mock_server_print_msgs[MOCK_MAX_MSGS][MOCK_MSG_LEN];
static int mock_server_print_count;

static char mock_client_print_msgs[MOCK_MAX_MSGS][MOCK_MSG_LEN];
static int mock_client_print_count;

int mock_get_alert_count(void) { return mock_alert_count; }
const char *mock_get_alert_msg(int index)
{
	if (index < 0 || index >= mock_alert_count) return "";
	return mock_alert_msgs[index];
}

int mock_get_server_print_count(void) { return mock_server_print_count; }
const char *mock_get_server_print_msg(int index)
{
	if (index < 0 || index >= mock_server_print_count) return "";
	return mock_server_print_msgs[index];
}

int mock_get_client_print_count(void) { return mock_client_print_count; }
const char *mock_get_client_print_msg(int index)
{
	if (index < 0 || index >= mock_client_print_count) return "";
	return mock_client_print_msgs[index];
}

// ============================================================
// Settable CMD_ARGV/ARGC/ARGS state
// ============================================================

static int mock_argc;
static const char *mock_argv[16];
static const char *mock_args_str;

void mock_set_cmd_args(int argc, const char **argv, const char *args)
{
	mock_argc = argc;
	for (int i = 0; i < argc && i < 16; i++)
		mock_argv[i] = argv[i];
	mock_args_str = args;
}

// ============================================================
// Settable CVarGetFloat state
// ============================================================

#define MOCK_MAX_CVARS 16
static struct { const char *name; float value; } mock_cvar_floats[MOCK_MAX_CVARS];
static int mock_cvar_float_count;

void mock_set_cvar_float(const char *name, float value)
{
	for (int i = 0; i < mock_cvar_float_count; i++) {
		if (strcmp(mock_cvar_floats[i].name, name) == 0) {
			mock_cvar_floats[i].value = value;
			return;
		}
	}
	if (mock_cvar_float_count < MOCK_MAX_CVARS) {
		mock_cvar_floats[mock_cvar_float_count].name = name;
		mock_cvar_floats[mock_cvar_float_count].value = value;
		mock_cvar_float_count++;
	}
}

// ============================================================
// Settable IndexOfEdict state
// ============================================================

static int mock_edict_index;

void mock_set_index_of_edict(int index)
{
	mock_edict_index = index;
}

static char mock_gamedir[PATH_MAX];

void mock_set_gamedir(const char *dir)
{
	strncpy(mock_gamedir, dir, sizeof(mock_gamedir) - 1);
	mock_gamedir[sizeof(mock_gamedir) - 1] = '\0';
}

// ============================================================
// Engine function stubs
// ============================================================

static void mock_pfnAlertMessage(ALERT_TYPE atype, char *szFmt, ...)
{
	(void)atype;
	if (mock_alert_count < MOCK_MAX_MSGS) {
		va_list ap;
		va_start(ap, szFmt);
		vsnprintf(mock_alert_msgs[mock_alert_count], MOCK_MSG_LEN,
		          szFmt, ap);
		va_end(ap);
		mock_alert_count++;
	}
}

static void mock_pfnServerPrint(const char *msg)
{
	if (mock_server_print_count < MOCK_MAX_MSGS) {
		strncpy(mock_server_print_msgs[mock_server_print_count], msg,
		        MOCK_MSG_LEN - 1);
		mock_server_print_msgs[mock_server_print_count][MOCK_MSG_LEN - 1] = '\0';
		mock_server_print_count++;
	}
}

static float mock_pfnCVarGetFloat(const char *szVarName)
{
	for (int i = 0; i < mock_cvar_float_count; i++) {
		if (strcmp(mock_cvar_floats[i].name, szVarName) == 0)
			return mock_cvar_floats[i].value;
	}
	return 0.0f;
}

static const char *mock_pfnCVarGetString(const char *szVarName)
{
	(void)szVarName;
	return "";
}

static void mock_pfnCVarSetFloat(const char *szVarName, float value)
{
	(void)szVarName; (void)value;
}

static void mock_pfnCVarSetString(const char *szVarName, const char *value)
{
	(void)szVarName; (void)value;
}

static int mock_pfnIndexOfEdict(const edict_t *pEdict)
{
	(void)pEdict;
	return mock_edict_index;
}

static int mock_pfnEntOffsetOfPEntity(const edict_t *pEdict)
{
	if (!pEdict) return 0;
	return 1;
}

static void mock_pfnAddServerCommand(char *cmd_name, void (*function)(void))
{
	(void)cmd_name; (void)function;
}

static void mock_pfnCVarRegister(cvar_t *variable)
{
	(void)variable;
}

static cvar_t *mock_pfnCVarGetPointer(const char *szVarName)
{
	(void)szVarName;
	return NULL;
}

static int mock_pfnRegUserMsg(const char *pszName, int iSize)
{
	(void)iSize;
	// MRegMsgList::add() passes a strdup'd name; free it here to avoid
	// valgrind leak reports (the real engine would store the pointer).
	free((void *)pszName);
	return 0;
}

static void mock_pfnMessageBegin(int msg_dest, int msg_type,
                                 const float *pOrigin, edict_t *ed)
{
	(void)msg_dest; (void)msg_type; (void)pOrigin; (void)ed;
}

static void mock_pfnMessageEnd(void) {}
static void mock_pfnWriteByte(int val) { (void)val; }
static void mock_pfnWriteChar(int val) { (void)val; }
static void mock_pfnWriteShort(int val) { (void)val; }
static void mock_pfnWriteLong(int val) { (void)val; }
static void mock_pfnWriteAngle(float val) { (void)val; }
static void mock_pfnWriteCoord(float val) { (void)val; }
static void mock_pfnWriteString(const char *s) { (void)s; }
static void mock_pfnWriteEntity(int val) { (void)val; }

static edict_t mock_edicts[33];

static edict_t *mock_pfnPEntityOfEntIndex(int iEntIndex)
{
	if (iEntIndex < 0 || iEntIndex >= 33) return NULL;
	return &mock_edicts[iEntIndex];
}

static char *mock_pfnGetInfoKeyBuffer(edict_t *e)
{
	static char buf[256] = "";
	(void)e;
	return buf;
}

#define MOCK_MAX_LOCALINFO 16
static char mock_localinfo_keys[MOCK_MAX_LOCALINFO][64];
static char mock_localinfo_values[MOCK_MAX_LOCALINFO][256];
static int mock_localinfo_count;

static char *mock_pfnInfoKeyValue(char *infobuffer, char *key)
{
	static char empty[] = "";
	(void)infobuffer;
	for (int i = 0; i < mock_localinfo_count; i++) {
		if (strcmp(mock_localinfo_keys[i], key) == 0)
			return mock_localinfo_values[i];
	}
	return empty;
}

void mock_set_localinfo(const char *key, const char *value)
{
	for (int i = 0; i < mock_localinfo_count; i++) {
		if (strcmp(mock_localinfo_keys[i], key) == 0) {
			strncpy(mock_localinfo_values[i], value, sizeof(mock_localinfo_values[i]) - 1);
			mock_localinfo_values[i][sizeof(mock_localinfo_values[i]) - 1] = '\0';
			return;
		}
	}
	if (mock_localinfo_count < MOCK_MAX_LOCALINFO) {
		strncpy(mock_localinfo_keys[mock_localinfo_count], key, sizeof(mock_localinfo_keys[0]) - 1);
		mock_localinfo_keys[mock_localinfo_count][sizeof(mock_localinfo_keys[0]) - 1] = '\0';
		strncpy(mock_localinfo_values[mock_localinfo_count], value, sizeof(mock_localinfo_values[0]) - 1);
		mock_localinfo_values[mock_localinfo_count][sizeof(mock_localinfo_values[0]) - 1] = '\0';
		mock_localinfo_count++;
	}
}

static void mock_pfnClientPrintf(edict_t *pEdict, PRINT_TYPE ptype, const char *szMsg)
{
	(void)pEdict; (void)ptype;
	if (mock_client_print_count < MOCK_MAX_MSGS) {
		strncpy(mock_client_print_msgs[mock_client_print_count], szMsg,
		        MOCK_MSG_LEN - 1);
		mock_client_print_msgs[mock_client_print_count][MOCK_MSG_LEN - 1] = '\0';
		mock_client_print_count++;
	}
}

static void mock_pfnGetGameDir(char *szGetGameDir)
{
	strcpy(szGetGameDir, mock_gamedir);
}

static void mock_pfnServerCommand(char *str) { (void)str; }
static byte *mock_pfnLoadFileForMe(char *filename, int *pLength)
{
	(void)filename;
	if (pLength) *pLength = 0;
	return NULL;
}
static void mock_pfnFreeFile(void *buffer) { (void)buffer; }
static void mock_pfnSetKeyValue(char *infobuffer, char *key, char *value)
{
	(void)infobuffer; (void)key; (void)value;
}

static int mock_pfnCmd_Argc(void) { return mock_argc; }
static const char *mock_pfnCmd_Argv(int argc)
{
	if (argc < 0 || argc >= mock_argc) return "";
	return mock_argv[argc];
}
static const char *mock_pfnCmd_Args(void)
{
	return mock_args_str ? mock_args_str : "";
}

// ============================================================
// mock_reset - reset all mock state between tests
// ============================================================

void mock_reset(void)
{
	mock_globalvars = globalvars_t();
	mock_globalvars.maxClients = 32;
	gpGlobals = &mock_globalvars;

	g_engfuncs = HL_enginefuncs_t();
	g_engfuncs.pfnAlertMessage = mock_pfnAlertMessage;
	g_engfuncs.pfnServerPrint = mock_pfnServerPrint;
	g_engfuncs.pfnCVarGetFloat = mock_pfnCVarGetFloat;
	g_engfuncs.pfnCVarGetString = mock_pfnCVarGetString;
	g_engfuncs.pfnCVarSetFloat = mock_pfnCVarSetFloat;
	g_engfuncs.pfnCVarSetString = mock_pfnCVarSetString;
	g_engfuncs.pfnIndexOfEdict = mock_pfnIndexOfEdict;
	g_engfuncs.pfnEntOffsetOfPEntity = mock_pfnEntOffsetOfPEntity;
	g_engfuncs.pfnPEntityOfEntIndex = mock_pfnPEntityOfEntIndex;
	g_engfuncs.pfnAddServerCommand = mock_pfnAddServerCommand;
	g_engfuncs.pfnCVarRegister = mock_pfnCVarRegister;
	g_engfuncs.pfnCVarGetPointer = mock_pfnCVarGetPointer;
	g_engfuncs.pfnRegUserMsg = mock_pfnRegUserMsg;
	g_engfuncs.pfnMessageBegin = mock_pfnMessageBegin;
	g_engfuncs.pfnMessageEnd = mock_pfnMessageEnd;
	g_engfuncs.pfnWriteByte = mock_pfnWriteByte;
	g_engfuncs.pfnWriteChar = mock_pfnWriteChar;
	g_engfuncs.pfnWriteShort = mock_pfnWriteShort;
	g_engfuncs.pfnWriteLong = mock_pfnWriteLong;
	g_engfuncs.pfnWriteAngle = mock_pfnWriteAngle;
	g_engfuncs.pfnWriteCoord = mock_pfnWriteCoord;
	g_engfuncs.pfnWriteString = mock_pfnWriteString;
	g_engfuncs.pfnWriteEntity = mock_pfnWriteEntity;
	g_engfuncs.pfnGetInfoKeyBuffer = mock_pfnGetInfoKeyBuffer;
	g_engfuncs.pfnInfoKeyValue = mock_pfnInfoKeyValue;
	g_engfuncs.pfnClientPrintf = mock_pfnClientPrintf;
	g_engfuncs.pfnCmd_Argc = mock_pfnCmd_Argc;
	g_engfuncs.pfnCmd_Argv = mock_pfnCmd_Argv;
	g_engfuncs.pfnCmd_Args = mock_pfnCmd_Args;
	g_engfuncs.pfnGetGameDir = mock_pfnGetGameDir;
	g_engfuncs.pfnServerCommand = mock_pfnServerCommand;
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;
	g_engfuncs.pfnSetKeyValue = mock_pfnSetKeyValue;

	mock_gamedir[0] = '\0';
	mock_localinfo_count = 0;

	Engine_funcs = (enginefuncs_t *)&g_engfuncs;
	Engine.globals = gpGlobals;

	memset(&GameDLL, 0, sizeof(GameDLL));
	memset(&GameDLL_funcs, 0, sizeof(GameDLL_funcs));

	metamod_handle = NULL;
	Config = NULL;
	Plugins = NULL;
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;

	memset(&PublicMetaGlobals, 0, sizeof(PublicMetaGlobals));
	memset(&PrivateMetaGlobals, 0, sizeof(PrivateMetaGlobals));

	g_pHookedDllFunctions = NULL;
	g_pHookedNewDllFunctions = NULL;

	g_Players.~MPlayerList();
	new (&g_Players) MPlayerList();

	g_plugin_engfuncs = meta_enginefuncs_t();

	meta_debug_value = 0;
	meta_errno = ME_NOERROR;

	metamod_not_loaded = 0;
	requestid_counter = 0;

	mock_alert_count = 0;
	mock_server_print_count = 0;
	mock_client_print_count = 0;
	mock_argc = 0;
	memset(mock_argv, 0, sizeof(mock_argv));
	mock_args_str = NULL;
	mock_cvar_float_count = 0;
	mock_edict_index = 0;
}
