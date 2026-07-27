//
// metamod-p - argument marshalling for every API hook wrapper
//
// A wrapper hands main_hook_function* the block the plugins and the gamedll
// are called with, mostly by passing on the one it was entered with, which
// only holds while that block really is the packed struct image.
//
// Every slot of every API table is driven with sentinel arguments and the
// delivered block compared against them, so a wrapper that stops passing
// its parameters through fails here instead of corrupting arguments.
// Walking the tables covers a newly added wrapper as soon as it appears.
//

#include <stdlib.h>
#include <string.h>
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
#include "conf_meta.h"

#include "engine_mock.h"
#include "test_common.h"

meta_globals_t *gpMetaGlobals = &PublicMetaGlobals;

// Reference wrappers: the same sources built with the argument copy forced
// on, their entry points renamed by the makefile. Global variables are not
// name-mangled, so the engine table is reachable as a plain slot array.
extern "C" int ref_GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
extern "C" int ref_GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pTable, int *interfaceVersion);
extern void *ref_meta_engfuncs[];

// A handful of wrappers consult metamod state before dispatching, so the
// globals they reach have to exist.
static MPluginList test_plugins("plugins.ini");
static MRegCmdList test_reg_cmds;
static MRegCvarList test_reg_cvars;
static MRegMsgList test_reg_msgs;
static MConfig test_config;
static option_t test_options[] = {
	{ NULL, CF_NONE, NULL, NULL }
};

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
}

// api_info's tables name every api_caller. The dispatchers are replaced
// below and never reach one, so the list is expanded here into stubs
// rather than linking the real api_hook.o, whose dispatchers would clash.
#define BEGIN_API_CALLER_FUNC(ret_type, args_type_code) \
	void * DLLINTERNAL _COMBINE4(api_caller_, ret_type, _args_, args_type_code)(const void *, const void *) {
#define END_API_CALLER_FUNC(ret_t, args_t, args)	return NULL; }
#define END_API_CALLER_FUNC_void(args_t, args)		return NULL; }
#define END_API_CALLER_FUNC_noargs(ret_t)		return NULL; }
#define END_API_CALLER_FUNC_noargs_void()		return NULL; }

#include "../api_caller_list.h"

#undef BEGIN_API_CALLER_FUNC
#undef END_API_CALLER_FUNC
#undef END_API_CALLER_FUNC_void
#undef END_API_CALLER_FUNC_noargs
#undef END_API_CALLER_FUNC_noargs_void

// An api_info entry names the caller that will consume the block, and the
// caller determines the packed struct, so expanding the same list a second
// time gives the exact block size to expect for every wrapper without
// keeping a parallel list by hand.
typedef struct {
	const void *caller;
	unsigned int nbytes;
} caller_size_t;

#define MAX_CALLER_SIZES 128
static caller_size_t caller_sizes[MAX_CALLER_SIZES];
static int num_caller_sizes;

static void add_caller_size(const void *caller, unsigned int nbytes)
{
	if (num_caller_sizes >= MAX_CALLER_SIZES)
		return;
	caller_sizes[num_caller_sizes].caller = caller;
	caller_sizes[num_caller_sizes].nbytes = nbytes;
	num_caller_sizes++;
}

// Expanded inside a function body: entries in the list are punctuated with
// stray semicolons, which are empty statements here but would not be
// allowed between initialisers.
#define BEGIN_API_CALLER_FUNC(ret_type, args_type_code) \
	add_caller_size((const void *)_COMBINE4(api_caller_, ret_type, _args_, args_type_code), \
			(unsigned int)sizeof(_COMBINE2(pack_args_type_, args_type_code)));
#define END_API_CALLER_FUNC(ret_t, args_t, args)
#define END_API_CALLER_FUNC_void(args_t, args)
#define END_API_CALLER_FUNC_noargs(ret_t)
#define END_API_CALLER_FUNC_noargs_void()

static void init_caller_sizes(void)
{
	num_caller_sizes = 0;
#include "../api_caller_list.h"
}
#define NUM_CALLER_SIZES num_caller_sizes

// pack_args_type_void is an empty placeholder, not an argument image.
#define NO_ARG_BYTES ((unsigned int)sizeof(pack_args_type_void))

static unsigned int caller_block_size(const void *caller, int *known)
{
	int i;
	for (i = 0; i < NUM_CALLER_SIZES; i++) {
		if (caller_sizes[i].caller == caller) {
			*known = 1;
			return caller_sizes[i].nbytes;
		}
	}
	*known = 0;
	return 0;
}

// Widest packed struct, and therefore the most argument slots any wrapper
// can hand over.
#define MAX_ARG_BYTES 48
#define MAX_ARG_SLOTS (MAX_ARG_BYTES / 4)

static unsigned char seen_block[MAX_ARG_BYTES];
static unsigned int seen_len;
static int seen_calls;

// Replaces the real dispatchers: records the block a wrapper delivered
// without calling any plugin or gamedll function.
void DLLINTERNAL main_hook_function_void(unsigned int, enum_api_t, unsigned int, const void *packed_args)
{
	seen_calls++;
	memcpy(seen_block, packed_args, seen_len);
}

void * DLLINTERNAL main_hook_function(const class_ret_t, unsigned int, enum_api_t, unsigned int, const void *packed_args)
{
	seen_calls++;
	memcpy(seen_block, packed_args, seen_len);
	return NULL;
}

// ============================================================
// Wrappers that legitimately deliver something other than their own
// incoming block.
// ============================================================

enum { T_DLL, T_NEW, T_ENG };

typedef struct {
	int table;
	unsigned int slot;	// index of the function pointer within the table
	unsigned int prefix;	// leading bytes that are still the wrapper's own arguments
	int ref_prefix_only;	// whether the reference may also diverge past it
	const char *why;
} exception_t;

static const exception_t exceptions[] = {
	// printf-style: past the first argument the block holds a formatted
	// buffer rather than anything the caller passed, and each copy of the
	// wrapper formats into a buffer of its own
	{ T_ENG, offsetof(enginefuncs_t, pfnClientCommand) / 4, 4, 1, "varargs" },
	{ T_ENG, offsetof(enginefuncs_t, pfnAlertMessage) / 4, 4, 1, "varargs" },
	{ T_ENG, offsetof(enginefuncs_t, pfnEngineFprintf) / 4, 4, 1, "varargs" },
	// kept on the packing path: the struct widens their sub-word arguments,
	// so the block diverges from the incoming slots at the first of them.
	// Both copies pack, so they must still agree over the whole block --
	// which is what catches one of these being moved off that path.
	{ T_ENG, offsetof(enginefuncs_t, pfnCRC32_ProcessByte) / 4, 4, 0, "sub-word" },
	{ T_ENG, offsetof(enginefuncs_t, pfnRunPlayerMove) / 4, 20, 0, "sub-word" },
	{ T_ENG, offsetof(enginefuncs_t, pfnPlaybackEvent) / 4, 8, 0, "sub-word" },
};
#define NUM_EXCEPTIONS ((int)(sizeof(exceptions) / sizeof(exceptions[0])))

static const exception_t *find_exception(int table, unsigned int slot)
{
	int i;
	for (i = 0; i < NUM_EXCEPTIONS; i++)
		if (exceptions[i].table == table && exceptions[i].slot == slot)
			return &exceptions[i];
	return NULL;
}

// ============================================================
// Driving a wrapper
// ============================================================

// Sentinels double as the pointer arguments a wrapper may dereference
// before dispatching, so they address readable zeroed storage.
static unsigned char arena[MAX_ARG_SLOTS * 256];
static unsigned int sentinel[MAX_ARG_SLOTS];

typedef void (*call_slots_t)(unsigned int, unsigned int, unsigned int, unsigned int,
			     unsigned int, unsigned int, unsigned int, unsigned int,
			     unsigned int, unsigned int, unsigned int, unsigned int);

static void init_sentinels(void)
{
	unsigned int i;
	memset(arena, 0, sizeof(arena));
	for (i = 0; i < MAX_ARG_SLOTS; i++)
		sentinel[i] = (unsigned int)(unsigned long)(arena + i * 256 + 16);
}

static void call_wrapper(void *fn)
{
	((call_slots_t)fn)(sentinel[0], sentinel[1], sentinel[2], sentinel[3],
			   sentinel[4], sentinel[5], sentinel[6], sentinel[7],
			   sentinel[8], sentinel[9], sentinel[10], sentinel[11]);
	// A wrapper returning float leaves its result on the x87 stack, which
	// nothing here pops; reset so repeated calls cannot overflow it.
	__asm__ __volatile__ ("fninit");
}

// Returns 0 on success, 1 on a reported failure.
static int check_table(const char *tabname, int table, void **slots, void **refslots,
		       const api_info_t *info, unsigned int nslots,
		       int *checked, int *skipped, int *noargs)
{
	unsigned int i;

	for (i = 0; i < nslots; i++) {
		const exception_t *exc;
		const char *name;
		unsigned int cmp_len;
		int known = 0;

		if (!slots[i])
			continue;

		name = info[i].name ? info[i].name : "?";

		// Read exactly what this wrapper delivers, never past it.
		seen_len = caller_block_size((const void *)info[i].api_caller, &known);
		if (!known) {
			printf("FAILED\n    %s[%u] (%s): api_caller is not in the caller list\n",
			       tabname, i, name);
			return 1;
		}
		if (seen_len > MAX_ARG_BYTES) {
			printf("FAILED\n    %s[%u] (%s): block of %u bytes exceeds the %d byte maximum\n",
			       tabname, i, name, seen_len, MAX_ARG_BYTES);
			return 1;
		}

		// A wrapper taking no arguments has no block; only its dispatch is
		// meaningful, and its pointer addresses the shared placeholder.
		if (seen_len == NO_ARG_BYTES)
			seen_len = 0;

		exc = find_exception(table, i);
		cmp_len = exc ? exc->prefix : seen_len;

		memset(seen_block, 0, sizeof(seen_block));
		seen_calls = 0;
		setup_globals();

		call_wrapper(slots[i]);

		if (seen_calls != 1) {
			printf("FAILED\n    %s[%u] (%s): dispatched %d times, expected 1\n",
			       tabname, i, name, seen_calls);
			return 1;
		}

		if (cmp_len && memcmp(seen_block, sentinel, cmp_len) != 0) {
			printf("FAILED\n    %s[%u] (%s): delivered block != incoming arguments\n",
			       tabname, i, name);
			return 1;
		}

		// The reference wrapper builds its block from the argument list
		// spelled out at the call site, which the shipped one no longer
		// reads. Comparing the two is what detects a wrapper whose packed
		// arguments were never simply its own parameters.
		if (refslots && refslots[i]) {
			unsigned char delivered[MAX_ARG_BYTES];
			unsigned int ref_len = seen_len;

			memcpy(delivered, seen_block, sizeof(delivered));

			memset(seen_block, 0, sizeof(seen_block));
			seen_calls = 0;
			seen_len = ref_len;
			setup_globals();

			call_wrapper(refslots[i]);

			if (seen_calls != 1) {
				printf("FAILED\n    %s[%u] (%s): reference dispatched %d times\n",
				       tabname, i, name, seen_calls);
				return 1;
			}
			// The printf-style wrappers each format into their own stack
			// buffer, so past the pass-through prefix the two copies point
			// at different strings and are not expected to agree.
			if (exc && exc->ref_prefix_only)
				ref_len = exc->prefix;
			if (ref_len && memcmp(seen_block, delivered, ref_len) != 0) {
				printf("FAILED\n    %s[%u] (%s): block differs from the declared argument list\n",
				       tabname, i, name);
				return 1;
			}
		}

		if (exc)
			(*skipped)++;
		else if (cmp_len)
			(*checked)++;
		else
			(*noargs)++;
	}
	return 0;
}

// ============================================================
// Tests
// ============================================================

static int test_all_wrappers_pass_incoming_args(void)
{
	DLL_FUNCTIONS dll_funcs, ref_dll_funcs;
	NEW_DLL_FUNCTIONS new_funcs, ref_new_funcs;
	int version;
	int checked = 0, skipped = 0, noargs = 0;

	TEST("api wrappers - every table slot delivers its incoming arguments");

	init_sentinels();
	init_caller_sizes();
	setup_globals();
	// NEW_DLL_FUNCTIONS is version-gated; pick the version that exposes
	// every slot so none is skipped here.
	meta_new_dll_functions_t::test_set_version(3);

	memset(&dll_funcs, 0, sizeof(dll_funcs));
	version = INTERFACE_VERSION;
	if (!GetEntityAPI2(&dll_funcs, &version)) {
		printf("FAILED\n    GetEntityAPI2 refused version %d\n", version);
		return 1;
	}

	memset(&new_funcs, 0, sizeof(new_funcs));
	version = NEW_DLL_FUNCTIONS_VERSION;
	if (!GetNewDLLFunctions(&new_funcs, &version)) {
		printf("FAILED\n    GetNewDLLFunctions refused version %d\n", version);
		return 1;
	}

	memset(&ref_dll_funcs, 0, sizeof(ref_dll_funcs));
	version = INTERFACE_VERSION;
	ref_GetEntityAPI2(&ref_dll_funcs, &version);
	memset(&ref_new_funcs, 0, sizeof(ref_new_funcs));
	version = NEW_DLL_FUNCTIONS_VERSION;
	ref_GetNewDLLFunctions(&ref_new_funcs, &version);

	if (check_table("DLL_FUNCTIONS", T_DLL, (void **)&dll_funcs, (void **)&ref_dll_funcs,
			(const api_info_t *)&dllapi_info,
			sizeof(dll_funcs) / sizeof(void *), &checked, &skipped, &noargs))
		return 1;

	if (check_table("NEW_DLL_FUNCTIONS", T_NEW, (void **)&new_funcs, (void **)&ref_new_funcs,
			(const api_info_t *)&newapi_info,
			sizeof(new_funcs) / sizeof(void *), &checked, &skipped, &noargs))
		return 1;

	if (check_table("enginefuncs_t", T_ENG, (void **)&meta_engfuncs, ref_meta_engfuncs,
			(const api_info_t *)&engine_info,
			sizeof(enginefuncs_t) / sizeof(void *), &checked, &skipped, &noargs))
		return 1;

	// Guard against a table that silently stopped being populated.
	if (checked < 170) {
		printf("FAILED\n    only %d wrappers exercised, expected at least 170\n", checked);
		return 1;
	}

	PASS();
	printf("    %d wrappers compared in full, %d prefix only, %d take no arguments\n",
	       checked, skipped, noargs);
	return 0;
}

// Every exception must name a slot that really is populated, so the list
// cannot rot into silently excusing nothing.
static int test_exception_list_is_live(void)
{
	DLL_FUNCTIONS dll_funcs;
	int version, i;

	TEST("api wrappers - exception list refers to live table slots");

	setup_globals();
	memset(&dll_funcs, 0, sizeof(dll_funcs));
	version = INTERFACE_VERSION;
	GetEntityAPI2(&dll_funcs, &version);

	for (i = 0; i < NUM_EXCEPTIONS; i++) {
		void **slots;
		unsigned int nslots;

		switch (exceptions[i].table) {
		case T_ENG:
			slots = (void **)&meta_engfuncs;
			nslots = sizeof(enginefuncs_t) / sizeof(void *);
			break;
		case T_DLL:
			slots = (void **)&dll_funcs;
			nslots = sizeof(DLL_FUNCTIONS) / sizeof(void *);
			break;
		default:
			continue;
		}

		ASSERT_TRUE(exceptions[i].slot < nslots);
		if (!slots[exceptions[i].slot]) {
			printf("FAILED\n    exception %d (%s) names an empty slot %u\n",
			       i, exceptions[i].why, exceptions[i].slot);
			return 1;
		}
		ASSERT_TRUE(exceptions[i].prefix > 0 && exceptions[i].prefix <= MAX_ARG_BYTES);
	}

	PASS();
	return 0;
}

int main(void)
{
	int fail = 0;

	mock_reset();

	printf("test_api_args:\n");

	fail |= test_all_wrappers_pass_incoming_args();
	fail |= test_exception_list_is_live();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
