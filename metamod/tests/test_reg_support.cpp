//
// metamod-p - tests for reg_support.cpp
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <extdll.h>

#include "metamod.h"
#include "reg_support.h"
#include "mreg.h"
#include "mlist.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// meta_command_handler tests
// ============================================================

static int handler_called;
static void test_handler_fn(void) { handler_called++; }

static int test_meta_command_handler_valid(void)
{
	TEST("meta_command_handler - valid cmd calls function");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	MRegCmd *cmd = cmds.add("test_cmd");
	cmd->pfnCmd = test_handler_fn;
	cmd->status = RG_VALID;

	const char *argv[] = {"test_cmd"};
	mock_set_cmd_args(1, argv, "");
	handler_called = 0;
	meta_command_handler();
	ASSERT_INT(handler_called, 1);

	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_meta_command_handler_null_cmd(void)
{
	TEST("meta_command_handler - NULL cmd warns");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	const char *argv[] = {NULL};
	mock_set_cmd_args(1, argv, "");
	meta_command_handler();
	ASSERT_TRUE(mock_get_alert_count() > 0);

	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_meta_command_handler_unknown(void)
{
	TEST("meta_command_handler - unknown cmd warns");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	const char *argv[] = {"unknown_cmd"};
	mock_set_cmd_args(1, argv, "");
	meta_command_handler();
	ASSERT_TRUE(mock_get_alert_count() > 0);

	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_meta_command_handler_unloaded(void)
{
	TEST("meta_command_handler - unloaded plugin prints message");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	MRegCmd *cmd = cmds.add("dead_cmd");
	cmd->pfnCmd = test_handler_fn;
	cmd->status = RG_INVALID;

	const char *argv[] = {"dead_cmd"};
	mock_set_cmd_args(1, argv, "");
	meta_command_handler();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "unavailable");

	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

// ============================================================
// meta_AddServerCommand tests
// ============================================================

static int test_meta_add_server_command_new(void)
{
	TEST("meta_AddServerCommand - registers new command");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	meta_AddServerCommand((char *)"new_cmd", test_handler_fn);
	MRegCmd *found = cmds.find("new_cmd");
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_INT(found->status, RG_VALID);
	ASSERT_PTR_EQ((void *)found->pfnCmd, (void *)test_handler_fn);

	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_meta_add_server_command_reregister(void)
{
	TEST("meta_AddServerCommand - re-register updates function");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	meta_AddServerCommand((char *)"re_cmd", test_handler_fn);
	meta_AddServerCommand((char *)"re_cmd", test_handler_fn);
	MRegCmd *found = cmds.find("re_cmd");
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_INT(found->status, RG_VALID);

	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

// ============================================================
// meta_CVarRegister tests
// ============================================================

static int test_meta_cvar_register_new(void)
{
	TEST("meta_CVarRegister - registers new cvar");
	mock_reset();
	MRegCvarList cvars;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCvars = &cvars;
	Plugins = plugins;

	cvar_t cv = {(char *)"sv_test", (char *)"42", FCVAR_EXTDLL, 42.0f, NULL};
	meta_CVarRegister(&cv);
	MRegCvar *found = cvars.find("sv_test");
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_INT(found->status, RG_VALID);

	RegCvars = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

static int test_meta_cvar_register_reregister(void)
{
	TEST("meta_CVarRegister - re-register keeps existing value");
	mock_reset();
	MRegCvarList cvars;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCvars = &cvars;
	Plugins = plugins;

	cvar_t cv1 = {(char *)"sv_retest", (char *)"1", 0, 1.0f, NULL};
	meta_CVarRegister(&cv1);
	cvar_t cv2 = {(char *)"sv_retest", (char *)"2", 0, 2.0f, NULL};
	meta_CVarRegister(&cv2);
	MRegCvar *found = cvars.find("sv_retest");
	ASSERT_PTR_NOT_NULL(found);

	RegCvars = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

// ============================================================
// meta_RegUserMsg tests
// ============================================================

static int test_meta_reg_user_msg(void)
{
	TEST("meta_RegUserMsg - passthrough to engine");
	mock_reset();
	int ret = meta_RegUserMsg("TestMsg", 10);
	ASSERT_INT(ret, 0);
	PASS();
	return 0;
}

// ============================================================
// meta_QueryClientCvarValue tests
// ============================================================

static edict_t test_edict;

static int qccv_called;
static void mock_pfnQueryClientCvarValue_test(const edict_t *player, const char *cvarName)
{
	(void)player; (void)cvarName;
	qccv_called++;
}

static int test_meta_query_client_cvar_with_fn(void)
{
	TEST("meta_QueryClientCvarValue - calls engine fn");
	mock_reset();
	mock_set_index_of_edict(1);
	memset(&test_edict, 0, sizeof(test_edict));
	g_engfuncs.pfnQueryClientCvarValue = mock_pfnQueryClientCvarValue_test;
	qccv_called = 0;
	meta_QueryClientCvarValue(&test_edict, "rate");
	ASSERT_INT(qccv_called, 1);
	ASSERT_STR(g_Players.is_querying_cvar(&test_edict), "rate");
	PASS();
	return 0;
}

static int test_meta_query_client_cvar_without_fn(void)
{
	TEST("meta_QueryClientCvarValue - no engine fn still stores");
	mock_reset();
	mock_set_index_of_edict(2);
	memset(&test_edict, 0, sizeof(test_edict));
	g_engfuncs.pfnQueryClientCvarValue = NULL;
	meta_QueryClientCvarValue(&test_edict, "cl_cmdrate");
	ASSERT_STR(g_Players.is_querying_cvar(&test_edict), "cl_cmdrate");
	PASS();
	return 0;
}

// ============================================================
// meta_AddServerCommand with iplug found (lines 155-156)
// ============================================================

static int test_meta_add_server_command_with_iplug(void)
{
	TEST("meta_AddServerCommand - sets plugid when plugin found via memloc");
	mock_reset();
	MRegCmdList cmds;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCmds = &cmds;
	Plugins = plugins;

	// Load fake_mm_plugin.so and get a function pointer from it
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	char sopath[PATH_MAX];
	snprintf(sopath, sizeof(sopath), "%s/fake_mm_plugin.so", cwd);
	void *handle = dlopen(sopath, RTLD_NOW);
	ASSERT_PTR_NOT_NULL(handle);

	void *sym = dlsym(handle, "GiveFnptrsToDll");
	ASSERT_PTR_NOT_NULL(sym);

	// Add a plugin to the list with matching pathname
	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "fake_mm_plugin.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.pathname, sopath, sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = plugins->add(&temp);
	ASSERT_PTR_NOT_NULL(added);
	added->status = PL_RUNNING;
	added->index = 1;

	// Register command using function from the plugin .so
	meta_AddServerCommand((char *)"plugin_cmd", (void (*)(void))sym);
	MRegCmd *found = cmds.find("plugin_cmd");
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_INT(found->plugid, 1);

	dlclose(handle);
	RegCmds = NULL;
	delete plugins;
	Plugins = NULL;
	PASS();
	return 0;
}

// ============================================================
// meta_CVarRegister with iplug found (lines 210-211)
// ============================================================

static int test_meta_cvar_register_with_iplug(void)
{
	TEST("meta_CVarRegister - sets plugid when plugin found via memloc");
	mock_reset();
	MRegCvarList cvars;
	MPluginList *plugins = new MPluginList("/dev/null");
	RegCvars = &cvars;
	Plugins = plugins;

	// Load fake_mm_plugin.so and get the exported cvar inside it
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	char sopath[PATH_MAX];
	snprintf(sopath, sizeof(sopath), "%s/fake_mm_plugin.so", cwd);
	void *handle = dlopen(sopath, RTLD_NOW);
	ASSERT_PTR_NOT_NULL(handle);

	cvar_t *plugin_cv = (cvar_t *)dlsym(handle, "fake_plugin_cvar");
	ASSERT_PTR_NOT_NULL(plugin_cv);

	// Add a plugin to the list with matching pathname
	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "fake_mm_plugin.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.pathname, sopath, sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = plugins->add(&temp);
	ASSERT_PTR_NOT_NULL(added);
	added->status = PL_RUNNING;
	added->index = 2;

	// Register the cvar that lives in the plugin .so
	// find_memloc will use dladdr on plugin_cv and find the .so
	meta_CVarRegister(plugin_cv);
	MRegCvar *found = cvars.find("sv_fake_plugin");
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_INT(found->plugid, 2);

	dlclose(handle);
	RegCvars = NULL;
	delete plugins;
	Plugins = NULL;
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

	printf("test_reg_support:\n");

	fail |= test_meta_command_handler_valid();
	fail |= test_meta_command_handler_null_cmd();
	fail |= test_meta_command_handler_unknown();
	fail |= test_meta_command_handler_unloaded();

	fail |= test_meta_add_server_command_new();
	fail |= test_meta_add_server_command_reregister();

	fail |= test_meta_cvar_register_new();
	fail |= test_meta_cvar_register_reregister();

	fail |= test_meta_reg_user_msg();

	fail |= test_meta_query_client_cvar_with_fn();
	fail |= test_meta_query_client_cvar_without_fn();

	fail |= test_meta_add_server_command_with_iplug();
	fail |= test_meta_cvar_register_with_iplug();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
