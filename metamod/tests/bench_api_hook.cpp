//
// metamod-p - dispatch cost microbenchmark
//
// Drives real API wrappers in a fixed-iteration loop: wrapper ->
// main_hook_function -> one hooked plugin -> gamedll/engine, with trivial
// callees so what is left is metamod's own dispatch and argument
// marshalling. A live server buries that under ~97% unrelated cycles.
//
// Reports TSC ticks per call, minimum of several repetitions so an
// interrupt lands in the discarded runs rather than the reported one.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include <extdll.h>

#include "metamod.h"
#include "dllapi.h"
#include "engine_api.h"
#include "api_info.h"
#include "api_hook.h"
#include "meta_api.h"
#include "meta_eiface.h"
#include "mlist.h"
#include "mreg.h"
#include "mplugin.h"
#include "conf_meta.h"

#include "engine_mock.h"

meta_globals_t *gpMetaGlobals = &PublicMetaGlobals;

static MPluginList &bench_plugins = *new MPluginList("plugins.ini");
static MRegCmdList bench_cmds;
static MRegCvarList bench_cvars;
static MRegMsgList bench_msgs;
static MConfig bench_config;
static option_t bench_options[] = { { NULL, CF_NONE, NULL, NULL } };

static DLL_FUNCTIONS gamedll_table;
static DLL_FUNCTIONS plugin_pre_table;
static enginefuncs_t engine_table;
static enginefuncs_t *engine_table_ptr = &engine_table;

// ---------------------------------------------------------------
// Callees. Empty, but not inlinable: what is being timed is the cost of
// reaching them, and each must set meta_result or dispatch warns.
// ---------------------------------------------------------------

static NOINLINE void plug_void_p(edict_t *) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_void_2p(edict_t *, edict_t *) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_void_p2i(edict_t *, int, int) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_void_4pi(SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int)
	{ PublicMetaGlobals.mres = MRES_IGNORED; }

static NOINLINE void dll_void_p(edict_t *) {}
static NOINLINE void dll_void_2p(edict_t *, edict_t *) {}
static NOINLINE void dll_void_p2i(edict_t *, int, int) {}
static NOINLINE void dll_void_4pi(SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int) {}

static NOINLINE void eng_runplayermove(edict_t *, const float *, float, float, float,
				       unsigned short, byte, byte) {}
static NOINLINE void eng_playbackevent(int, const edict_t *, unsigned short, float, float *,
				       float *, float, float, int, int, int, int) {}
static NOINLINE void eng_setmodel(edict_t *, const char *) {}

// ---------------------------------------------------------------
// Timing
// ---------------------------------------------------------------

static inline unsigned long long rdtsc_now(void)
{
	unsigned int lo, hi;
	__asm__ __volatile__ ("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) :: "memory");
	return ((unsigned long long)hi << 32) | lo;
}

#define ITERS 200000
#define REPS  15

// Each case is a block that calls the wrapper ITERS times; the wrapper is
// reached through a volatile pointer so the call cannot be devirtualized
// or hoisted.
#define BENCH(name, decl_fn, setup, call) do { \
	double best = 0; \
	for (int rep = 0; rep < REPS; rep++) { \
		setup; \
		unsigned long long t0 = rdtsc_now(); \
		for (int i = 0; i < ITERS; i++) { call; } \
		unsigned long long t1 = rdtsc_now(); \
		double per = (double)(t1 - t0) / ITERS; \
		if (rep == 0 || per < best) best = per; \
	} \
	printf("%-28s %8.1f\n", name, best); \
} while (0)

static void setup_globals(void)
{
	mock_reset();
	Plugins = &bench_plugins;
	RegCmds = &bench_cmds;
	RegCvars = &bench_cvars;
	RegMsgs = &bench_msgs;
	memset(&bench_config, 0, sizeof(bench_config));
	bench_config.init(bench_options);
	Config = &bench_config;
	metamod_not_loaded = 0;

	memset(&gamedll_table, 0, sizeof(gamedll_table));
	gamedll_table.pfnThink = dll_void_p;
	gamedll_table.pfnUse = dll_void_2p;
	gamedll_table.pfnServerActivate = dll_void_p2i;
	gamedll_table.pfnSaveWriteFields = dll_void_4pi;
	GameDLL_funcs.dllapi_table = &gamedll_table;

	memset(&engine_table, 0, sizeof(engine_table));
	engine_table.pfnRunPlayerMove = eng_runplayermove;
	engine_table.pfnPlaybackEvent = eng_playbackevent;
	engine_table.pfnSetModel = eng_setmodel;
	Engine_funcs = engine_table_ptr;

	// One plugin hooking the same functions, so each dispatch walks a
	// non-empty hook list the way a real server does.
	memset(&plugin_pre_table, 0, sizeof(plugin_pre_table));
	plugin_pre_table.pfnThink = plug_void_p;
	plugin_pre_table.pfnUse = plug_void_2p;
	plugin_pre_table.pfnServerActivate = plug_void_p2i;
	plugin_pre_table.pfnSaveWriteFields = plug_void_4pi;

	free(bench_plugins.hook_list_data);
	memset(&bench_plugins, 0, sizeof(bench_plugins));
	MPlugin *plug = &bench_plugins.plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->index = 1;
	snprintf(plug->filename, sizeof(plug->filename), "bench_plugin");
	plug->file = plug->filename;
	plug->tables.dllapi = &plugin_pre_table;
	bench_plugins.endlist = 1;
	bench_plugins.rebuild_hook_lists();
}

int main(void)
{
	DLL_FUNCTIONS dll;
	int version = INTERFACE_VERSION;
	edict_t ed;
	float vec[3] = { 1.0f, 2.0f, 3.0f };
	SAVERESTOREDATA srd;
	TYPEDESCRIPTION td;

	memset(&ed, 0, sizeof(ed));
	memset(&srd, 0, sizeof(srd));
	memset(&td, 0, sizeof(td));

	setup_globals();
	meta_new_dll_functions_t::test_set_version(3);
	memset(&dll, 0, sizeof(dll));
	if (!GetEntityAPI2(&dll, &version)) {
		printf("GetEntityAPI2 failed\n");
		return 1;
	}

	printf("%-28s %8s   (%d iters, best of %d)\n", "wrapper (pack, args)", "ticks", ITERS, REPS);
	printf("---------------------------------------------\n");

	void (* volatile think)(edict_t *) = dll.pfnThink;
	void (* volatile use)(edict_t *, edict_t *) = dll.pfnUse;
	void (* volatile activate)(edict_t *, int, int) = dll.pfnServerActivate;
	void (* volatile savefields)(SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int)
		= dll.pfnSaveWriteFields;
	void (* volatile setmodel)(edict_t *, const char *) = meta_engfuncs.pfnSetModel;
	void (* volatile runmove)(edict_t *, const float *, float, float, float, unsigned short, byte, byte)
		= meta_engfuncs.pfnRunPlayerMove;
	void (* volatile playback)(int, const edict_t *, unsigned short, float, float *, float *,
				   float, float, int, int, int, int) = meta_engfuncs.pfnPlaybackEvent;

	BENCH("dllapi Think (p, 1)",        , (void)0, think(&ed));
	BENCH("dllapi Use (2p, 2)",         , (void)0, use(&ed, &ed));
	BENCH("dllapi ServerActivate (p2i, 3)", , (void)0, activate(&ed, 32, 16));
	BENCH("dllapi SaveWriteFields (4pi, 5)", , (void)0, savefields(&srd, "n", &srd, &td, 1));
	BENCH("engine SetModel (2p, 2)",    , (void)0, setmodel(&ed, "m"));
	BENCH("engine RunPlayerMove (2p3fus2uc, 8)", , (void)0,
	      runmove(&ed, vec, 1.0f, 2.0f, 3.0f, (unsigned short)0x1234, (byte)7, (byte)13));
	BENCH("engine PlaybackEvent (ipusf2p2f4i, 12)", , (void)0,
	      playback(1, &ed, (unsigned short)0x5678, 0.5f, vec, vec, 1.5f, 2.5f, 3, 4, 5, 6));

	return 0;
}
