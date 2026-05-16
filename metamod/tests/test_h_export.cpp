//
// metamod-p - tests for h_export.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "h_export.h"

#include "engine_mock.h"
#include "test_common.h"

static int g_metamod_startup_retval;

int metamod_startup(void)
{
	return g_metamod_startup_retval;
}

static void stub_alert(ALERT_TYPE, char *, ...) {}
static float stub_cvar_get_float(const char *) { return 0; }
static const char *stub_cvar_get_string(const char *) { return ""; }

static void prepare_engfuncs(enginefuncs_t *ef)
{
	memset(ef, 0, sizeof(*ef));
	ef->pfnAlertMessage = stub_alert;
	ef->pfnCVarGetFloat = stub_cvar_get_float;
	ef->pfnCVarGetString = stub_cvar_get_string;
}

// ============================================================
// GiveFnptrsToDll tests
// ============================================================

static int test_give_sets_gpglobals(void)
{
	TEST("GiveFnptrsToDll - sets gpGlobals");
	mock_reset();
	g_metamod_startup_retval = 1;
	metamod_not_loaded = 0;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(gpGlobals == &test_globals);
	PASS();
	return 0;
}

static int test_give_sets_engine_globals(void)
{
	TEST("GiveFnptrsToDll - sets Engine.globals");
	mock_reset();
	g_metamod_startup_retval = 1;
	metamod_not_loaded = 0;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(Engine.globals == &test_globals);
	PASS();
	return 0;
}

static int test_give_sets_engine_funcs(void)
{
	TEST("GiveFnptrsToDll - sets Engine_funcs to g_engfuncs");
	mock_reset();
	g_metamod_startup_retval = 1;
	metamod_not_loaded = 0;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(Engine_funcs == (enginefuncs_t *)&g_engfuncs);
	PASS();
	return 0;
}

static int test_give_calls_get_module_handle(void)
{
	TEST("GiveFnptrsToDll - calls get_module_handle_of_memptr without crash");
	mock_reset();
	g_metamod_startup_retval = 1;
	metamod_not_loaded = 0;
	metamod_handle = NULL;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(1);
	PASS();
	return 0;
}

static int test_give_startup_success(void)
{
	TEST("GiveFnptrsToDll - metamod_startup success keeps not_loaded=0");
	mock_reset();
	g_metamod_startup_retval = 1;
	metamod_not_loaded = 0;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(metamod_not_loaded == 0);
	PASS();
	return 0;
}

static int test_give_startup_failure(void)
{
	TEST("GiveFnptrsToDll - metamod_startup failure sets not_loaded=1");
	mock_reset();
	g_metamod_startup_retval = 0;
	metamod_not_loaded = 0;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(metamod_not_loaded == 1);
	PASS();
	return 0;
}

static int test_give_copies_engine_funcs(void)
{
	TEST("GiveFnptrsToDll - copies engine funcs from passed table");
	mock_reset();
	g_metamod_startup_retval = 1;
	metamod_not_loaded = 0;

	globalvars_t test_globals;
	memset(&test_globals, 0, sizeof(test_globals));
	enginefuncs_t test_engfuncs;
	prepare_engfuncs(&test_engfuncs);

	GiveFnptrsToDll(&test_engfuncs, &test_globals);

	ASSERT_TRUE(g_engfuncs.pfnAlertMessage == stub_alert);
	ASSERT_TRUE(g_engfuncs.pfnCVarGetFloat == stub_cvar_get_float);
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

	printf("test_h_export:\n");

	fail |= test_give_sets_gpglobals();
	fail |= test_give_sets_engine_globals();
	fail |= test_give_sets_engine_funcs();
	fail |= test_give_calls_get_module_handle();
	fail |= test_give_startup_success();
	fail |= test_give_startup_failure();
	fail |= test_give_copies_engine_funcs();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
