/*
 * Fake metamod plugin .so for unit testing.
 *
 * Environment variables:
 *   FAKE_MM_LOADABLE     - PLUG_LOADTIME value (default: 3 = PT_ANYTIME)
 *   FAKE_MM_UNLOADABLE   - PLUG_LOADTIME value (default: 3 = PT_ANYTIME)
 *   FAKE_MM_ATTACH_FAIL  - if set to "1", Meta_Attach returns failure
 *
 * PLUG_LOADTIME values:
 *   0 = PT_NEVER
 *   1 = PT_STARTUP
 *   2 = PT_CHANGELEVEL
 *   3 = PT_ANYTIME
 *   4 = PT_ANYPAUSE
 */
#include <stdlib.h>
#include <string.h>

/* Minimal redefinitions to avoid pulling in HL SDK headers. */
typedef enum {
	PT_NEVER = 0,
	PT_STARTUP,
	PT_CHANGELEVEL,
	PT_ANYTIME,
	PT_ANYPAUSE,
} PLUG_LOADTIME;

typedef struct {
	char *ifvers;
	char *name;
	char *version;
	char *date;
	char *author;
	char *url;
	char *logtag;
	PLUG_LOADTIME loadable;
	PLUG_LOADTIME unloadable;
} plugin_info_t;

static plugin_info_t s_info;

/* Exported cvar-like struct for unit test usage. */
typedef struct {
	char *name;
	char *string;
	int flags;
	float value;
	void *next;
} test_cvar_t;

test_cvar_t fake_plugin_cvar = {
	"sv_fake_plugin", "1", 0, 1.0f, 0
};

void GiveFnptrsToDll(void *pFuncs, void *pGlobals) {
	(void)pFuncs;
	(void)pGlobals;
}

int Meta_Query(char *interfaceVersion, plugin_info_t **plinfo, void *pMetaUtilFuncs) {
	const char *env;
	int loadable_val;
	int unloadable_val;

	(void)interfaceVersion;
	(void)pMetaUtilFuncs;

	env = getenv("FAKE_MM_LOADABLE");
	loadable_val = env ? atoi(env) : PT_ANYTIME;
	env = getenv("FAKE_MM_UNLOADABLE");
	unloadable_val = env ? atoi(env) : PT_ANYTIME;

	memset(&s_info, 0, sizeof(s_info));
	s_info.ifvers = "5:13";
	s_info.name = "FakeMMPlugin";
	s_info.version = "0.1";
	s_info.date = "2025-01-01";
	s_info.author = "Test";
	s_info.url = "http://test";
	s_info.logtag = "FAKEMM";
	s_info.loadable = (PLUG_LOADTIME)loadable_val;
	s_info.unloadable = (PLUG_LOADTIME)unloadable_val;

	*plinfo = &s_info;
	return 1; /* TRUE */
}

int Meta_Attach(PLUG_LOADTIME now, void *pFunctionTable, void *pMGlobals, void *pGamedllFuncs) {
	const char *env;
	(void)now;
	(void)pFunctionTable;
	(void)pMGlobals;
	(void)pGamedllFuncs;

	env = getenv("FAKE_MM_ATTACH_FAIL");
	if (env && env[0] == '1')
		return 0; /* FALSE */
	return 1; /* TRUE */
}

int Meta_Detach(PLUG_LOADTIME now, int reason) {
	(void)now;
	(void)reason;
	return 1; /* TRUE */
}
