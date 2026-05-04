//
// metamod-p - engine mock for unit testing
//
// engine_mock.cpp
//
// Provides stub implementations of engine, metamod globals and functions
// needed to link metamod source files for testing.
//

#include <string.h>
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
__attribute__((weak)) meta_enginefuncs_t g_plugin_engfuncs;

// ============================================================
// Metamod globals
// ============================================================

__attribute__((weak)) DLHANDLE metamod_handle = NULL;
__attribute__((weak)) cvar_t meta_version = {(char *)"metamod_version", (char *)"test", 0, 0, NULL};

__attribute__((weak)) gamedll_t GameDLL;

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
// Engine function stubs
// ============================================================

static void mock_pfnAlertMessage(ALERT_TYPE atype, char *szFmt, ...)
{
	(void)atype; (void)szFmt;
}

static void mock_pfnServerPrint(const char *msg)
{
	(void)msg;
}

static float mock_pfnCVarGetFloat(const char *szVarName)
{
	(void)szVarName;
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
	return 0;
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
	(void)pszName; (void)iSize;
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

static char *mock_pfnGetInfoKeyBuffer(edict_t *e)
{
	static char buf[256] = "";
	(void)e;
	return buf;
}

static char *mock_pfnInfoKeyValue(char *infobuffer, char *key)
{
	static char empty[] = "";
	(void)infobuffer; (void)key;
	return empty;
}

static void mock_pfnClientPrintf(edict_t *pEdict, PRINT_TYPE ptype, const char *szMsg)
{
	(void)pEdict; (void)ptype; (void)szMsg;
}

static int mock_pfnCmd_Argc(void) { return 0; }
static const char *mock_pfnCmd_Argv(int argc) { (void)argc; return ""; }
static const char *mock_pfnCmd_Args(void) { return ""; }

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

	Engine.funcs = (enginefuncs_t *)&g_engfuncs;
	Engine.globals = gpGlobals;

	memset(&GameDLL, 0, sizeof(GameDLL));

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
}
