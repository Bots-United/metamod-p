//
// metamod-p - stub definitions for symbols needed when the real .o
// files are not linked. All symbols are weak so that linking the real
// .o alongside stubs.o simply overrides the stub.
//

#include <extdll.h>

#include "metamod.h"
#include "api_info.h"
#include "api_hook.h"
#include "mutil.h"
#include "engine_api.h"
#include "commands_meta.h"

extern __attribute__((weak)) const dllapi_info_t dllapi_info = {};
extern __attribute__((weak)) const newapi_info_t newapi_info = {};
extern __attribute__((weak)) const engine_info_t engine_info = {};
__attribute__((weak)) gamedll_funcs_t GameDLL_funcs = {};
__attribute__((weak)) mutil_funcs_t MetaUtilFunctions = {};
__attribute__((weak)) enginefuncs_t *Engine_funcs = NULL;
// Storage only, under the real symbol name, for tests that do not link
// engine_api.o. A weak object of the real type would run its default
// constructor even where engine_api.o's strong definition wins, and which
// of the two runs last is unspecified: with LTO it was the default one,
// which zeroed the table engine_api.o had just filled.
__attribute__((weak, aligned(16)))
char meta_engfuncs_stub_storage[sizeof(meta_enginefuncs_t)] __asm__("meta_engfuncs");

__attribute__((weak)) void DLLINTERNAL main_hook_function_void(unsigned int, enum_api_t, unsigned int, const void *) {}
__attribute__((weak)) void * DLLINTERNAL main_hook_function(const class_ret_t, unsigned int, enum_api_t, unsigned int, const void *) { return NULL; }

__attribute__((weak)) void DLLINTERNAL client_meta(edict_t *) {}

__attribute__((weak)) const char * DLLINTERNAL META_UTIL_VarArgs(const char *, ...)
{
	static char buf[] = "";
	return buf;
}
