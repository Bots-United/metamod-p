//
// metamod-p - tests for mreg.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>

#include "metamod.h"
#include "mreg.h"
#include "mlist.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// MRegCmd tests
// ============================================================

static int test_regcmd_init(void)
{
	TEST("MRegCmd::init - sets fields");
	MRegCmd cmd;
	memset(&cmd, 0xFF, sizeof(cmd));
	cmd.init(5);
	ASSERT_PTR_NULL(cmd.name);
	ASSERT_PTR_NULL(cmd.pfnCmd);
	ASSERT_INT(cmd.plugid, 0);
	ASSERT_INT(cmd.status, RG_INVALID);
	PASS();
	return 0;
}

static int call_counter;
static void test_cmd_fn(void) { call_counter++; }

static int test_regcmd_call_valid(void)
{
	TEST("MRegCmd::call - valid function executes");
	mock_reset();
	MRegCmd cmd;
	cmd.init(1);
	cmd.status = RG_VALID;
	cmd.pfnCmd = test_cmd_fn;
	cmd.name = (char *)"testcmd";
	call_counter = 0;
	ASSERT_TRUE(cmd.call() == mTRUE);
	ASSERT_INT(call_counter, 1);
	PASS();
	return 0;
}

static int test_regcmd_call_invalid_status(void)
{
	TEST("MRegCmd::call - invalid status returns mFALSE");
	mock_reset();
	MRegCmd cmd;
	cmd.init(1);
	cmd.pfnCmd = test_cmd_fn;
	// status is RG_INVALID from init
	ASSERT_TRUE(cmd.call() == mFALSE);
	ASSERT_INT(meta_errno, ME_BADREQ);
	PASS();
	return 0;
}

static int test_regcmd_call_null_fn(void)
{
	TEST("MRegCmd::call - null pfnCmd returns mFALSE");
	mock_reset();
	MRegCmd cmd;
	cmd.init(1);
	cmd.status = RG_VALID;
	cmd.pfnCmd = NULL;
	ASSERT_TRUE(cmd.call() == mFALSE);
	ASSERT_INT(meta_errno, ME_ARGUMENT);
	PASS();
	return 0;
}

// ============================================================
// MRegCmdList tests
// ============================================================

static int test_regcmdlist_constructor(void)
{
	TEST("MRegCmdList - constructor allocates list");
	MRegCmdList list;
	MRegCmd *found = list.find("nonexistent");
	ASSERT_PTR_NULL(found);
	PASS();
	return 0;
}

static int test_regcmdlist_add_find(void)
{
	TEST("MRegCmdList - add then find");
	mock_reset();
	MRegCmdList list;
	MRegCmd *cmd = list.add("test_command");
	ASSERT_PTR_NOT_NULL(cmd);
	ASSERT_STR(cmd->name, "test_command");
	MRegCmd *found = list.find("test_command");
	ASSERT_PTR_EQ(found, cmd);
	PASS();
	return 0;
}

static int test_regcmdlist_find_miss(void)
{
	TEST("MRegCmdList - find miss returns NULL");
	mock_reset();
	MRegCmdList list;
	list.add("existing");
	MRegCmd *found = list.find("missing");
	ASSERT_PTR_NULL(found);
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

static int test_regcmdlist_add_multiple(void)
{
	TEST("MRegCmdList - add multiple commands");
	mock_reset();
	MRegCmdList list;
	MRegCmd *c1 = list.add("cmd1");
	MRegCmd *c2 = list.add("cmd2");
	MRegCmd *c3 = list.add("cmd3");
	ASSERT_PTR_NOT_NULL(c1);
	ASSERT_PTR_NOT_NULL(c2);
	ASSERT_PTR_NOT_NULL(c3);
	ASSERT_STR(list.find("cmd2")->name, "cmd2");
	PASS();
	return 0;
}

static int test_regcmdlist_disable(void)
{
	TEST("MRegCmdList - disable invalidates by plugin_id");
	mock_reset();
	MRegCmdList list;
	MRegCmd *cmd = list.add("testcmd");
	cmd->plugid = 42;
	cmd->status = RG_VALID;
	list.disable(42);
	ASSERT_INT(cmd->status, RG_INVALID);
	PASS();
	return 0;
}

static int test_regcmdlist_show_by_plugin(void)
{
	TEST("MRegCmdList - show(plugin_id) lists plugin cmds");
	mock_reset();
	MRegCmdList list;
	MRegCmd *cmd = list.add("my_cmd");
	cmd->plugid = 7;
	cmd->status = RG_VALID;
	list.show(7);
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	PASS();
	return 0;
}

static int test_regcmdlist_show_all(void)
{
	TEST("MRegCmdList - show() lists all cmds");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	MRegCmdList list;
	MRegCmd *cmd = list.add("global_cmd");
	cmd->plugid = 1;
	cmd->status = RG_VALID;
	list.show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

static int test_regcmdlist_show_all_found_plugin(void)
{
	TEST("MRegCmdList - show() with found plugin shows desc");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	plugins->plist[0].status = PL_VALID;
	STRNCPY(plugins->plist[0].desc, "TestPlugin", sizeof(plugins->plist[0].desc));
	MRegCmdList list;
	MRegCmd *cmd = list.add("test_cmd");
	cmd->plugid = 1;
	cmd->status = RG_VALID;
	list.show();
	int found = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "TestPlugin"))
			found = 1;
	}
	ASSERT_TRUE(found);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

// ============================================================
// MRegCvar tests
// ============================================================

static int test_regcvar_init(void)
{
	TEST("MRegCvar::init - sets fields");
	MRegCvar cv;
	memset(&cv, 0xFF, sizeof(cv));
	cv.init(3);
	ASSERT_PTR_NULL(cv.data);
	ASSERT_INT(cv.plugid, 0);
	ASSERT_INT(cv.status, RG_INVALID);
	PASS();
	return 0;
}

static int test_regcvar_set_match(void)
{
	TEST("MRegCvar::set - matching name copies values");
	mock_reset();
	cvar_t dst_data = {(char *)"test_cv", (char *)"old", 0, 0, NULL};
	MRegCvar cv;
	cv.init(1);
	cv.data = &dst_data;
	cvar_t src = {(char *)"test_cv", (char *)"new_val", FCVAR_EXTDLL, 3.14f, NULL};
	ASSERT_TRUE(cv.set(&src) == mTRUE);
	ASSERT_FLOAT(cv.data->value, 3.14f);
	free(cv.data->string);
	PASS();
	return 0;
}

static int test_regcvar_set_mismatch(void)
{
	TEST("MRegCvar::set - mismatched name returns mFALSE");
	mock_reset();
	cvar_t dst_data = {(char *)"cv_a", (char *)"", 0, 0, NULL};
	MRegCvar cv;
	cv.init(1);
	cv.data = &dst_data;
	cvar_t src = {(char *)"cv_b", (char *)"val", 0, 0, NULL};
	ASSERT_TRUE(cv.set(&src) == mFALSE);
	ASSERT_INT(meta_errno, ME_ARGUMENT);
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList tests
// ============================================================

static int test_regcvarlist_constructor(void)
{
	TEST("MRegCvarList - constructor allocates list");
	MRegCvarList list;
	MRegCvar *found = list.find("nonexistent");
	ASSERT_PTR_NULL(found);
	PASS();
	return 0;
}

static int test_regcvarlist_add_find(void)
{
	TEST("MRegCvarList - add then find");
	mock_reset();
	MRegCvarList list;
	MRegCvar *cv = list.add("sv_test");
	ASSERT_PTR_NOT_NULL(cv);
	ASSERT_PTR_NOT_NULL(cv->data);
	ASSERT_STR(cv->data->name, "sv_test");
	MRegCvar *found = list.find("sv_test");
	ASSERT_PTR_EQ(found, cv);
	PASS();
	return 0;
}

static int test_regcvarlist_find_miss(void)
{
	TEST("MRegCvarList - find miss returns NULL");
	mock_reset();
	MRegCvarList list;
	list.add("existing_cv");
	ASSERT_PTR_NULL(list.find("missing_cv"));
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

static int test_regcvarlist_disable(void)
{
	TEST("MRegCvarList - disable invalidates by plugin_id");
	mock_reset();
	MRegCvarList list;
	MRegCvar *cv = list.add("my_cvar");
	cv->plugid = 10;
	cv->status = RG_VALID;
	list.disable(10);
	ASSERT_INT(cv->status, RG_INVALID);
	ASSERT_INT(cv->plugid, 0);
	PASS();
	return 0;
}

static int test_regcvarlist_show_by_plugin(void)
{
	TEST("MRegCvarList - show(plugin_id) lists plugin cvars");
	mock_reset();
	MRegCvarList list;
	MRegCvar *cv = list.add("pl_cvar");
	cv->plugid = 5;
	cv->data->string = strdup("hello");
	cv->data->value = 1.0f;
	list.show(5);
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	PASS();
	return 0;
}

static int test_regcvarlist_show_all(void)
{
	TEST("MRegCvarList - show() lists all cvars");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	MRegCvarList list;
	MRegCvar *cv = list.add("all_cvar");
	cv->plugid = 1;
	cv->status = RG_VALID;
	cv->data->string = strdup("val");
	cv->data->value = 2.0f;
	list.show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

static int test_regcvarlist_show_all_found_plugin(void)
{
	TEST("MRegCvarList - show() with found plugin shows desc");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	plugins->plist[0].status = PL_VALID;
	STRNCPY(plugins->plist[0].desc, "CvarPlugin", sizeof(plugins->plist[0].desc));
	MRegCvarList list;
	MRegCvar *cv = list.add("found_cvar");
	cv->plugid = 1;
	cv->status = RG_VALID;
	cv->data->string = strdup("val");
	cv->data->value = 2.0f;
	list.show();
	int found = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "CvarPlugin"))
			found = 1;
	}
	ASSERT_TRUE(found);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

// ============================================================
// MRegMsgList tests
// ============================================================

static int test_regmsglist_constructor(void)
{
	TEST("MRegMsgList - constructor initializes");
	MRegMsgList list;
	ASSERT_PTR_NULL(list.find("nonexistent"));
	PASS();
	return 0;
}

static int test_regmsglist_add_find_name(void)
{
	TEST("MRegMsgList - add then find by name");
	mock_reset();
	MRegMsgList list;
	MRegMsg *msg = list.add("SayText", 10, -1);
	ASSERT_PTR_NOT_NULL(msg);
	ASSERT_STR(msg->name, "SayText");
	ASSERT_INT(msg->msgid, 10);
	ASSERT_INT(msg->size, -1);
	MRegMsg *found = list.find("SayText");
	ASSERT_PTR_EQ(found, msg);
	PASS();
	return 0;
}

static int test_regmsglist_find_by_id(void)
{
	TEST("MRegMsgList - find by msgid");
	mock_reset();
	MRegMsgList list;
	list.add("TextMsg", 20, 0);
	MRegMsg *found = list.find(20);
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_STR(found->name, "TextMsg");
	PASS();
	return 0;
}

static int test_regmsglist_find_name_miss(void)
{
	TEST("MRegMsgList - find name miss");
	mock_reset();
	MRegMsgList list;
	list.add("SayText", 10, -1);
	ASSERT_PTR_NULL(list.find("Missing"));
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

static int test_regmsglist_find_id_miss(void)
{
	TEST("MRegMsgList - find id miss");
	mock_reset();
	MRegMsgList list;
	list.add("SayText", 10, -1);
	ASSERT_PTR_NULL(list.find(99));
	ASSERT_INT(meta_errno, ME_NOTFOUND);
	PASS();
	return 0;
}

static int test_regmsglist_show(void)
{
	TEST("MRegMsgList - show lists msgs");
	mock_reset();
	MRegMsgList list;
	list.add("SayText", 10, -1);
	list.add("TextMsg", 20, 4);
	list.show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	PASS();
	return 0;
}

static int test_regmsglist_overflow(void)
{
	TEST("MRegMsgList - add beyond MAX_REG_MSGS returns NULL");
	mock_reset();
	MRegMsgList list;
	char name[32];
	MRegMsg *msg = NULL;
	for (int i = 0; i < MAX_REG_MSGS; i++) {
		snprintf(name, sizeof(name), "msg%d", i);
		msg = list.add(name, i + 1, 0);
		ASSERT_PTR_NOT_NULL(msg);
	}
	msg = list.add("overflow", 999, 0);
	ASSERT_PTR_NULL(msg);
	ASSERT_INT(meta_errno, ME_MAXREACHED);
	PASS();
	return 0;
}

// ============================================================
// MRegCmdList grow tests
// ============================================================

static int test_regcmdlist_add_grow(void)
{
	TEST("MRegCmdList - add beyond initial size triggers grow");
	mock_reset();
	MRegCmdList list;
	char name[32];
	MRegCmd *cmd = NULL;
	for (int i = 0; i < REG_CMD_GROWSIZE + 1; i++) {
		snprintf(name, sizeof(name), "cmd%d", i);
		cmd = list.add(name);
		ASSERT_PTR_NOT_NULL(cmd);
	}
	snprintf(name, sizeof(name), "cmd%d", REG_CMD_GROWSIZE);
	MRegCmd *found = list.find(name);
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_STR(found->name, name);
	PASS();
	return 0;
}

static int test_regcmdlist_show_unloaded(void)
{
	TEST("MRegCmdList - show() with unloaded cmd shows (unloaded)");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	MRegCmdList list;
	MRegCmd *cmd = list.add("dead_cmd");
	cmd->plugid = 1;
	cmd->status = RG_INVALID;
	list.show();
	int found = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "(unloaded)"))
			found = 1;
	}
	ASSERT_TRUE(found);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

// ============================================================
// MRegCvarList grow tests
// ============================================================

static int test_regcvarlist_add_grow(void)
{
	TEST("MRegCvarList - add beyond initial size triggers grow");
	mock_reset();
	MRegCvarList list;
	char name[32];
	MRegCvar *cv = NULL;
	for (int i = 0; i < REG_CVAR_GROWSIZE + 1; i++) {
		snprintf(name, sizeof(name), "sv_cv%d", i);
		cv = list.add(name);
		ASSERT_PTR_NOT_NULL(cv);
	}
	snprintf(name, sizeof(name), "sv_cv%d", REG_CVAR_GROWSIZE);
	MRegCvar *found = list.find(name);
	ASSERT_PTR_NOT_NULL(found);
	ASSERT_STR(found->data->name, name);
	PASS();
	return 0;
}

static int test_regcvarlist_show_unloaded(void)
{
	TEST("MRegCvarList - show() with unloaded cvar shows (unloaded)");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	MRegCvarList list;
	MRegCvar *cv = list.add("dead_cvar");
	cv->plugid = 1;
	cv->status = RG_INVALID;
	cv->data->string = strdup("x");
	cv->data->value = 0;
	list.show();
	int found = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "(unloaded)"))
			found = 1;
	}
	ASSERT_TRUE(found);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

// ============================================================
// show_for skip tests
// ============================================================

static int test_regcmdlist_show_for_skip(void)
{
	TEST("MRegCmdList - show_for skips non-matching plugid");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	plugins->plist[0].status = PL_RUNNING;
	STRNCPY(plugins->plist[0].desc, "Plug1", sizeof(plugins->plist[0].desc));
	plugins->endlist = 1;
	MRegCmdList list;
	MRegCmd *cmd1 = list.add("other_cmd");
	cmd1->plugid = 2;
	cmd1->status = RG_VALID;
	MRegCmd *cmd2 = list.add("my_cmd");
	cmd2->plugid = 1;
	cmd2->status = RG_VALID;
	list.show(1);
	int found_my = 0, found_other = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "my_cmd"))
			found_my = 1;
		if (strstr(mock_get_server_print_msg(i), "other_cmd"))
			found_other = 1;
	}
	ASSERT_TRUE(found_my);
	ASSERT_FALSE(found_other);
	Plugins = NULL;
	delete plugins;
	PASS();
	return 0;
}

static int test_regcvarlist_show_for_skip(void)
{
	TEST("MRegCvarList - show_for skips non-matching plugid");
	mock_reset();
	MPluginList *plugins = new MPluginList("/dev/null");
	Plugins = plugins;
	plugins->plist[0].status = PL_RUNNING;
	STRNCPY(plugins->plist[0].desc, "Plug1", sizeof(plugins->plist[0].desc));
	plugins->endlist = 1;
	MRegCvarList list;
	MRegCvar *cv1 = list.add("sv_other");
	cv1->plugid = 2;
	cv1->status = RG_VALID;
	cv1->data->string = strdup("x");
	MRegCvar *cv2 = list.add("sv_mine");
	cv2->plugid = 1;
	cv2->status = RG_VALID;
	cv2->data->string = strdup("y");
	list.show(1);
	int found_mine = 0, found_other = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "sv_mine"))
			found_mine = 1;
		if (strstr(mock_get_server_print_msg(i), "sv_other"))
			found_other = 1;
	}
	ASSERT_TRUE(found_mine);
	ASSERT_FALSE(found_other);
	Plugins = NULL;
	delete plugins;
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

	printf("test_mreg:\n");

	fail |= test_regcmd_init();
	fail |= test_regcmd_call_valid();
	fail |= test_regcmd_call_invalid_status();
	fail |= test_regcmd_call_null_fn();

	fail |= test_regcmdlist_constructor();
	fail |= test_regcmdlist_add_find();
	fail |= test_regcmdlist_find_miss();
	fail |= test_regcmdlist_add_multiple();
	fail |= test_regcmdlist_disable();
	fail |= test_regcmdlist_show_by_plugin();
	fail |= test_regcmdlist_show_all();
	fail |= test_regcmdlist_add_grow();
	fail |= test_regcmdlist_show_unloaded();
	fail |= test_regcmdlist_show_all_found_plugin();

	fail |= test_regcvar_init();
	fail |= test_regcvar_set_match();
	fail |= test_regcvar_set_mismatch();

	fail |= test_regcvarlist_constructor();
	fail |= test_regcvarlist_add_find();
	fail |= test_regcvarlist_find_miss();
	fail |= test_regcvarlist_disable();
	fail |= test_regcvarlist_show_by_plugin();
	fail |= test_regcvarlist_show_all();
	fail |= test_regcvarlist_add_grow();
	fail |= test_regcvarlist_show_unloaded();
	fail |= test_regcvarlist_show_all_found_plugin();

	fail |= test_regmsglist_constructor();
	fail |= test_regmsglist_add_find_name();
	fail |= test_regmsglist_find_by_id();
	fail |= test_regmsglist_find_name_miss();
	fail |= test_regmsglist_find_id_miss();
	fail |= test_regmsglist_show();
	fail |= test_regmsglist_overflow();

	fail |= test_regcmdlist_show_for_skip();
	fail |= test_regcvarlist_show_for_skip();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
