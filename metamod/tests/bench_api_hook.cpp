//
// metamod-p - dispatch cost microbenchmark
//
// Drives real API wrappers in a fixed-iteration loop: wrapper ->
// main_hook_function -> hooked plugins -> gamedll/engine, with trivial
// callees so what is left is metamod's own dispatch and argument
// marshalling. A live server buries that under ~97% unrelated cycles.
//
// Measured across several plugin populations, because a dispatch walks the
// hook list for the api group and not for the function being called:
//
//   N  plugins registered a table for the api group, so they sit in that
//      group's hook list and the dispatch loop walks over all of them
//   K  of those N actually hook the function being called
//
// The N-K plugins in between hook other functions of the same table, which
// is what a real plugin looks like: a table has 50 (dllapi), 175 (engine)
// or 5 (newapi) slots and a plugin fills a handful. Every plugin gets its
// own tables so the loads scatter the way they do when each table lives in
// its own shared object.
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
static enginefuncs_t engine_table;
static enginefuncs_t *engine_table_ptr = &engine_table;

// One table per plugin, pre and post, so walking the list touches a
// different cache line for every entry.
#define MAX_BENCH_PLUGINS 40
static DLL_FUNCTIONS bench_dll_tables[MAX_BENCH_PLUGINS];
static DLL_FUNCTIONS bench_dll_post_tables[MAX_BENCH_PLUGINS];
static enginefuncs_t bench_eng_tables[MAX_BENCH_PLUGINS];
static enginefuncs_t bench_eng_post_tables[MAX_BENCH_PLUGINS];

// ---------------------------------------------------------------
// Callees. Empty, but not inlinable: what is being timed is the cost of
// reaching them, and each must set meta_result or dispatch warns.
// ---------------------------------------------------------------

static NOINLINE void plug_void_p(edict_t *) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_void_2p(edict_t *, edict_t *) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_void_p2i(edict_t *, int, int) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_void_4pi(SAVERESTOREDATA *, const char *, void *, TYPEDESCRIPTION *, int)
	{ PublicMetaGlobals.mres = MRES_IGNORED; }

static NOINLINE void plug_setmodel(edict_t *, const char *) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_runplayermove(edict_t *, const float *, float, float, float,
					unsigned short, byte, byte)
	{ PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_playbackevent(int, const edict_t *, unsigned short, float, float *,
					float *, float, float, int, int, int, int)
	{ PublicMetaGlobals.mres = MRES_IGNORED; }

// Hooks that are never called, so a plugin that does not hook the function
// under test still holds a table with something in it.
static NOINLINE void plug_filler_touch(edict_t *, edict_t *) { PublicMetaGlobals.mres = MRES_IGNORED; }
static NOINLINE void plug_filler_setorigin(edict_t *, const float *) { PublicMetaGlobals.mres = MRES_IGNORED; }

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

// Calls the wrapper ITERS times and keeps the best per-call tick count. The
// wrapper is reached through a volatile pointer so the call cannot be
// devirtualized or hoisted.
#define BENCH(slot, call) do { \
	double best = 0; \
	for (int rep = 0; rep < REPS; rep++) { \
		unsigned long long t0 = rdtsc_now(); \
		for (int i = 0; i < ITERS; i++) { call; } \
		unsigned long long t1 = rdtsc_now(); \
		double per = (double)(t1 - t0) / ITERS; \
		if (rep == 0 || per < best) best = per; \
	} \
	(slot) = best; \
} while (0)

// ---------------------------------------------------------------
// Plugin populations
// ---------------------------------------------------------------

struct bench_config_t {
	const char *label;
	int nplugins;		// plugins holding a table for the api group
	int khooks;		// of those, how many hook the timed function
	int with_post;		// also register post tables, hooking nothing
};

#define NCONFIGS  6
#define NWRAPPERS 7

// K=0 is the case a server spends most of its time in: of the 214 functions
// across the three tables, any one of them is hooked by nobody far more
// often than by somebody, and the wrappers are called regardless.
static const bench_config_t configs[NCONFIGS] = {
	{ "N=1/K=1",        1, 1, 0 },
	{ "N=5/K=1",        5, 1, 0 },
	{ "N=20/K=2",      20, 2, 0 },
	{ "N=40/K=2",      40, 2, 0 },
	{ "N=20/K=2+post", 20, 2, 1 },
	{ "N=20/K=0",      20, 0, 0 },
};

static const char *wrapper_names[NWRAPPERS] = {
	"dllapi Think (p, 1)",
	"dllapi Use (2p, 2)",
	"dllapi ServerActivate (p2i, 3)",
	"dllapi SaveWriteFields (4pi, 5)",
	"engine SetModel (2p, 2)",
	"engine RunPlayerMove (2p3fus2uc, 8)",
	"engine PlaybackEvent (ipusf2p2f4i, 12)",
};

static double results[NCONFIGS][NWRAPPERS];

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
}

// Builds a population of n running plugins, k of them hooking the functions
// under test, spread evenly through the list.
static void setup_plugins(int n, int k, int with_post)
{
	char is_hooker[MAX_BENCH_PLUGINS];
	int i, j;

	memset(is_hooker, 0, sizeof(is_hooker));
	for (j = 0; j < k; j++)
		is_hooker[(j * n) / k] = 1;

	free(bench_plugins.hook_list_data);
	memset(&bench_plugins, 0, sizeof(bench_plugins));

	for (i = 0; i < n; i++) {
		DLL_FUNCTIONS *dt = &bench_dll_tables[i];
		DLL_FUNCTIONS *dpt = &bench_dll_post_tables[i];
		enginefuncs_t *et = &bench_eng_tables[i];
		enginefuncs_t *ept = &bench_eng_post_tables[i];
		MPlugin *plug = &bench_plugins.plist[i];

		memset(dt, 0, sizeof(*dt));
		memset(dpt, 0, sizeof(*dpt));
		memset(et, 0, sizeof(*et));
		memset(ept, 0, sizeof(*ept));

		// Something this plugin hooks other than what is timed.
		dt->pfnTouch = plug_filler_touch;
		dpt->pfnTouch = plug_filler_touch;
		et->pfnSetOrigin = plug_filler_setorigin;
		ept->pfnSetOrigin = plug_filler_setorigin;

		if (is_hooker[i]) {
			dt->pfnThink = plug_void_p;
			dt->pfnUse = plug_void_2p;
			dt->pfnServerActivate = plug_void_p2i;
			dt->pfnSaveWriteFields = plug_void_4pi;
			et->pfnSetModel = plug_setmodel;
			et->pfnRunPlayerMove = plug_runplayermove;
			et->pfnPlaybackEvent = plug_playbackevent;
		}

		memset(plug, 0, sizeof(*plug));
		plug->status = PL_RUNNING;
		plug->index = i + 1;
		snprintf(plug->filename, sizeof(plug->filename), "bench_plugin%d", i);
		plug->file = plug->filename;
		plug->tables.dllapi = dt;
		plug->tables.engine = et;
		if (with_post) {
			plug->post_tables.dllapi = dpt;
			plug->post_tables.engine = ept;
		}
	}

	bench_plugins.endlist = n;
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
	int c, w;

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

	for (c = 0; c < NCONFIGS; c++) {
		setup_plugins(configs[c].nplugins, configs[c].khooks, configs[c].with_post);

		BENCH(results[c][0], think(&ed));
		BENCH(results[c][1], use(&ed, &ed));
		BENCH(results[c][2], activate(&ed, 32, 16));
		BENCH(results[c][3], savefields(&srd, "n", &srd, &td, 1));
		BENCH(results[c][4], setmodel(&ed, "m"));
		BENCH(results[c][5], runmove(&ed, vec, 1.0f, 2.0f, 3.0f,
					     (unsigned short)0x1234, (byte)7, (byte)13));
		BENCH(results[c][6], playback(1, &ed, (unsigned short)0x5678, 0.5f, vec, vec,
					      1.5f, 2.5f, 3, 4, 5, 6));
	}

	printf("TSC ticks per call (%d iters, best of %d)\n\n", ITERS, REPS);
	printf("%-40s", "wrapper (pack, args)");
	for (c = 0; c < NCONFIGS; c++)
		printf("%15s", configs[c].label);
	printf("\n");
	for (c = 0; c < 40 + 15 * NCONFIGS; c++)
		printf("-");
	printf("\n");
	for (w = 0; w < NWRAPPERS; w++) {
		printf("%-40s", wrapper_names[w]);
		for (c = 0; c < NCONFIGS; c++)
			printf("%15.1f", results[c][w]);
		printf("\n");
	}

	return 0;
}
