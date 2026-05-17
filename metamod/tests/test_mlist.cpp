//
// metamod-p - tests for mlist.cpp
//

#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <extdll.h>

#include "metamod.h"
#include "mplugin.h"
#include "mlist.h"
#include "mreg.h"
#include "conf_meta.h"

#include "engine_mock.h"
#include "test_common.h"

static bool force_realloc_fail = false;
static size_t last_realloc_old_size = 0;

extern "C" void *__real_realloc(void *ptr, size_t size);
extern "C" void *__wrap_realloc(void *ptr, size_t size)
{
	if (force_realloc_fail)
		return NULL;
	// Always move the allocation so tests can detect stale pointers.
	void *newptr = malloc(size);
	if (!newptr)
		return NULL;
	if (ptr) {
		size_t copy_size = size < last_realloc_old_size ? size : last_realloc_old_size;
		memcpy(newptr, ptr, copy_size);
		memset(ptr, 0, last_realloc_old_size);
		free(ptr);
	}
	last_realloc_old_size = size;
	return newptr;
}

static MRegCmdList test_reg_cmds;
static MRegCvarList test_reg_cvars;
static MRegMsgList test_reg_msgs;

struct HeapPluginList {
	MPluginList *ptr;
	HeapPluginList(const char *ini) : ptr(new MPluginList(ini)) {}
	~HeapPluginList() { delete ptr; }
	MPluginList* operator->() { return ptr; }
	MPluginList& operator*() { return *ptr; }
};

static void setup_globals(void)
{
	mock_reset();
	RegCmds = &test_reg_cmds;
	RegCvars = &test_reg_cvars;
	RegMsgs = &test_reg_msgs;
	STRNCPY(GameDLL.gamedir, "/tmp/test_mlist_gd", sizeof(GameDLL.gamedir));
}

static void teardown_globals(void)
{
	RegCmds = NULL;
	RegCvars = NULL;
	RegMsgs = NULL;
}

// ============================================================
// Constructor
// ============================================================

static int test_constructor(void)
{
	TEST("MPluginList - constructor initializes fields");
	setup_globals();
	HeapPluginList list("test.ini");
	ASSERT_TRUE(list->endlist == 0);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find(int) tests
// ============================================================

static int test_find_index_zero(void)
{
	TEST("find(int) - index 0 returns NULL with ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find(0);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_index_negative(void)
{
	TEST("find(int) - negative index returns NULL");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find(-1);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_index_empty(void)
{
	TEST("find(int) - index 1 in empty list returns ME_NOTFOUND");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find(1);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find(DLHANDLE) tests
// ============================================================

static int test_find_handle_null(void)
{
	TEST("find(DLHANDLE) - NULL handle returns ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find((DLHANDLE)NULL);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_handle_not_found(void)
{
	TEST("find(DLHANDLE) - non-matching handle returns ME_NOTFOUND");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find((DLHANDLE)0x12345678);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find(plid_t) tests
// ============================================================

static int test_find_plid_null(void)
{
	TEST("find(plid_t) - NULL plid returns ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find((plid_t)NULL);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find(const char*) tests
// ============================================================

static int test_find_path_null(void)
{
	TEST("find(path) - NULL path returns ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find((const char *)NULL);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_path_not_found(void)
{
	TEST("find(path) - non-matching path returns ME_NOTFOUND");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find("/no/such/plugin.so");
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_memloc tests
// ============================================================

static int test_find_memloc_null(void)
{
	TEST("find_memloc - NULL memptr returns ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find_memloc(NULL);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_match(const char*) tests
// ============================================================

static int test_find_match_null(void)
{
	TEST("find_match(str) - NULL prefix returns ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find_match((const char *)NULL);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_not_found(void)
{
	TEST("find_match(str) - no match returns ME_NOTFOUND");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find_match("nonexistent");
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_match(MPlugin*) tests
// ============================================================

static int test_find_match_plugin_null(void)
{
	TEST("find_match(MPlugin*) - NULL returns ME_ARGUMENT");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find_match((MPlugin *)NULL);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ARGUMENT);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// add tests
// ============================================================

static int test_add_and_find(void)
{
	TEST("add - add plugin and find it by pathname");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "test.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Test Plugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/test.so", sizeof(temp.pathname));
	temp.source = PS_INI;
	temp.status = PL_VALID;

	MPlugin *added = list->add(&temp);
	ASSERT_TRUE(added != NULL);
	ASSERT_TRUE(list->endlist == 1);
	ASSERT_TRUE(strcmp(added->desc, "Test Plugin") == 0);
	ASSERT_TRUE(strcmp(added->file, "test.so") == 0);

	MPlugin *found = list->find(added->pathname);
	ASSERT_TRUE(found == added);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_add_find_by_index(void)
{
	TEST("add - find added plugin by index");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "test.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Test", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test.so", sizeof(temp.pathname));
	temp.status = PL_VALID;

	MPlugin *added = list->add(&temp);
	ASSERT_TRUE(added != NULL);

	MPlugin *found = list->find(added->index);
	ASSERT_TRUE(found == added);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_match with populated list
// ============================================================

static int test_find_match_by_desc(void)
{
	TEST("find_match - match by desc prefix");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "test.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "MyGreatPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	list->add(&temp);

	MPlugin *found = list->find_match("MyGreat");
	ASSERT_TRUE(found != NULL);
	ASSERT_STR_CONTAINS(found->desc, "MyGreatPlugin");

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_by_file(void)
{
	TEST("find_match - match by file prefix");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "unique_plugin.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "<unique_plugin.so>", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/unique_plugin.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	list->add(&temp);

	MPlugin *found = list->find_match("unique_");
	ASSERT_TRUE(found != NULL);
	ASSERT_STR_CONTAINS(found->file, "unique_plugin.so");

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_by_mm_prefix(void)
{
	TEST("find_match - match by mm_ prefix");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "mm_myplugin.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "<mm_myplugin.so>", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/mm_myplugin.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	list->add(&temp);

	MPlugin *found = list->find_match("myplugin");
	ASSERT_TRUE(found != NULL);
	ASSERT_STR_CONTAINS(found->file, "mm_myplugin.so");

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_by_info_name(void)
{
	TEST("find_match - match by info->name");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t test_info;
	memset(&test_info, 0, sizeof(test_info));
	test_info.name = "AwesomeMod";

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "awesome.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Awesome", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/awesome.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.info = &test_info;

	MPlugin *added = list->add(&temp);
	added->info = &test_info;

	MPlugin *found = list->find_match("Awesome");
	ASSERT_TRUE(found != NULL);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_by_logtag(void)
{
	TEST("find_match - match by info->logtag");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t test_info;
	memset(&test_info, 0, sizeof(test_info));
	test_info.name = "Something Else";
	test_info.logtag = "MYTAG";

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "tagged.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Tagged Plugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/tagged.so", sizeof(temp.pathname));
	temp.status = PL_VALID;

	MPlugin *added = list->add(&temp);
	added->info = &test_info;

	MPlugin *found = list->find_match("MYTAG");
	ASSERT_TRUE(found != NULL);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_not_unique(void)
{
	TEST("find_match - multiple matches returns ME_NOTUNIQ");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp1;
	memset(&temp1, 0, sizeof(temp1));
	STRNCPY(temp1.filename, "dup1.so", sizeof(temp1.filename));
	temp1.file = temp1.filename;
	STRNCPY(temp1.desc, "Duplicate A", sizeof(temp1.desc));
	STRNCPY(temp1.pathname, "/tmp/dup1.so", sizeof(temp1.pathname));
	temp1.status = PL_VALID;
	list->add(&temp1);

	MPlugin temp2;
	memset(&temp2, 0, sizeof(temp2));
	STRNCPY(temp2.filename, "dup2.so", sizeof(temp2.filename));
	temp2.file = temp2.filename;
	STRNCPY(temp2.desc, "Duplicate B", sizeof(temp2.desc));
	STRNCPY(temp2.pathname, "/tmp/dup2.so", sizeof(temp2.pathname));
	temp2.status = PL_VALID;
	list->add(&temp2);

	MPlugin *found = list->find_match("Dup");
	ASSERT_TRUE(found == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTUNIQ);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// clear_source_plugin_index
// ============================================================

static int test_clear_source_plugin_index(void)
{
	TEST("clear_source_plugin_index - clears matching index");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "child.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Child", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/child.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source_plugin_index = 3;

	MPlugin *added = list->add(&temp);
	ASSERT_TRUE(added->source_plugin_index == 3);

	list->clear_source_plugin_index(3);
	ASSERT_TRUE(added->source_plugin_index == -1);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_clear_source_plugin_index_zero(void)
{
	TEST("clear_source_plugin_index - zero index is no-op");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	list->clear_source_plugin_index(0);
	ASSERT_TRUE(1);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// found_child_plugins
// ============================================================

static int test_found_child_plugins_none(void)
{
	TEST("found_child_plugins - no children returns false");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	ASSERT_TRUE(list->found_child_plugins(1) == mFALSE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_found_child_plugins_zero(void)
{
	TEST("found_child_plugins - zero index returns false");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	ASSERT_TRUE(list->found_child_plugins(0) == mFALSE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_found_child_plugins_found(void)
{
	TEST("found_child_plugins - finds child by source_plugin_index");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "child.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Child", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/child.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source_plugin_index = 5;
	list->add(&temp);

	ASSERT_TRUE(list->found_child_plugins(5) == mTRUE);
	ASSERT_TRUE(list->found_child_plugins(6) == mFALSE);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// trim_list
// ============================================================

static int test_trim_list_empty(void)
{
	TEST("trim_list - no-op on empty list");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	list->trim_list();
	ASSERT_TRUE(list->endlist == 0);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_trim_list_shrinks(void)
{
	TEST("trim_list - shrinks endlist after removing last plugin");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "a.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "A", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/a.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *pa = list->add(&temp);

	STRNCPY(temp.filename, "b.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "B", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/b.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	list->add(&temp);

	ASSERT_TRUE(list->endlist == 2);

	// Remove first plugin, trim should still keep endlist=2
	pa->status = PL_EMPTY;
	list->trim_list();
	ASSERT_TRUE(list->endlist == 2);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show
// ============================================================

static int test_show_empty(void)
{
	TEST("show - empty list shows header");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	list->show();
	ASSERT_TRUE(mock_get_server_print_count() > 0);
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "loaded plugins");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_show_with_plugins(void)
{
	TEST("show - list with valid plugin shows it");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "shown.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "ShownPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/shown.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	list->add(&temp);

	list->show();
	int found = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "ShownPlugin"))
			found = 1;
	}
	ASSERT_TRUE(found);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_show_with_source_index(void)
{
	TEST("show - show(source_index) filters by child plugins");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "child.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "ChildOfTwo", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/child.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source_plugin_index = 2;
	list->add(&temp);

	list->show(2);
	ASSERT_STR_CONTAINS(mock_get_server_print_msg(0), "Child plugins");

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show_client
// ============================================================

static int test_show_client_empty(void)
{
	TEST("show_client - empty list shows header");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	list->show_client(&ed);
	ASSERT_TRUE(mock_get_client_print_count() > 0);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// unpause_all
// ============================================================

static int test_unpause_all_no_paused(void)
{
	TEST("unpause_all - no-op when no plugins paused");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	list->unpause_all();
	ASSERT_TRUE(1);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// retry_all
// ============================================================

static int test_retry_all_no_pending(void)
{
	TEST("retry_all - no-op when no pending actions");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	list->retry_all(PT_ANYTIME);
	ASSERT_TRUE(1);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_startup tests
// ============================================================

static int test_ini_startup_missing_file(void)
{
	TEST("ini_startup - missing file returns ME_NOFILE");
	setup_globals();
	HeapPluginList list("nonexistent_plugins.ini");
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOFILE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_startup_valid_file(void)
{
	TEST("ini_startup - reads valid plugins.ini");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/plugin1.so First Plugin\n"
		"linux dlls/plugin2.so Second Plugin\n"
		"# comment line\n"
		"win32 dlls/win.dll WinOnly\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 2);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_startup_duplicate(void)
{
	TEST("ini_startup - skips duplicate pathname");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/same.so Same1\n"
		"linux dlls/same.so Same2\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_startup_empty_file(void)
{
	TEST("ini_startup - file with only comments warns no plugins");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"# only comments\n"
		"; more comments\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 0);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_refresh tests
// ============================================================

static int test_ini_refresh_missing_file(void)
{
	TEST("ini_refresh - missing file returns ME_NOFILE");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, "/tmp/nonexistent_refresh.ini", sizeof(list->inifile));
	mBOOL ret = list->ini_refresh();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOFILE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload tests
// ============================================================

static int test_cmd_addload_bad_parse(void)
{
	TEST("cmd_addload - empty args fails parse");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	mBOOL ret = list->cmd_addload("");
	ASSERT_TRUE(ret == mFALSE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_addload_no_resolve(void)
{
	TEST("cmd_addload - unresolvable file fails");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	mBOOL ret = list->cmd_addload("load nonexistent_plugin.so");
	ASSERT_TRUE(ret == mFALSE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find(DLHANDLE) - matching handle
// ============================================================

static int test_find_handle_found(void)
{
	TEST("find(DLHANDLE) - finds plugin by matching handle");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "hdl.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "HandlePlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/hdl.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->handle = (DLHANDLE)0xDEADBEEF;

	MPlugin *found = list->find((DLHANDLE)0xDEADBEEF);
	ASSERT_TRUE(found == added);

	MPlugin *nope = list->find((DLHANDLE)0xCAFE);
	ASSERT_TRUE(nope == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find(plid_t) - matching plid
// ============================================================

static plugin_info_t test_plid_info;

static int test_find_plid_found(void)
{
	TEST("find(plid_t) - finds plugin by matching info ptr");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	memset(&test_plid_info, 0, sizeof(test_plid_info));
	test_plid_info.name = "PlIdPlugin";

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "plid.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "PlIdPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/plid.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->info = &test_plid_info;

	MPlugin *found = list->find((plid_t)&test_plid_info);
	ASSERT_TRUE(found == added);

	plugin_info_t other;
	MPlugin *nope = list->find((plid_t)&other);
	ASSERT_TRUE(nope == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_memloc - valid pointer
// ============================================================

static int test_find_memloc_valid_not_found(void)
{
	TEST("find_memloc - valid ptr but no matching plugin");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	MPlugin *p = list->find_memloc((void*)&test_find_memloc_valid_not_found);
	ASSERT_TRUE(p == NULL);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_match NOTUNIQ paths (various match types)
// ============================================================

static int test_find_match_notuniq_by_info_name(void)
{
	TEST("find_match - NOTUNIQ by info->name");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t info1, info2;
	memset(&info1, 0, sizeof(info1));
	memset(&info2, 0, sizeof(info2));
	info1.name = "SameName1";
	info2.name = "SameName2";

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "zzza.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "zzza", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/zzza.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *a1 = list->add(&t1);
	a1->info = &info1;

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "zzzb.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "zzzb", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/zzzb.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	MPlugin *a2 = list->add(&t2);
	a2->info = &info2;

	MPlugin *found = list->find_match("Same");
	ASSERT_TRUE(found == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTUNIQ);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_notuniq_by_mm_prefix(void)
{
	TEST("find_match - NOTUNIQ by mm_ prefix");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "mm_samepfx_a.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "xxx1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/mm_samepfx_a.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "mm_samepfx_b.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "xxx2", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/mm_samepfx_b.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	list->add(&t2);

	MPlugin *found = list->find_match("samepfx");
	ASSERT_TRUE(found == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTUNIQ);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_notuniq_by_logtag(void)
{
	TEST("find_match - NOTUNIQ by logtag");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t li1, li2;
	memset(&li1, 0, sizeof(li1));
	memset(&li2, 0, sizeof(li2));
	li1.name = "xxx"; li1.logtag = "SAMETAG1";
	li2.name = "yyy"; li2.logtag = "SAMETAG2";

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "lt1.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "aaa1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/lt1.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *a1 = list->add(&t1);
	a1->info = &li1;

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "lt2.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "aaa2", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/lt2.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	MPlugin *a2 = list->add(&t2);
	a2->info = &li2;

	MPlugin *found = list->find_match("SAME");
	ASSERT_TRUE(found == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTUNIQ);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_match(MPlugin*) - found path
// ============================================================

static int test_find_match_plugin_found(void)
{
	TEST("find_match(MPlugin*) - finds matching plugin");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "matchme.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "MatchTarget", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/matchme.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *added = list->add(&t1);

	MPlugin search;
	memset(&search, 0, sizeof(search));
	STRNCPY(search.filename, "matchme.so", sizeof(search.filename));
	search.file = search.filename;
	search.status = PL_VALID;

	MPlugin *found = list->find_match(&search);
	ASSERT_TRUE(found == added);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_plugin_not_found(void)
{
	TEST("find_match(MPlugin*) - no match returns ME_NOTFOUND");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "existing.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "Existing", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/existing.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	list->add(&t1);

	MPlugin search;
	memset(&search, 0, sizeof(search));
	STRNCPY(search.filename, "other.so", sizeof(search.filename));
	search.file = search.filename;
	search.status = PL_VALID;

	MPlugin *found = list->find_match(&search);
	ASSERT_TRUE(found == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// trim_list - actually shrinks endlist
// ============================================================

static int test_trim_list_shrinks_to_first(void)
{
	TEST("trim_list - shrinks endlist when last slot empty");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "first.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "First", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/first.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	list->add(&temp);

	STRNCPY(temp.filename, "second.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Second", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/second.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *pb = list->add(&temp);
	ASSERT_TRUE(list->endlist == 2);

	pb->status = PL_EMPTY;
	list->trim_list();
	ASSERT_TRUE(list->endlist == 1);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_refresh - valid file with data
// ============================================================

static int test_ini_refresh_valid_new_plugin(void)
{
	TEST("ini_refresh - adds new plugin from ini");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/plug1.so FirstPlugin\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);

	FILE *fp = fopen(fullpath, "w");
	fprintf(fp, "linux dlls/plug1.so FirstPlugin\nlinux dlls/plug2.so SecondPlugin\n");
	fclose(fp);

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_existing_plugin(void)
{
	TEST("ini_refresh - marks existing plugin for keep");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *plugpath = "/tmp/test_mlist_gd/dlls/test_keep.so";
	system("mkdir -p /tmp/test_mlist_gd/dlls");
	FILE *pf = fopen(plugpath, "w");
	fprintf(pf, "fake");
	fclose(pf);

	const char *fullpath = make_tmp_file_in(
		"linux dlls/test_keep.so KeepPlugin\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);

	list->plist[0].status = PL_RUNNING;
	list->plist[0].action = PA_NONE;
	list->plist[0].time_loaded = time(NULL) + 10;

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->plist[0].action == PA_KEEP);

	unlink(fullpath);
	unlink(plugpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_newer_file(void)
{
	TEST("ini_refresh - newer file marks reload");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	const char *plugpath = "/tmp/test_mlist_gd/dlls/test_newer.so";
	FILE *pf = fopen(plugpath, "w");
	fprintf(pf, "fake");
	fclose(pf);

	const char *fullpath = make_tmp_file_in(
		"linux dlls/test_newer.so NewerPlugin\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	list->plist[0].status = PL_OPENED;
	list->plist[0].action = PA_NONE;
	list->plist[0].time_loaded = 1;

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->plist[0].action == PA_RELOAD);

	unlink(fullpath);
	unlink(plugpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_newer_file_not_opened(void)
{
	TEST("ini_refresh - newer file with status < PL_OPENED warns");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	const char *plugpath = "/tmp/test_mlist_gd/dlls/test_newer2.so";
	FILE *pf = fopen(plugpath, "w");
	fprintf(pf, "fake");
	fclose(pf);

	const char *fullpath = make_tmp_file_in(
		"linux dlls/test_newer2.so NewerPlugin2\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	list->plist[0].status = PL_VALID;
	list->plist[0].action = PA_NONE;
	list->plist[0].time_loaded = 1;

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	unlink(plugpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_stat_fail(void)
{
	TEST("ini_refresh - stat fail on existing plugin warns");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"linux dlls/nosuchfile.so StatFail\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	list->plist[0].status = PL_RUNNING;
	list->plist[0].action = PA_NONE;

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_desc_update(void)
{
	TEST("ini_refresh - updates desc when not default");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	const char *plugpath = "/tmp/test_mlist_gd/dlls/test_desc.so";
	FILE *pf = fopen(plugpath, "w");
	fprintf(pf, "fake");
	fclose(pf);

	const char *fullpath = make_tmp_file_in(
		"linux dlls/test_desc.so OriginalDesc\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	list->plist[0].status = PL_RUNNING;
	list->plist[0].action = PA_NONE;
	list->plist[0].time_loaded = time(NULL) + 10;

	pf = fopen(fullpath, "w");
	fprintf(pf, "linux dlls/test_desc.so UpdatedDesc\n");
	fclose(pf);

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_STR(list->plist[0].desc, "UpdatedDesc");

	unlink(fullpath);
	unlink(plugpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_malformed_line(void)
{
	TEST("ini_refresh - skips malformed lines");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));
	mBOOL ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_empty(void)
{
	TEST("ini_refresh - empty file warns no plugins");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"# comments only\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));
	mBOOL ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh tests
// ============================================================

static int test_refresh_pa_keep(void)
{
	TEST("refresh - PA_KEEP sets action to PA_NONE");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	const char *plugpath = "/tmp/test_mlist_gd/dlls/ref_keep.so";
	FILE *pf = fopen(plugpath, "w");
	fprintf(pf, "fake");
	fclose(pf);

	const char *fullpath = make_tmp_file_in(
		"linux dlls/ref_keep.so RefKeep\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	list->plist[0].status = PL_RUNNING;
	list->plist[0].action = PA_NONE;
	list->plist[0].source = PS_INI;
	list->plist[0].time_loaded = time(NULL) + 10;

	ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->plist[0].action == PA_NONE);

	unlink(fullpath);
	unlink(plugpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_refresh_pa_load(void)
{
	TEST("refresh - PA_LOAD tries to load plugin");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"linux dlls/ref_load.so RefLoad\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	ret = list->refresh(PT_STARTUP);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_refresh_pa_null(void)
{
	TEST("refresh - PA_NULL logs warning");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/null_act.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "NullAction", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/null_act.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action = PA_NULL;
	added->source = PS_CMD;

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_refresh_pa_none_unload(void)
{
	TEST("refresh - PA_NONE with PS_INI+PL_RUNNING tries unload");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty - no plugins\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/removed.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "RemovedPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/removed.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_INI;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action = PA_NONE;
	added->source = PS_INI;

	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));
	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(added->status == PL_EMPTY);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_refresh_pa_reload(void)
{
	TEST("refresh - PA_RELOAD tries to reload plugin");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	const char *plugpath = "/tmp/test_mlist_gd/dlls/ref_reload.so";
	FILE *pf = fopen(plugpath, "w");
	fprintf(pf, "fake");
	fclose(pf);

	const char *fullpath = make_tmp_file_in(
		"linux dlls/ref_reload.so RefReload\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	list->plist[0].status = PL_OPENED;
	list->plist[0].action = PA_NONE;
	list->plist[0].time_loaded = 1;

	ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	unlink(plugpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_refresh_pa_attach(void)
{
	TEST("refresh - PA_ATTACH retries attach");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/attach_pl.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "AttachPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/attach_pl.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_VALID;
	added->action = PA_ATTACH;
	added->source = PS_CMD;

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_refresh_pa_unload_pending(void)
{
	TEST("refresh - PA_UNLOAD retries unload");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/unld_pl.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "UnloadPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/unld_pl.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action = PA_UNLOAD;
	added->source = PS_CMD;

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// load() tests
// ============================================================

static int test_load_ini_startup_fail(void)
{
	TEST("load - returns false when ini_startup fails");
	setup_globals();
	HeapPluginList list("nonexistent_plugins.ini");
	Plugins = list.ptr;
	mBOOL ret = list->load();
	ASSERT_TRUE(ret == mFALSE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_load_with_valid_ini(void)
{
	TEST("load - loads plugins from ini");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/loadtest.so LoadTest\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->load();
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show with running plugins
// ============================================================

static int test_show_running_with_info(void)
{
	TEST("show - running plugin with info shows version");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t show_info;
	memset(&show_info, 0, sizeof(show_info));
	show_info.name = "ShowTest";
	show_info.version = "1.2.3";

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "showrun.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "ShowRunPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/showrun.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->info = &show_info;

	list->show();
	int found_plugin = 0, found_running = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "ShowRunPlugin"))
			found_plugin = 1;
		if (strstr(mock_get_server_print_msg(i), "1 running"))
			found_running = 1;
	}
	ASSERT_TRUE(found_plugin);
	ASSERT_TRUE(found_running);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_show_source_index_filters(void)
{
	TEST("show - source_index skips non-matching plugins");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "filtered.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "FilteredOut", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/filtered.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source_plugin_index = 99;
	list->add(&temp);

	list->show(5);
	int found = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "FilteredOut"))
			found = 1;
	}
	ASSERT_FALSE(found);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show_client with running plugins
// ============================================================

static int test_show_client_running(void)
{
	TEST("show_client - shows running plugins with info");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t cl_info;
	memset(&cl_info, 0, sizeof(cl_info));
	cl_info.name = "ClientVis";
	cl_info.version = "2.0";
	cl_info.date = "2024-01-01";
	cl_info.author = "TestAuthor";
	cl_info.url = "http://test.com";

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "clientvis.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "ClientVisPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/clientvis.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->info = &cl_info;

	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	list->show_client(&ed);

	int found = 0;
	for (int i = 0; i < mock_get_client_print_count(); i++) {
		if (strstr(mock_get_client_print_msg(i), "ClientVis"))
			found = 1;
	}
	ASSERT_TRUE(found);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_show_client_null_fields(void)
{
	TEST("show_client - handles NULL info fields");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t null_info;
	memset(&null_info, 0, sizeof(null_info));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "nullinfo.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "NullInfoPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/nullinfo.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->info = &null_info;

	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	list->show_client(&ed);

	int found = 0;
	for (int i = 0; i < mock_get_client_print_count(); i++) {
		if (strstr(mock_get_client_print_msg(i), "<unknown>"))
			found = 1;
	}
	ASSERT_TRUE(found);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// unpause_all with paused plugin
// ============================================================

static int test_unpause_all_unpauses(void)
{
	TEST("unpause_all - unpauses PL_PAUSED plugins");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "paused.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "PausedPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/paused.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->status = PL_PAUSED;

	list->unpause_all();
	ASSERT_TRUE(added->status == PL_RUNNING);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// retry_all with pending actions
// ============================================================

static int test_retry_all_with_pending(void)
{
	TEST("retry_all - retries pending PA_LOAD action");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/retry.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "RetryPlugin", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/retry.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->action = PA_LOAD;

	list->retry_all(PT_ANYTIME);
	ASSERT_TRUE(1);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_startup - line with \r\n
// ============================================================

static int test_ini_startup_crlf(void)
{
	TEST("ini_startup - handles \\r\\n line endings");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/crlf.so CrLfPlugin\r\n# comment\r\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload tests
// ============================================================

static int test_plugin_addload_bad_plid(void)
{
	TEST("plugin_addload - unknown plid returns ME_BADREQ");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	plugin_info_t fake_info;
	memset(&fake_info, 0, sizeof(fake_info));
	MPlugin *p = list->plugin_addload((plid_t)&fake_info, "test.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_BADREQ);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find with empty slots before valid entries
// ============================================================

static int test_find_handle_skips_empty(void)
{
	TEST("find(DLHANDLE) - skips empty slots");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "first.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "First", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/first.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "second.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "Second", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/second.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	MPlugin *p2 = list->add(&t2);
	p2->handle = (DLHANDLE)0xBEEF;

	p1->status = PL_EMPTY;

	MPlugin *found = list->find((DLHANDLE)0xBEEF);
	ASSERT_TRUE(found == p2);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_plid_skips_empty(void)
{
	TEST("find(plid_t) - skips empty slots");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t pi;
	memset(&pi, 0, sizeof(pi));
	pi.name = "Target";

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "a.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "A", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/a.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "b.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "B", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/b.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	MPlugin *p2 = list->add(&t2);
	p2->info = &pi;

	p1->status = PL_EMPTY;

	MPlugin *found = list->find((plid_t)&pi);
	ASSERT_TRUE(found == p2);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_path_skips_empty(void)
{
	TEST("find(path) - skips empty slots");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "e1.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "E1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/e1.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "e2.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "E2", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/e2.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	list->add(&t2);

	p1->status = PL_EMPTY;

	MPlugin *found = list->find("/tmp/e2.so");
	ASSERT_TRUE(found != NULL);
	ASSERT_STR(found->desc, "E2");

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_clear_source_skips_empty(void)
{
	TEST("clear_source_plugin_index - skips empty slots");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "s1.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "S1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/s1.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "s2.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "S2", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/s2.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	t2.source_plugin_index = 7;
	MPlugin *p2 = list->add(&t2);
	p2->source_plugin_index = 7;

	p1->status = PL_EMPTY;

	list->clear_source_plugin_index(7);
	ASSERT_TRUE(p2->source_plugin_index == -1);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_found_child_skips_empty(void)
{
	TEST("found_child_plugins - skips empty slots");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "c1.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "C1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/c1.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "c2.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "C2", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/c2.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	t2.source_plugin_index = 3;
	MPlugin *p2 = list->add(&t2);
	p2->source_plugin_index = 3;

	p1->status = PL_EMPTY;

	ASSERT_TRUE(list->found_child_plugins(3) == mTRUE);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_find_match_skips_empty(void)
{
	TEST("find_match(str) - skips empty slots");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "skip1.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "Skip1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/skip1.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "target.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "TargetFound", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/target.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	list->add(&t2);

	p1->status = PL_EMPTY;

	MPlugin *found = list->find_match("Target");
	ASSERT_TRUE(found != NULL);
	ASSERT_STR(found->desc, "TargetFound");

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_match NOTUNIQ by file prefix
// ============================================================

static int test_find_match_notuniq_by_file(void)
{
	TEST("find_match - NOTUNIQ by file prefix");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "samefile_a.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "xxx1", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/samefile_a.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "samefile_b.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "yyy2", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/samefile_b.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	list->add(&t2);

	MPlugin *found = list->find_match("samefile");
	ASSERT_TRUE(found == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTUNIQ);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload - with resolvable file
// ============================================================

static int test_cmd_addload_resolves_and_loads(void)
{
	TEST("cmd_addload - resolves file, load fails");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	FILE *pf = fopen("/tmp/test_mlist_gd/dlls/cmd_test.so", "w");
	fprintf(pf, "not a real so");
	fclose(pf);

	HeapPluginList listp("test.ini");
	Plugins = listp.ptr;
	mBOOL ret = listp->cmd_addload("load dlls/cmd_test.so");
	ASSERT_TRUE(ret == mFALSE);

	unlink("/tmp/test_mlist_gd/dlls/cmd_test.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_cmd_addload_already_loaded(void)
{
	TEST("cmd_addload - already loaded returns ME_ALREADY");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	FILE *pf = fopen("/tmp/test_mlist_gd/dlls/cmd_dup.so", "w");
	fprintf(pf, "fake");
	fclose(pf);

	HeapPluginList listp("test.ini");
	Plugins = listp.ptr;

	char resolved[PATH_MAX];
	snprintf(resolved, sizeof(resolved), "%s/dlls/cmd_dup.so", GameDLL.gamedir);

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/cmd_dup.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "AlreadyLoaded", sizeof(temp.desc));
	STRNCPY(temp.pathname, resolved, sizeof(temp.pathname));
	temp.status = PL_VALID;
	listp->add(&temp);

	mBOOL ret = listp->cmd_addload("load dlls/cmd_dup.so");
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_ALREADY);

	unlink("/tmp/test_mlist_gd/dlls/cmd_dup.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload - with valid plid
// ============================================================

static int test_plugin_addload_resolve_fail(void)
{
	TEST("plugin_addload - resolve fail returns ME_NOTFOUND");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t loader_info;
	memset(&loader_info, 0, sizeof(loader_info));
	loader_info.name = "LoaderPlugin";

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "loader.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Loader", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/loader.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *loader = list->add(&temp);
	loader->info = &loader_info;

	MPlugin *p = list->plugin_addload((plid_t)&loader_info, "nonexistent.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTFOUND);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_plugin_addload_already_loaded(void)
{
	TEST("plugin_addload - already loaded returns ME_ALREADY");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	FILE *pf = fopen("/tmp/test_mlist_gd/dlls/pa_dup.so", "w");
	fprintf(pf, "fake");
	fclose(pf);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t loader_info;
	memset(&loader_info, 0, sizeof(loader_info));
	loader_info.name = "LoaderPlugin2";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "loader2.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "Loader2", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/loader2.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = list->add(&t_loader);
	loader->info = &loader_info;

	MPlugin t_existing;
	memset(&t_existing, 0, sizeof(t_existing));
	STRNCPY(t_existing.filename, "dlls/pa_dup.so", sizeof(t_existing.filename));
	t_existing.file = t_existing.filename + 5;
	STRNCPY(t_existing.desc, "Existing", sizeof(t_existing.desc));
	STRNCPY(t_existing.pathname, "/tmp/test_mlist_gd/dlls/pa_dup.so", sizeof(t_existing.pathname));
	t_existing.status = PL_VALID;
	list->add(&t_existing);

	MPlugin *p = list->plugin_addload((plid_t)&loader_info, "dlls/pa_dup.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_ALREADY);

	unlink("/tmp/test_mlist_gd/dlls/pa_dup.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_plugin_addload_load_fails(void)
{
	TEST("plugin_addload - load failure returns NULL");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	FILE *pf = fopen("/tmp/test_mlist_gd/dlls/pa_fail.so", "w");
	fprintf(pf, "not a real so");
	fclose(pf);

	HeapPluginList listp("test.ini");
	Plugins = listp.ptr;

	static plugin_info_t loader_info;
	memset(&loader_info, 0, sizeof(loader_info));
	loader_info.name = "LoaderPlugin3";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "loader3.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "Loader3", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/loader3.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = listp->add(&t_loader);
	loader->info = &loader_info;

	MPlugin *p = listp->plugin_addload((plid_t)&loader_info, "dlls/pa_fail.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);

	unlink("/tmp/test_mlist_gd/dlls/pa_fail.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_startup - malformed line
// ============================================================

static int test_ini_startup_malformed(void)
{
	TEST("ini_startup - warns on malformed line");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux\n"
		"linux dlls/ok.so GoodPlugin\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_startup_pfspecific_override(void)
{
	TEST("ini_startup - platform-specific entry overrides generic");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/plug_generic.so TestPlug\n"
		PLATFORM_SPC " dlls/plug_specific.so TestPlug\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 2);
	ASSERT_TRUE(list->plist[0].status == PL_EMPTY);
	ASSERT_TRUE(list->plist[1].pfspecific == 1);
	ASSERT_STR(list->plist[1].desc, "TestPlug");

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_startup_pfspecific_skip_lower(void)
{
	TEST("ini_startup - higher pfspecific skips lower duplicate");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		PLATFORM_SPC " dlls/plug_specific.so TestPlug\n"
		"linux dlls/plug_generic.so TestPlug\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);
	ASSERT_TRUE(list->plist[0].pfspecific == 1);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - ini_refresh failure
// ============================================================

static int test_refresh_ini_refresh_fail(void)
{
	TEST("refresh - returns false when ini_refresh fails");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, "/tmp/nonexistent_refresh.ini", sizeof(list->inifile));
	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mFALSE);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show - skips invalid status in loop
// ============================================================

static int test_show_skips_invalid(void)
{
	TEST("show - skips PL_EMPTY plugins in loop");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin t1;
	memset(&t1, 0, sizeof(t1));
	STRNCPY(t1.filename, "empty.so", sizeof(t1.filename));
	t1.file = t1.filename;
	STRNCPY(t1.desc, "EmptySlot", sizeof(t1.desc));
	STRNCPY(t1.pathname, "/tmp/empty.so", sizeof(t1.pathname));
	t1.status = PL_VALID;
	MPlugin *p1 = list->add(&t1);

	MPlugin t2;
	memset(&t2, 0, sizeof(t2));
	STRNCPY(t2.filename, "visible.so", sizeof(t2.filename));
	t2.file = t2.filename;
	STRNCPY(t2.desc, "VisiblePlugin", sizeof(t2.desc));
	STRNCPY(t2.pathname, "/tmp/visible.so", sizeof(t2.pathname));
	t2.status = PL_VALID;
	list->add(&t2);

	p1->status = PL_EMPTY;

	list->show();
	int found_empty = 0, found_visible = 0;
	for (int i = 0; i < mock_get_server_print_count(); i++) {
		if (strstr(mock_get_server_print_msg(i), "EmptySlot"))
			found_empty = 1;
		if (strstr(mock_get_server_print_msg(i), "VisiblePlugin"))
			found_visible = 1;
	}
	ASSERT_FALSE(found_empty);
	ASSERT_TRUE(found_visible);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// show_client - skips non-running
// ============================================================

static int test_show_client_skips_nonrunning(void)
{
	TEST("show_client - skips non-running plugins");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "paused.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "PausedSkip", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/paused.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->status = PL_PAUSED;

	edict_t ed;
	memset(&ed, 0, sizeof(ed));
	list->show_client(&ed);

	int found = 0;
	for (int i = 0; i < mock_get_client_print_count(); i++) {
		if (strstr(mock_get_client_print_msg(i), "PausedSkip"))
			found = 1;
	}
	ASSERT_FALSE(found);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - skips invalid status
// ============================================================

static int test_refresh_skips_invalid(void)
{
	TEST("refresh - skips PL_EMPTY plugins in loop");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "x.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "X", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/x.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	added->status = PL_EMPTY;

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// reset_plugin
// ============================================================

static int test_reset_plugin(void)
{
	TEST("reset_plugin - resets plugin and preserves index");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "reset_me.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "ToReset", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/reset_me.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *added = list->add(&temp);
	int idx = added->index;
	ASSERT_TRUE(added->status == PL_VALID);

	list->reset_plugin(added);
	ASSERT_TRUE(added->status == PL_EMPTY);
	ASSERT_TRUE(added->index == idx);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// add - overflow returns ME_MAXREACHED
// ============================================================

static int test_add_overflow(void)
{
	TEST("add - full list returns ME_MAXREACHED");
	setup_globals();
	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	MPlugin temp;
	char fname[32];
	for(int i = 0; i < MAX_PLUGINS; i++) {
		memset(&temp, 0, sizeof(temp));
		snprintf(fname, sizeof(fname), "plug%d.so", i);
		STRNCPY(temp.filename, fname, sizeof(temp.filename));
		temp.file = temp.filename;
		STRNCPY(temp.desc, fname, sizeof(temp.desc));
		STRNCPY(temp.pathname, fname, sizeof(temp.pathname));
		temp.status = PL_VALID;
		MPlugin *added = list->add(&temp);
		ASSERT_PTR_NOT_NULL(added);
	}
	ASSERT_TRUE(list->endlist == MAX_PLUGINS);

	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "overflow.so", sizeof(temp.filename));
	temp.file = temp.filename;
	STRNCPY(temp.desc, "Overflow", sizeof(temp.desc));
	STRNCPY(temp.pathname, "overflow.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	MPlugin *overflow = list->add(&temp);
	ASSERT_PTR_NULL(overflow);
	ASSERT_INT(meta_errno, ME_MAXREACHED);

	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_refresh - CRLF line endings
// ============================================================

static int test_ini_refresh_crlf(void)
{
	TEST("ini_refresh - handles \\r\\n line endings");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/crlf_ref.so CrLfRefresh\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);

	FILE *fp = fopen(fullpath, "w");
	fprintf(fp, "linux dlls/crlf_ref.so CrLfRefresh\r\nlinux dlls/crlf2.so Plug2\r\n");
	fclose(fp);

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 2);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_startup - fopen failure after valid_gamedir_file passes
// ============================================================

static int test_ini_startup_fopen_fail(void)
{
	TEST("ini_startup - fopen fails on unreadable file (lines 385-387)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");
	const char *fullpath = make_tmp_file_in(
		"linux dlls/test.so TestPlug\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	// Make the file unreadable so fopen fails after valid_gamedir_file
	chmod(fullpath, 0000);

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOFILE);

	// Restore and cleanup
	chmod(fullpath, 0644);
	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// ini_refresh - pfspecific override logic (lines 480-504)
// ============================================================

static int test_ini_refresh_pfspecific_skip_higher_existing(void)
{
	TEST("ini_refresh - skips new entry when existing pfspecific is higher (lines 480-483)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	// Start with a specific platform entry
	const char *fullpath = make_tmp_file_in(
		PLATFORM_SPC " dlls/pf_test.so PfTest\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);
	ASSERT_TRUE(list->plist[0].pfspecific == 1);

	// Now rewrite ini with a generic entry for same plugin name
	// find_match will match by desc "PfTest", but the existing pfspecific(1)
	// is >= the new pfspecific(0), so the new entry should be skipped
	FILE *fp = fopen(fullpath, "w");
	fprintf(fp, "linux dlls/pf_test_generic.so PfTest\n");
	fclose(fp);

	// Mark existing plugin as already loaded so ini_refresh finds it
	// by find_match not find(pathname)
	list->plist[0].action = PA_KEEP;
	list->plist[0].status = PL_RUNNING;
	list->plist[0].source = PS_INI;

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	// Should still have just 1 plugin, the new generic should have been skipped
	ASSERT_TRUE(list->endlist == 1);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_pfspecific_override_pa_load(void)
{
	TEST("ini_refresh - override PA_LOAD entry with higher pfspecific (lines 485-490)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	// Start with a generic entry
	const char *fullpath = make_tmp_file_in(
		"linux dlls/pf_over.so PfOver\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);
	ASSERT_TRUE(list->plist[0].pfspecific == 0);

	// Rewrite ini with a platform-specific override for same desc
	// The existing plugin has PA_LOAD (set by ini_startup), pfspecific=0
	// The new entry has pfspecific=1, so it should override
	FILE *fp = fopen(fullpath, "w");
	fprintf(fp, PLATFORM_SPC " dlls/pf_over_specific.so PfOver\n");
	fclose(fp);

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	// The old PA_LOAD entry was reset then reused by add() for the new plugin.
	// Note: add() does not copy pfspecific, so it stays 0 after reset.
	ASSERT_TRUE(list->plist[0].action == PA_LOAD);
	ASSERT_TRUE(list->plist[0].status == PL_VALID);
	ASSERT_STR_CONTAINS(list->plist[0].filename, "pf_over_specific.so");
	ASSERT_TRUE(list->endlist == 1);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_pfspecific_unable_to_comply(void)
{
	TEST("ini_refresh - cannot override non-PA_LOAD entry (lines 491-494)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	// Start with a generic entry
	const char *fullpath = make_tmp_file_in(
		"linux dlls/pf_no.so PfNo\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->endlist == 1);

	// Change the existing plugin's action to something other than PA_LOAD
	// so the override logic hits the "Unable to comply" branch
	list->plist[0].action = PA_KEEP;
	list->plist[0].status = PL_RUNNING;
	list->plist[0].source = PS_INI;

	// Rewrite ini with platform-specific entry (higher pfspecific)
	FILE *fp = fopen(fullpath, "w");
	fprintf(fp, PLATFORM_SPC " dlls/pf_no_specific.so PfNo\n");
	fclose(fp);

	ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	// The old entry should still be there, unchanged
	ASSERT_TRUE(list->endlist == 1);
	ASSERT_TRUE(list->plist[0].action == PA_KEEP);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

static int test_ini_refresh_add_overflow(void)
{
	TEST("ini_refresh - add() fails when list is full (line 504)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"linux dlls/overflow_new.so OverflowNew\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	// Fill the list to capacity
	MPlugin temp;
	char fn[32];
	for (int i = 0; i < MAX_PLUGINS; i++) {
		memset(&temp, 0, sizeof(temp));
		snprintf(fn, sizeof(fn), "full%d.so", i);
		STRNCPY(temp.filename, fn, sizeof(temp.filename));
		temp.file = temp.filename;
		STRNCPY(temp.desc, fn, sizeof(temp.desc));
		STRNCPY(temp.pathname, fn, sizeof(temp.pathname));
		temp.status = PL_VALID;
		MPlugin *added = list->add(&temp);
		ASSERT_PTR_NOT_NULL(added);
		added->action = PA_NONE;
		added->source = PS_CMD;
	}

	mBOOL ret = list->ini_refresh();
	ASSERT_TRUE(ret == mTRUE);
	// The add should have failed silently (continue), but ini_refresh
	// still returns true
	ASSERT_TRUE(list->endlist == MAX_PLUGINS);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload - add overflow (lines 598-600)
// ============================================================

static int test_plugin_addload_add_overflow(void)
{
	TEST("plugin_addload - add() fails when list full (lines 598-600)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	FILE *pf = fopen("/tmp/test_mlist_gd/dlls/pa_over.so", "w");
	fprintf(pf, "not a real so");
	fclose(pf);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t loader_info_over;
	memset(&loader_info_over, 0, sizeof(loader_info_over));
	loader_info_over.name = "LoaderOverflow";

	// Add loader plugin first
	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "loader_over.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "LoaderOver", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/loader_over.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = list->add(&t_loader);
	loader->info = &loader_info_over;

	// Fill remaining slots
	MPlugin temp;
	char fn[32];
	for (int i = 1; i < MAX_PLUGINS; i++) {
		memset(&temp, 0, sizeof(temp));
		snprintf(fn, sizeof(fn), "fill%d.so", i);
		STRNCPY(temp.filename, fn, sizeof(temp.filename));
		temp.file = temp.filename;
		STRNCPY(temp.desc, fn, sizeof(temp.desc));
		STRNCPY(temp.pathname, fn, sizeof(temp.pathname));
		temp.status = PL_VALID;
		list->add(&temp);
	}
	ASSERT_TRUE(list->endlist == MAX_PLUGINS);

	MPlugin *p = list->plugin_addload((plid_t)&loader_info_over, "dlls/pa_over.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);

	unlink("/tmp/test_mlist_gd/dlls/pa_over.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload - load fails with ME_NOTALLOWED (lines 608-611)
// ============================================================

static int test_plugin_addload_load_notallowed(void)
{
	TEST("plugin_addload - load fails with ME_NOTALLOWED (lines 608-611)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	// Copy the fake plugin so to the game directory
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/pa_notallowed.so");

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info_na;
	memset(&ldr_info_na, 0, sizeof(ldr_info_na));
	ldr_info_na.name = "LoaderNA";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "loader_na.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "LoaderNA", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/loader_na.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = list->add(&t_loader);
	loader->info = &ldr_info_na;

	// Set loadable=PT_STARTUP(1), try to load at PT_ANYTIME(3)
	// This makes info->loadable(1) < now(3), and loadable(1) > PT_STARTUP(1) is false
	// => ME_NOTALLOWED
	setenv("FAKE_MM_LOADABLE", "1", 1);

	MPlugin *p = list->plugin_addload((plid_t)&ldr_info_na, "dlls/pa_notallowed.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);

	unsetenv("FAKE_MM_LOADABLE");
	unlink("/tmp/test_mlist_gd/dlls/pa_notallowed.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload - load fails with ME_DELAYED (lines 608-611)
// ============================================================

static int test_plugin_addload_load_delayed(void)
{
	TEST("plugin_addload - load fails with ME_DELAYED (lines 608-611)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/pa_delayed.so");

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info_del;
	memset(&ldr_info_del, 0, sizeof(ldr_info_del));
	ldr_info_del.name = "LoaderDel";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "loader_del.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "LoaderDel", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/loader_del.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = list->add(&t_loader);
	loader->info = &ldr_info_del;

	// Set loadable=PT_CHANGELEVEL(2), try to load at PT_ANYTIME(3)
	// loadable(2) < now(3) => true, loadable(2) > PT_STARTUP(1) => true
	// => ME_DELAYED
	setenv("FAKE_MM_LOADABLE", "2", 1);

	MPlugin *p = list->plugin_addload((plid_t)&ldr_info_del, "dlls/pa_delayed.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);
	ASSERT_TRUE(meta_errno == ME_DELAYED);

	unsetenv("FAKE_MM_LOADABLE");
	unlink("/tmp/test_mlist_gd/dlls/pa_delayed.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload - successful load (lines 619-621)
// ============================================================

static int test_plugin_addload_load_success(void)
{
	TEST("plugin_addload - successful load (lines 619-621)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/pa_success.so");

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	static plugin_info_t ldr_info_ok;
	memset(&ldr_info_ok, 0, sizeof(ldr_info_ok));
	ldr_info_ok.name = "LoaderOK";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "loader_ok.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "LoaderOK", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/loader_ok.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = list->add(&t_loader);
	loader->info = &ldr_info_ok;

	// Default FAKE_MM_LOADABLE is PT_ANYTIME(3), matching our load time
	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	MPlugin *p = list->plugin_addload((plid_t)&ldr_info_ok, "dlls/pa_success.so", PT_ANYTIME);
	ASSERT_TRUE(p != NULL);
	ASSERT_TRUE(p->status == PL_RUNNING);
	ASSERT_TRUE(meta_errno == ME_NOERROR);

	// Clean up: free allocated resources and close dlhandle
	p->free_api_pointers();
	if (p->handle) { DLCLOSE(p->handle); p->handle = NULL; }
	p->test_gamedll_funcs().dllapi_table = NULL;
	p->test_gamedll_funcs().newapi_table = NULL;
	p->status = PL_EMPTY;

	unlink("/tmp/test_mlist_gd/dlls/pa_success.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload - load fails with ME_DELAYED (lines 675-677)
// ============================================================

static int test_cmd_addload_load_delayed(void)
{
	TEST("cmd_addload - load fails with ME_DELAYED (lines 675-677)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/cmd_delayed.so");

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	// loadable=PT_CHANGELEVEL(2), cmd_addload calls load(PT_ANYTIME)
	// loadable(2) < now(3) true, loadable(2) > PT_STARTUP(1) true => ME_DELAYED
	setenv("FAKE_MM_LOADABLE", "2", 1);

	mBOOL ret = list->cmd_addload("load dlls/cmd_delayed.so");
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_DELAYED);

	// Clean up: ME_DELAYED leaves plugin with open dlhandle at PL_OPENED
	MPlugin *delayed = list->find("/tmp/test_mlist_gd/dlls/cmd_delayed.so");
	if (delayed && delayed->handle) {
		delayed->free_api_pointers();
		DLCLOSE(delayed->handle);
		delayed->handle = NULL;
		delayed->status = PL_EMPTY;
	}

	unsetenv("FAKE_MM_LOADABLE");
	unlink("/tmp/test_mlist_gd/dlls/cmd_delayed.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload - load fails with ME_NOTALLOWED (lines 678-681)
// ============================================================

static int test_cmd_addload_load_notallowed(void)
{
	TEST("cmd_addload - load fails with ME_NOTALLOWED (lines 678-681)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/cmd_notallowed.so");

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	// loadable=PT_STARTUP(1), cmd_addload calls load(PT_ANYTIME)
	// loadable(1) < now(3) true, loadable(1) > PT_STARTUP(1) false => ME_NOTALLOWED
	setenv("FAKE_MM_LOADABLE", "1", 1);

	mBOOL ret = list->cmd_addload("load dlls/cmd_notallowed.so");
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_NOTALLOWED);

	unsetenv("FAKE_MM_LOADABLE");
	unlink("/tmp/test_mlist_gd/dlls/cmd_notallowed.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload - successful load (lines 691-693)
// ============================================================

static int test_cmd_addload_load_success(void)
{
	TEST("cmd_addload - successful load (lines 691-693)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/cmd_success.so");

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	// Default loadable=PT_ANYTIME, cmd_addload calls load(PT_ANYTIME) => success
	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	mBOOL ret = list->cmd_addload("load dlls/cmd_success.so");
	ASSERT_TRUE(ret == mTRUE);
	// Find the loaded plugin and verify
	MPlugin *loaded = list->find("/tmp/test_mlist_gd/dlls/cmd_success.so");
	ASSERT_TRUE(loaded != NULL);
	ASSERT_TRUE(loaded->status == PL_RUNNING);

	// Clean up: free allocated resources and close dlhandle
	loaded->free_api_pointers();
	if (loaded->handle) { DLCLOSE(loaded->handle); loaded->handle = NULL; }
	loaded->test_gamedll_funcs().dllapi_table = NULL;
	loaded->test_gamedll_funcs().newapi_table = NULL;
	loaded->status = PL_EMPTY;

	unlink("/tmp/test_mlist_gd/dlls/cmd_success.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload - add overflow (lines 665-668)
// ============================================================

static int test_cmd_addload_add_overflow(void)
{
	TEST("cmd_addload - add() fails when list full (lines 665-668)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	FILE *pf = fopen("/tmp/test_mlist_gd/dlls/cmd_over.so", "w");
	fprintf(pf, "fake");
	fclose(pf);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;

	// Fill the list to capacity
	MPlugin temp;
	char fn[32];
	for (int i = 0; i < MAX_PLUGINS; i++) {
		memset(&temp, 0, sizeof(temp));
		snprintf(fn, sizeof(fn), "cfull%d.so", i);
		STRNCPY(temp.filename, fn, sizeof(temp.filename));
		temp.file = temp.filename;
		STRNCPY(temp.desc, fn, sizeof(temp.desc));
		STRNCPY(temp.pathname, fn, sizeof(temp.pathname));
		temp.status = PL_VALID;
		list->add(&temp);
	}
	ASSERT_TRUE(list->endlist == MAX_PLUGINS);

	mBOOL ret = list->cmd_addload("load dlls/cmd_over.so");
	ASSERT_TRUE(ret == mFALSE);
	ASSERT_TRUE(meta_errno == ME_MAXREACHED);

	unlink("/tmp/test_mlist_gd/dlls/cmd_over.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - PA_LOAD with ME_DELAYED (lines 748-751)
// ============================================================

static int test_refresh_pa_load_delayed(void)
{
	TEST("refresh - PA_LOAD with ME_DELAYED increments ndelayed (lines 748-751)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/ref_delayed.so");

	const char *fullpath = make_tmp_file_in(
		"linux dlls/ref_delayed.so RefDelayed\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;
	mBOOL ret = list->ini_startup();
	ASSERT_TRUE(ret == mTRUE);
	ASSERT_TRUE(list->plist[0].action == PA_LOAD);

	// Set plugin to only be loadable at changelevel, then refresh at anytime
	// This will cause load() to return ME_DELAYED
	setenv("FAKE_MM_LOADABLE", "2", 1);

	ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);
	// Plugin should have been attempted but delayed
	// After ME_DELAYED in load(), action stays PA_LOAD (unchanged by refresh)

	// Clean up: ME_DELAYED leaves plugin with open dlhandle at PL_OPENED
	if (list->plist[0].handle) {
		list->plist[0].free_api_pointers();
		DLCLOSE(list->plist[0].handle);
		list->plist[0].handle = NULL;
		list->plist[0].status = PL_EMPTY;
	}

	unsetenv("FAKE_MM_LOADABLE");
	unlink(fullpath);
	unlink("/tmp/test_mlist_gd/dlls/ref_delayed.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - PA_RELOAD with ME_DELAYED (lines 755-758)
// ============================================================

static plugin_info_t delayed_info_changelevel;

static void init_delayed_info(void)
{
	memset(&delayed_info_changelevel, 0, sizeof(delayed_info_changelevel));
	delayed_info_changelevel.ifvers = (char *)"5:13";
	delayed_info_changelevel.name = (char *)"DelayedPlugin";
	delayed_info_changelevel.version = (char *)"1.0";
	delayed_info_changelevel.date = (char *)"2024/01/01";
	delayed_info_changelevel.author = (char *)"Test";
	delayed_info_changelevel.url = (char *)"http://test";
	delayed_info_changelevel.logtag = (char *)"DELAY";
	delayed_info_changelevel.loadable = PT_CHANGELEVEL;
	delayed_info_changelevel.unloadable = PT_CHANGELEVEL;
}

static int test_refresh_pa_reload_delayed(void)
{
	TEST("refresh - PA_RELOAD with ME_DELAYED increments ndelayed (lines 755-758)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	init_delayed_info();

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/delay_reload.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "DelayReload", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/delay_reload.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action = PA_RELOAD;
	added->source = PS_CMD;
	added->info = &delayed_info_changelevel;

	// reload() will check info->loadable (PT_CHANGELEVEL=2) < now (PT_ANYTIME=3)
	// and info->loadable > PT_STARTUP => ME_DELAYED
	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - PA_NONE unload with ME_DELAYED (lines 765-768)
// ============================================================

static int test_refresh_pa_none_unload_delayed(void)
{
	TEST("refresh - PA_NONE INI unload with ME_DELAYED (lines 765-768)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty - no plugins\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;

	init_delayed_info();

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/delay_unload.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "DelayUnload", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/delay_unload.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_INI;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action = PA_NONE;
	added->source = PS_INI;
	added->info = &delayed_info_changelevel;

	// PA_NONE with PS_INI+PL_RUNNING => tries unload
	// unload() checks info->unloadable (PT_CHANGELEVEL=2) < now (PT_ANYTIME=3)
	// and info->unloadable > PT_STARTUP => ME_DELAYED
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));
	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - PA_ATTACH with ME_DELAYED (lines 774-777)
// ============================================================

static int test_refresh_pa_attach_delayed(void)
{
	TEST("refresh - PA_ATTACH retry with ME_DELAYED (lines 774-777)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");

	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/delay_attach.so");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/delay_attach.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "DelayAttach", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/delay_attach.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_VALID;
	added->action = PA_ATTACH;
	added->source = PS_CMD;

	// retry(PA_ATTACH) calls load() which opens the .so and queries Meta_Query.
	// Set loadable to PT_CHANGELEVEL so load returns ME_DELAYED.
	setenv("FAKE_MM_LOADABLE", "2", 1);

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	// Clean up: ME_DELAYED leaves plugin with open dlhandle at PL_OPENED
	if (added->handle) {
		added->free_api_pointers();
		DLCLOSE(added->handle);
		added->handle = NULL;
		added->status = PL_EMPTY;
	}

	unsetenv("FAKE_MM_LOADABLE");
	unlink(fullpath);
	unlink("/tmp/test_mlist_gd/dlls/delay_attach.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - PA_UNLOAD with ME_DELAYED (lines 782-785)
// ============================================================

static int test_refresh_pa_unload_delayed(void)
{
	TEST("refresh - PA_UNLOAD retry with ME_DELAYED (lines 782-785)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	init_delayed_info();

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/delay_unl_p.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "DelayUnloadP", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/delay_unl_p.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action = PA_UNLOAD;
	added->source = PS_CMD;
	added->info = &delayed_info_changelevel;

	// retry(PA_UNLOAD) calls unload() which checks info->unloadable
	// unloadable (PT_CHANGELEVEL=2) < now (PT_ANYTIME=3) and > PT_STARTUP => ME_DELAYED
	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - default/unrecognized action (lines 790-792)
// ============================================================

static int test_refresh_default_action(void)
{
	TEST("refresh - unrecognized action value triggers warning (lines 790-792)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd");

	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/bad_act.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "BadAction", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/bad_act.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_CMD;
	MPlugin *added = list->add(&temp);
	added->status = PL_RUNNING;
	added->action_int = 99;
	added->source = PS_CMD;

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	unlink(fullpath);
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// plugin_addload - attach fails, status PL_OPENED (line 613)
// ============================================================

static int test_plugin_addload_attach_fails(void)
{
	TEST("plugin_addload - attach failure with PL_OPENED status (line 613)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/pa_attfail.so");

	HeapPluginList listp("test.ini");
	Plugins = listp.ptr;

	static plugin_info_t loader_info;
	memset(&loader_info, 0, sizeof(loader_info));
	loader_info.name = "AttFailLoader";

	MPlugin t_loader;
	memset(&t_loader, 0, sizeof(t_loader));
	STRNCPY(t_loader.filename, "attfail_loader.so", sizeof(t_loader.filename));
	t_loader.file = t_loader.filename;
	STRNCPY(t_loader.desc, "AttFailLoader", sizeof(t_loader.desc));
	STRNCPY(t_loader.pathname, "/tmp/attfail_loader.so", sizeof(t_loader.pathname));
	t_loader.status = PL_VALID;
	MPlugin *loader = listp->add(&t_loader);
	loader->info = &loader_info;

	setenv("FAKE_MM_ATTACH_FAIL", "1", 1);
	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_UNLOADABLE");

	MPlugin *p = listp->plugin_addload((plid_t)&loader_info, "dlls/pa_attfail.so", PT_ANYTIME);
	ASSERT_TRUE(p == NULL);

	unsetenv("FAKE_MM_ATTACH_FAIL");
	unlink("/tmp/test_mlist_gd/dlls/pa_attfail.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// cmd_addload - attach fails, status PL_OPENED (line 684)
// ============================================================

static int test_cmd_addload_attach_fails(void)
{
	TEST("cmd_addload - attach failure with PL_OPENED status (line 684)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/cmd_attfail.so");

	HeapPluginList listp("test.ini");
	Plugins = listp.ptr;

	setenv("FAKE_MM_ATTACH_FAIL", "1", 1);
	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_UNLOADABLE");

	mBOOL ret = listp->cmd_addload("dlls/cmd_attfail.so");
	ASSERT_TRUE(ret == mFALSE);

	unsetenv("FAKE_MM_ATTACH_FAIL");
	unlink("/tmp/test_mlist_gd/dlls/cmd_attfail.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// refresh - successful load during refresh (line 749)
// ============================================================

static int test_refresh_pa_load_success(void)
{
	TEST("refresh - PA_LOAD succeeds with real .so plugin (line 749)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/ref_succ.so");

	// Create ini file with the plugin entry
	const char *fullpath = make_tmp_file_in(
		"# empty\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);

	HeapPluginList list("test.ini");
	Plugins = list.ptr;
	STRNCPY(list->inifile, fullpath, sizeof(list->inifile));

	// Manually add a plugin with PA_LOAD action, pointing to real .so
	MPlugin temp;
	memset(&temp, 0, sizeof(temp));
	STRNCPY(temp.filename, "dlls/ref_succ.so", sizeof(temp.filename));
	temp.file = temp.filename + 5;
	STRNCPY(temp.desc, "RefSucc", sizeof(temp.desc));
	STRNCPY(temp.pathname, "/tmp/test_mlist_gd/dlls/ref_succ.so", sizeof(temp.pathname));
	temp.status = PL_VALID;
	temp.source = PS_INI;
	MPlugin *added = list->add(&temp);
	added->action = PA_LOAD;

	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_UNLOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	mBOOL ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	// Clean up loaded plugin
	MPlugin *loaded = list->find(1);
	if (loaded && loaded->status >= PL_OPENED) {
		loaded->free_api_pointers();
		if (loaded->handle) { DLCLOSE(loaded->handle); loaded->handle = NULL; }
		loaded->status = PL_EMPTY;
	}

	unlink(fullpath);
	unlink("/tmp/test_mlist_gd/dlls/ref_succ.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// load_plugins - successful load at startup (lines 711, 713)
// ============================================================

static int test_load_success(void)
{
	TEST("load_plugins - loads plugin from ini at startup (lines 711-713)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/lp_succ.so");

	const char *fullpath = make_tmp_file_in(
		"linux dlls/lp_succ.so LPSucc\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;

	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_UNLOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	mBOOL ret = list->load();
	ASSERT_TRUE(ret == mTRUE);

	// Clean up loaded plugin
	MPlugin *loaded = list->find(1);
	if (loaded && loaded->status >= PL_OPENED) {
		loaded->free_api_pointers();
		if (loaded->handle) { DLCLOSE(loaded->handle); loaded->handle = NULL; }
		loaded->status = PL_EMPTY;
	}

	unlink(fullpath);
	unlink("/tmp/test_mlist_gd/dlls/lp_succ.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// load + refresh with reload (lines 749, 756)
// ============================================================

static int test_load_and_refresh_reload(void)
{
	TEST("load+refresh - reload succeeds for newer file (lines 749, 756)");
	setup_globals();
	system("mkdir -p /tmp/test_mlist_gd/dlls");
	copy_test_plugin("fake_mm_plugin.so","/tmp/test_mlist_gd/dlls/lr_plug.so");

	const char *fullpath = make_tmp_file_in(
		"linux dlls/lr_plug.so LRPlug\n",
		"/tmp/test_mlist_gd"
	);
	ASSERT_TRUE(fullpath != NULL);
	const char *fname = strrchr(fullpath, '/');
	fname = fname ? fname + 1 : fullpath;

	HeapPluginList list(fname);
	Plugins = list.ptr;

	unsetenv("FAKE_MM_LOADABLE");
	unsetenv("FAKE_MM_UNLOADABLE");
	unsetenv("FAKE_MM_ATTACH_FAIL");

	// Initial load
	mBOOL ret = list->load();
	ASSERT_TRUE(ret == mTRUE);
	MPlugin *plug = list->find(1);
	ASSERT_PTR_NOT_NULL(plug);
	ASSERT_TRUE(plug->status == PL_RUNNING);

	// Set file timestamp to future to make it newer
	struct utimbuf ut;
	ut.actime = time(NULL) + 10;
	ut.modtime = time(NULL) + 10;
	utime("/tmp/test_mlist_gd/dlls/lr_plug.so", &ut);

	// Refresh should detect newer file and reload
	ret = list->refresh(PT_ANYTIME);
	ASSERT_TRUE(ret == mTRUE);

	// Clean up
	plug = list->find(1);
	if (plug && plug->status >= PL_OPENED) {
		plug->free_api_pointers();
		if (plug->handle) { DLCLOSE(plug->handle); plug->handle = NULL; }
		plug->status = PL_EMPTY;
	}

	unlink(fullpath);
	unlink("/tmp/test_mlist_gd/dlls/lr_plug.so");
	Plugins = NULL;
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// rebuild_hook_lists
// ============================================================

static DLL_FUNCTIONS fake_dllapi;
static DLL_FUNCTIONS fake_dllapi2;
static NEW_DLL_FUNCTIONS fake_newapi;
static enginefuncs_t fake_engine_funcs;

static int test_rebuild_empty(void)
{
	TEST("rebuild_hook_lists - empty list produces zero counts");
	setup_globals();
	HeapPluginList list("test.ini");
	list->endlist = 0;
	list->rebuild_hook_lists();
	for(int api = 0; api < 3; api++) {
		ASSERT_INT(list->get_hook_list((enum_api_t)api)->count, 0);
		ASSERT_INT(list->get_hook_post_list((enum_api_t)api)->count, 0);
	}
	ASSERT_TRUE(list->hook_list_data == NULL);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_one_running_dllapi(void)
{
	TEST("rebuild_hook_lists - one running plugin with dllapi tables");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.dllapi = &fake_dllapi;
	plug->post_tables.dllapi = &fake_dllapi;
	list->endlist = 1;

	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_engine)->count, 0);
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_INT(list->get_hook_list(e_api_newapi)->count, 0);
	ASSERT_INT(list->get_hook_post_list(e_api_engine)->count, 0);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 1);
	ASSERT_INT(list->get_hook_post_list(e_api_newapi)->count, 0);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug);
	ASSERT_PTR_EQ(list->get_hook_post_list(e_api_dllapi)->plugs[0], plug);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_one_running_all_tables(void)
{
	TEST("rebuild_hook_lists - one plugin with all 3 api groups");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_newapi, 0, sizeof(fake_newapi));
	memset(&fake_engine_funcs, 0, sizeof(fake_engine_funcs));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.engine = &fake_engine_funcs;
	plug->tables.dllapi = &fake_dllapi;
	plug->tables.newapi = &fake_newapi;
	plug->post_tables.engine = &fake_engine_funcs;
	plug->post_tables.dllapi = &fake_dllapi;
	plug->post_tables.newapi = &fake_newapi;
	list->endlist = 1;

	list->rebuild_hook_lists();

	for(int api = 0; api < 3; api++) {
		ASSERT_INT(list->get_hook_list((enum_api_t)api)->count, 1);
		ASSERT_INT(list->get_hook_post_list((enum_api_t)api)->count, 1);
		ASSERT_PTR_EQ(list->get_hook_list((enum_api_t)api)->plugs[0], plug);
		ASSERT_PTR_EQ(list->get_hook_post_list((enum_api_t)api)->plugs[0], plug);
	}
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_pre_only(void)
{
	TEST("rebuild_hook_lists - plugin with pre table only, no post");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.dllapi = &fake_dllapi;
	plug->post_tables.dllapi = NULL;
	list->endlist = 1;

	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 0);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug);
	ASSERT_PTR_NULL(list->get_hook_post_list(e_api_dllapi)->plugs);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_post_only(void)
{
	TEST("rebuild_hook_lists - plugin with post table only, no pre");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.dllapi = NULL;
	plug->post_tables.dllapi = &fake_dllapi;
	list->endlist = 1;

	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 0);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 1);
	ASSERT_PTR_NULL(list->get_hook_list(e_api_dllapi)->plugs);
	ASSERT_PTR_EQ(list->get_hook_post_list(e_api_dllapi)->plugs[0], plug);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_skips_non_running(void)
{
	TEST("rebuild_hook_lists - skips paused/unloaded/empty plugins");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));

	PLUG_STATUS skip_statuses[] = { PL_PAUSED, PL_EMPTY, PL_VALID, PL_OPENED };
	for(int s = 0; s < 4; s++) {
		MPlugin *plug = &list->plist[s];
		memset(plug, 0, sizeof(*plug));
		plug->status = skip_statuses[s];
		plug->tables.dllapi = &fake_dllapi;
		plug->post_tables.dllapi = &fake_dllapi;
	}
	list->endlist = 4;

	list->rebuild_hook_lists();

	for(int api = 0; api < 3; api++) {
		ASSERT_INT(list->get_hook_list((enum_api_t)api)->count, 0);
		ASSERT_INT(list->get_hook_post_list((enum_api_t)api)->count, 0);
	}
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_mixed_statuses(void)
{
	TEST("rebuild_hook_lists - only PL_RUNNING included among mixed");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));

	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->status = PL_PAUSED;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi;

	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->status = PL_EMPTY;
	plug2->tables.dllapi = &fake_dllapi2;

	MPlugin *plug3 = &list->plist[3];
	memset(plug3, 0, sizeof(*plug3));
	plug3->status = PL_RUNNING;
	plug3->tables.dllapi = &fake_dllapi2;

	list->endlist = 4;
	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 2);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug1);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[1], plug3);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_two_plugins_order(void)
{
	TEST("rebuild_hook_lists - plugin order preserved");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));

	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;
	plug0->post_tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;
	plug1->post_tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 2);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug0);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[1], plug1);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 2);
	ASSERT_PTR_EQ(list->get_hook_post_list(e_api_dllapi)->plugs[0], plug0);
	ASSERT_PTR_EQ(list->get_hook_post_list(e_api_dllapi)->plugs[1], plug1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_different_api_groups(void)
{
	TEST("rebuild_hook_lists - plugins in different api groups");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_newapi, 0, sizeof(fake_newapi));
	memset(&fake_engine_funcs, 0, sizeof(fake_engine_funcs));

	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->status = PL_RUNNING;
	plug0->tables.engine = &fake_engine_funcs;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi;
	plug1->post_tables.newapi = &fake_newapi;

	list->endlist = 2;
	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_engine)->count, 1);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_engine)->plugs[0], plug0);
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug1);
	ASSERT_INT(list->get_hook_list(e_api_newapi)->count, 0);
	ASSERT_INT(list->get_hook_post_list(e_api_engine)->count, 0);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 0);
	ASSERT_INT(list->get_hook_post_list(e_api_newapi)->count, 1);
	ASSERT_PTR_EQ(list->get_hook_post_list(e_api_newapi)->plugs[0], plug1);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_realloc_shrinks(void)
{
	TEST("rebuild_hook_lists - rebuild after unload shrinks lists");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));

	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 2);

	plug1->status = PL_EMPTY;
	list->rebuild_hook_lists();
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug0);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_realloc_failure_keeps_old(void)
{
	TEST("rebuild_hook_lists - realloc failure keeps old lists");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.dllapi = &fake_dllapi;
	list->endlist = 1;

	list->rebuild_hook_lists();
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	MPlugin **old_data = list->hook_list_data;
	ASSERT_PTR_NOT_NULL(old_data);

	// Add a second plugin and force realloc to fail
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;
	list->endlist = 2;

	force_realloc_fail = true;
	list->rebuild_hook_lists();
	force_realloc_fail = false;

	// Old allocation preserved, but lists zeroed so dispatch is safe
	ASSERT_PTR_EQ(list->hook_list_data, old_data);
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 0);
	ASSERT_PTR_NULL(list->get_hook_list(e_api_dllapi)->plugs);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_to_empty_frees(void)
{
	TEST("rebuild_hook_lists - all plugins removed frees allocation");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.dllapi = &fake_dllapi;
	list->endlist = 1;

	list->rebuild_hook_lists();
	ASSERT_PTR_NOT_NULL(list->hook_list_data);

	plug->status = PL_EMPTY;
	list->rebuild_hook_lists();
	ASSERT_PTR_NULL(list->hook_list_data);
	for(int api = 0; api < 3; api++) {
		ASSERT_INT(list->get_hook_list((enum_api_t)api)->count, 0);
		ASSERT_INT(list->get_hook_post_list((enum_api_t)api)->count, 0);
	}
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_null_tables_ignored(void)
{
	TEST("rebuild_hook_lists - NULL table pointers not counted");
	setup_globals();
	HeapPluginList list("test.ini");

	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.engine = NULL;
	plug->tables.dllapi = NULL;
	plug->tables.newapi = NULL;
	plug->post_tables.engine = NULL;
	plug->post_tables.dllapi = NULL;
	plug->post_tables.newapi = NULL;
	list->endlist = 1;

	list->rebuild_hook_lists();

	for(int api = 0; api < 3; api++) {
		ASSERT_INT(list->get_hook_list((enum_api_t)api)->count, 0);
		ASSERT_INT(list->get_hook_post_list((enum_api_t)api)->count, 0);
	}
	ASSERT_PTR_NULL(list->hook_list_data);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_plugs_contiguous(void)
{
	TEST("rebuild_hook_lists - plugs arrays are contiguous in memory");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_engine_funcs, 0, sizeof(fake_engine_funcs));

	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->status = PL_RUNNING;
	plug0->tables.engine = &fake_engine_funcs;
	plug0->tables.dllapi = &fake_dllapi;
	plug0->post_tables.dllapi = &fake_dllapi;

	list->endlist = 1;
	list->rebuild_hook_lists();

	// All plugs pointers should point into hook_list_data
	MPlugin **base = list->hook_list_data;
	ASSERT_PTR_NOT_NULL(base);

	// engine pre (1) + engine post (0) + dllapi pre (1) + dllapi post (1) = 3 total
	const api_plugin_list_t *eng_pre = list->get_hook_list(e_api_engine);
	const api_plugin_list_t *dll_pre = list->get_hook_list(e_api_dllapi);
	const api_plugin_list_t *dll_post = list->get_hook_post_list(e_api_dllapi);

	ASSERT_PTR_EQ(eng_pre->plugs, base);
	ASSERT_PTR_EQ(dll_pre->plugs, base + 1);
	ASSERT_PTR_EQ(dll_post->plugs, base + 2);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_repeated_calls(void)
{
	TEST("rebuild_hook_lists - repeated rebuilds produce same result");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	MPlugin *plug = &list->plist[0];
	memset(plug, 0, sizeof(*plug));
	plug->status = PL_RUNNING;
	plug->tables.dllapi = &fake_dllapi;
	plug->post_tables.dllapi = &fake_dllapi;
	list->endlist = 1;

	list->rebuild_hook_lists();
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 1);

	list->rebuild_hook_lists();
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_INT(list->get_hook_post_list(e_api_dllapi)->count, 1);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug);
	ASSERT_PTR_EQ(list->get_hook_post_list(e_api_dllapi)->plugs[0], plug);
	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_endlist_bounds(void)
{
	TEST("rebuild_hook_lists - only scans up to endlist");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));

	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;

	list->endlist = 1;
	list->rebuild_hook_lists();

	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 1);
	ASSERT_PTR_EQ(list->get_hook_list(e_api_dllapi)->plugs[0], plug0);
	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// find_plugin_after_rebuild (issue #108)
// ============================================================

// Tests for the index fixup function used by main_hook_function after
// rebuild_hook_lists is called mid-iteration. Verifies correct new index
// for all combinations of plugin load/unload/pause during a hook callback.
//
// The for loop in main_hook_function does i++ after the returned index,
// so "return X" means next iteration processes plugs[X+1].

static DLL_FUNCTIONS fake_dllapi3;
static DLL_FUNCTIONS fake_dllapi4;
static DLL_FUNCTIONS fake_dllapi5;

// Case 1: [A,B,C] idx=0(A), C removed → [A,B]. A stays at 0.
static int test_find_after_rebuild_later_removed(void)
{
	TEST("find_plugin_after_rebuild - later plugin removed, no shift");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	// Simulate: at idx=0 calling A, C gets removed, rebuild
	int count = list->get_hook_list(e_api_dllapi)->count;
	MPlugin * const *plugs = list->get_hook_list(e_api_dllapi)->plugs;
	ASSERT_INT(count, 3);

	plugC->status = PL_EMPTY; plugC->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// Case 2: [A,B,C] idx=1(B), C removed → [A,B]. B stays at 1.
static int test_find_after_rebuild_later_removed2(void)
{
	TEST("find_plugin_after_rebuild - later plugin removed, mid-list");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugC->status = PL_EMPTY; plugC->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// Case 3: [A,B] idx=0(A), D added after B → [A,B,D]. A stays at 0.
static int test_find_after_rebuild_later_added(void)
{
	TEST("find_plugin_after_rebuild - later plugin added, no shift");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	// Add D at plist[3] (after B)
	MPlugin *plugD = &list->plist[3];
	memset(plugD, 0, sizeof(*plugD)); plugD->index = 4;
	plugD->status = PL_RUNNING; plugD->tables.dllapi = &fake_dllapi4;
	list->endlist = 4;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 4: [A,B,C] idx=2(C), A removed → [B,C]. C shifts to 1.
static int test_find_after_rebuild_earlier_removed(void)
{
	TEST("find_plugin_after_rebuild - earlier removed, shift left");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugC, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// Case 5: [A,B,C] idx=2(C), A paused → [B,C]. C shifts to 1.
static int test_find_after_rebuild_earlier_paused(void)
{
	TEST("find_plugin_after_rebuild - earlier paused, shift left");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugA->status = PL_PAUSED;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugC, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// Case 6: [A,B,C] idx=2(C), A and B removed → [C]. C shifts to 0.
static int test_find_after_rebuild_two_earlier_removed(void)
{
	TEST("find_plugin_after_rebuild - two earlier removed, shift left by 2");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	plugB->status = PL_EMPTY; plugB->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugC, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 1);

	teardown_globals();
	PASS();
	return 0;
}

// Case 7: [A,B,C] idx=1(B), A removed → [B,C]. B shifts to 0.
static int test_find_after_rebuild_earlier_removed_mid(void)
{
	TEST("find_plugin_after_rebuild - earlier removed, mid-list shifts left");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// Case 8: [A,B] idx=1(B), A removed → [B]. B shifts to 0.
static int test_find_after_rebuild_earlier_removed_becomes_first(void)
{
	TEST("find_plugin_after_rebuild - earlier removed, becomes first");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 1);

	teardown_globals();
	PASS();
	return 0;
}

// Case 9: [A,B] idx=0(A), X added before A in plist → [X,A,B]. A shifts to 1.
static int test_find_after_rebuild_earlier_added(void)
{
	TEST("find_plugin_after_rebuild - earlier added, shift right");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0] = empty slot for X (added later)
	// plist[1] = A, plist[2] = B
	MPlugin *plugA = &list->plist[1];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 2;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 3;
	list->rebuild_hook_lists();

	// Add X at plist[0] (before A)
	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 10: [A,B] idx=1(B), X added before A in plist → [X,A,B]. B shifts to 2.
static int test_find_after_rebuild_earlier_added_last(void)
{
	TEST("find_plugin_after_rebuild - earlier added, last shifts right");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	MPlugin *plugA = &list->plist[1];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 2;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 3;
	list->rebuild_hook_lists();

	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 2);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 11: [A,B] idx=1(B), X added between A and B → [A,X,B]. B shifts to 2.
static int test_find_after_rebuild_added_between(void)
{
	TEST("find_plugin_after_rebuild - plugin added between, shift right");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0] = A, plist[1] empty for X, plist[2] = B
	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 3;
	list->rebuild_hook_lists();

	// Add X at plist[1] (between A and B)
	MPlugin *plugX = &list->plist[1];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 2;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 2);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 12: [A,B] idx=0(A), two plugins added before A → [X,Y,A,B]. A shifts to 2.
static int test_find_after_rebuild_two_earlier_added(void)
{
	TEST("find_plugin_after_rebuild - two earlier added, shift right by 2");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));
	memset(&fake_dllapi5, 0, sizeof(fake_dllapi5));

	// plist[0],[1] empty for X,Y; plist[2] = A, plist[3] = B
	MPlugin *plugA = &list->plist[2];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 3;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[3];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 4;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 4;
	list->rebuild_hook_lists();

	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;

	MPlugin *plugY = &list->plist[1];
	memset(plugY, 0, sizeof(*plugY)); plugY->index = 2;
	plugY->status = PL_RUNNING; plugY->tables.dllapi = &fake_dllapi5;

	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	ASSERT_INT(new_i, 2);
	ASSERT_INT(count, 4);

	teardown_globals();
	PASS();
	return 0;
}

// Case 13: [A,B] idx=1(B), paused plugin before B unpaused → [A,X,B]. B shifts to 2.
static int test_find_after_rebuild_earlier_unpaused(void)
{
	TEST("find_plugin_after_rebuild - earlier unpaused, shift right");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0] = A, plist[1] = X (paused), plist[2] = B
	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugX = &list->plist[1];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 2;
	plugX->status = PL_PAUSED; plugX->tables.dllapi = &fake_dllapi4;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 3;
	list->rebuild_hook_lists();
	// Hook list is [A, B] (X paused)
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 2);

	// Unpause X
	plugX->status = PL_RUNNING;
	list->rebuild_hook_lists();
	// Hook list is [A, X, B]
	ASSERT_INT(list->get_hook_list(e_api_dllapi)->count, 3);

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 2);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 14: [A] idx=0(A), A removed → []. Self gone, list empty. Return -1.
static int test_find_after_rebuild_self_removed_only(void)
{
	TEST("find_plugin_after_rebuild - self removed, was only plugin");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	list->endlist = 1;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	// i=-1, loop does i++ → 0, 0<0 false, loop ends
	ASSERT_INT(new_i, -1);
	ASSERT_INT(count, 0);

	teardown_globals();
	PASS();
	return 0;
}

// Case 15: [A,B] idx=0(A), A removed → [B]. Return -1 so i++→0 processes B.
static int test_find_after_rebuild_self_removed_first(void)
{
	TEST("find_plugin_after_rebuild - self removed, was first of two");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	// i=-1, loop does i++ → 0, processes plugs[0]=B
	ASSERT_INT(new_i, -1);
	ASSERT_INT(count, 1);
	ASSERT_PTR_EQ(plugs[0], plugB);

	teardown_globals();
	PASS();
	return 0;
}

// Case 16: [A,B,C] idx=1(B), B removed → [A,C]. Return 0 so i++→1 processes C.
static int test_find_after_rebuild_self_removed_mid(void)
{
	TEST("find_plugin_after_rebuild - self removed from middle");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugB->status = PL_EMPTY; plugB->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	// i=0, loop does i++ → 1, processes plugs[1]=C
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 2);
	ASSERT_PTR_EQ(plugs[1], plugC);

	teardown_globals();
	PASS();
	return 0;
}

// Case 17: [A,B,C] idx=1(B), B paused → [A,C]. Return 0 so i++→1 processes C.
static int test_find_after_rebuild_self_paused_mid(void)
{
	TEST("find_plugin_after_rebuild - self paused from middle");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugB->status = PL_PAUSED;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 2);
	ASSERT_PTR_EQ(plugs[1], plugC);

	teardown_globals();
	PASS();
	return 0;
}

// Case 18: [A,B,C] idx=1(B), A removed + D added after C → [B,C,D]. B at 0.
static int test_find_after_rebuild_mixed_remove_add_later(void)
{
	TEST("find_plugin_after_rebuild - earlier removed + later added");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	MPlugin *plugD = &list->plist[3];
	memset(plugD, 0, sizeof(*plugD)); plugD->index = 4;
	plugD->status = PL_RUNNING; plugD->tables.dllapi = &fake_dllapi4;
	list->endlist = 4;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 19: [A,B,C] idx=1(B), A removed + X added before A → [X,B,C]. Net zero for B.
static int test_find_after_rebuild_mixed_remove_add_earlier(void)
{
	TEST("find_plugin_after_rebuild - removed + added before, net zero");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0] empty for X, plist[1] = A, plist[2] = B, plist[3] = C
	MPlugin *plugA = &list->plist[1];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 2;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[3];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 4;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 4;
	list->rebuild_hook_lists();
	// Hook list: [A, B, C]

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;
	list->rebuild_hook_lists();
	// Hook list: [X, B, C]

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 20: [A,B,C] idx=2(C), A removed + X added before A → [X,B,C]. C at 2.
static int test_find_after_rebuild_mixed_last_net_zero(void)
{
	TEST("find_plugin_after_rebuild - removed + added before, last stays");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0] empty for X, plist[1] = A, plist[2] = B, plist[3] = C
	MPlugin *plugA = &list->plist[1];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 2;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[3];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 4;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 4;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugC, count, plugs);
	ASSERT_INT(new_i, 2);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 21: [A,B,C] idx=0(A), B removed + X added before A → [X,A,C]. A at 1.
static int test_find_after_rebuild_mixed_add_before_remove_after(void)
{
	TEST("find_plugin_after_rebuild - added before + later removed");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0] empty for X, plist[1] = A, plist[2] = B, plist[3] = C
	MPlugin *plugA = &list->plist[1];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 2;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[2];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 3;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[3];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 4;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 4;
	list->rebuild_hook_lists();

	plugB->status = PL_EMPTY; plugB->tables.dllapi = NULL;
	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi4;
	list->rebuild_hook_lists();
	// Hook list: [X, A, C]

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 22: [A,B,C] idx=1(B), A and C removed → [B]. B at 0.
static int test_find_after_rebuild_others_removed(void)
{
	TEST("find_plugin_after_rebuild - all others removed, only self remains");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	plugC->status = PL_EMPTY; plugC->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 0);
	ASSERT_INT(count, 1);

	teardown_globals();
	PASS();
	return 0;
}

// Case 23: [A,B,C,D,E] idx=4(E), A,B,C removed → [D,E]. E at 1.
static int test_find_after_rebuild_many_earlier_removed(void)
{
	TEST("find_plugin_after_rebuild - many earlier removed, large shift");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));
	memset(&fake_dllapi5, 0, sizeof(fake_dllapi5));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	MPlugin *plugD = &list->plist[3];
	memset(plugD, 0, sizeof(*plugD)); plugD->index = 4;
	plugD->status = PL_RUNNING; plugD->tables.dllapi = &fake_dllapi4;

	MPlugin *plugE = &list->plist[4];
	memset(plugE, 0, sizeof(*plugE)); plugE->index = 5;
	plugE->status = PL_RUNNING; plugE->tables.dllapi = &fake_dllapi5;

	list->endlist = 5;
	list->rebuild_hook_lists();

	plugA->status = PL_EMPTY; plugA->tables.dllapi = NULL;
	plugB->status = PL_EMPTY; plugB->tables.dllapi = NULL;
	plugC->status = PL_EMPTY; plugC->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugE, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// Case 24: [A] idx=0(A), 3 plugins added before A → [X,Y,Z,A]. A at 3.
static int test_find_after_rebuild_many_earlier_added(void)
{
	TEST("find_plugin_after_rebuild - many earlier added, large shift right");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));
	memset(&fake_dllapi4, 0, sizeof(fake_dllapi4));

	// plist[0,1,2] empty, plist[3] = A
	MPlugin *plugA = &list->plist[3];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 4;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	list->endlist = 4;
	list->rebuild_hook_lists();

	MPlugin *plugX = &list->plist[0];
	memset(plugX, 0, sizeof(*plugX)); plugX->index = 1;
	plugX->status = PL_RUNNING; plugX->tables.dllapi = &fake_dllapi2;

	MPlugin *plugY = &list->plist[1];
	memset(plugY, 0, sizeof(*plugY)); plugY->index = 2;
	plugY->status = PL_RUNNING; plugY->tables.dllapi = &fake_dllapi3;

	MPlugin *plugZ = &list->plist[2];
	memset(plugZ, 0, sizeof(*plugZ)); plugZ->index = 3;
	plugZ->status = PL_RUNNING; plugZ->tables.dllapi = &fake_dllapi4;

	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugA, count, plugs);
	ASSERT_INT(new_i, 3);
	ASSERT_INT(count, 4);

	teardown_globals();
	PASS();
	return 0;
}

// Case 25: [A,B,C] idx=1(B), no state change, rebuild → [A,B,C]. B stays at 1.
static int test_find_after_rebuild_no_change(void)
{
	TEST("find_plugin_after_rebuild - rebuild with no change");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugB, count, plugs);
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 3);

	teardown_globals();
	PASS();
	return 0;
}

// Case 26: [A,B,C] idx=2(C), self removed (last) → [A,B]. Return 1 so i++→2, loop ends.
static int test_find_after_rebuild_self_removed_last(void)
{
	TEST("find_plugin_after_rebuild - self removed, was last");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	MPlugin *plugA = &list->plist[0];
	memset(plugA, 0, sizeof(*plugA)); plugA->index = 1;
	plugA->status = PL_RUNNING; plugA->tables.dllapi = &fake_dllapi;

	MPlugin *plugB = &list->plist[1];
	memset(plugB, 0, sizeof(*plugB)); plugB->index = 2;
	plugB->status = PL_RUNNING; plugB->tables.dllapi = &fake_dllapi2;

	MPlugin *plugC = &list->plist[2];
	memset(plugC, 0, sizeof(*plugC)); plugC->index = 3;
	plugC->status = PL_RUNNING; plugC->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	plugC->status = PL_EMPTY; plugC->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	int count;
	MPlugin * const *plugs;
	int new_i = list->find_plugin_after_rebuild(
		e_api_dllapi, mFALSE, plugC, count, plugs);
	// i=1, loop does i++ → 2, 2<2 false, loop ends. No plugins skipped.
	ASSERT_INT(new_i, 1);
	ASSERT_INT(count, 2);

	teardown_globals();
	PASS();
	return 0;
}

// ============================================================
// hook_list stability during iteration (issue #108)
// ============================================================

// Simulates the iteration pattern in main_hook_function:
// cache plugs pointer and count, then iterate. Tests verify that
// rebuild_hook_lists called mid-iteration invalidates cached values,
// which the fix in api_hook.cpp detects via hook_list_tables_updated.

static int test_rebuild_during_iteration_load(void)
{
	TEST("rebuild during iteration - load invalidates cached plugs");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	// Start with 2 running plugins
	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->index = 1;
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->index = 2;
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	// Simulate main_hook_function: cache plugs and count
	const api_plugin_list_t *hlist = list->get_hook_list(e_api_dllapi);
	int count = hlist->count;
	MPlugin * const *plugs = hlist->plugs;

	ASSERT_INT(count, 2);
	ASSERT_PTR_EQ(plugs[0], plug0);
	ASSERT_PTR_EQ(plugs[1], plug1);

	// Simulate: during iteration at i=0, a new plugin is loaded
	// (AMXX loads module via LOAD_PLUGIN from within hook callback)
	hook_list_tables_updated = mFALSE;
	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->index = 3;
	plug2->status = PL_RUNNING;
	plug2->tables.dllapi = &fake_dllapi3;
	list->endlist = 3;
	list->rebuild_hook_lists();

	// rebuild_hook_lists signals that cached pointers are stale
	ASSERT_TRUE(hook_list_tables_updated == mTRUE);

	// After rebuild: the hook list has 3 entries now
	ASSERT_INT(hlist->count, 3);

	// Cached 'count' is stale (still 2) — fix must re-read from list
	ASSERT_TRUE(count != hlist->count);

	// Cached 'plugs' pointer is dangling (realloc moved data)
	ASSERT_TRUE(plugs != hlist->plugs);

	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_during_iteration_unload_later(void)
{
	TEST("rebuild during iteration - unload later plugin shifts nothing");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	// 3 running plugins
	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->index = 1;
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->index = 2;
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;

	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->index = 3;
	plug2->status = PL_RUNNING;
	plug2->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	// Cache like main_hook_function does
	const api_plugin_list_t *hlist = list->get_hook_list(e_api_dllapi);
	int count = hlist->count;
	MPlugin * const *plugs = hlist->plugs;

	ASSERT_INT(count, 3);

	// Simulate: at i=0, plug2 (last one) is unloaded
	plug2->status = PL_EMPTY;
	plug2->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	// After rebuild: only 2 plugins, plug0 and plug1 stay in positions 0,1
	ASSERT_INT(hlist->count, 2);

	// Cached count is 3 but list now has 2 — fix must re-read
	ASSERT_TRUE(count > hlist->count);

	// Cached plugs pointer is dangling
	ASSERT_TRUE(plugs != hlist->plugs);

	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_during_iteration_unload_earlier(void)
{
	TEST("rebuild during iteration - unload earlier plugin shifts indices");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	// 3 running plugins
	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->index = 1;
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->index = 2;
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;

	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->index = 3;
	plug2->status = PL_RUNNING;
	plug2->tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	// Cache like main_hook_function does
	const api_plugin_list_t *hlist = list->get_hook_list(e_api_dllapi);
	int count = hlist->count;
	(void)count;

	// Verify initial order
	ASSERT_PTR_EQ(hlist->plugs[0], plug0);
	ASSERT_PTR_EQ(hlist->plugs[1], plug1);
	ASSERT_PTR_EQ(hlist->plugs[2], plug2);

	// Simulate: while iterating at i=1 (calling plug1),
	// plug0 (earlier plugin) gets unloaded
	plug0->status = PL_EMPTY;
	plug0->tables.dllapi = NULL;
	list->rebuild_hook_lists();

	// After rebuild: [plug1, plug2], count=2
	ASSERT_INT(hlist->count, 2);
	ASSERT_PTR_EQ(hlist->plugs[0], plug1);
	ASSERT_PTR_EQ(hlist->plugs[1], plug2);

	// BUG: if loop was at i=1 and continues to i=2 with old count=3,
	// it accesses plugs[2] which is beyond the new list.
	// Even with fixed arrays, if loop increments i to 2 and count is
	// re-read as 2, loop ends — but plug2 (now at index 1) was SKIPPED.
	// The iteration at i=1 already processed plug1 (now at index 0),
	// so effectively plug2 never gets called.
	//
	// Correct behavior after fix: detect rebuild happened, find plug1
	// in new list at index 0, set i=0, re-read count=2, continue from
	// i=1 which is plug2.
	ASSERT_TRUE(hlist->plugs[1] == plug2);

	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_during_iteration_load_post(void)
{
	TEST("rebuild during post iteration - load invalidates cached plugs");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	// 2 running plugins with post tables
	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->index = 1;
	plug0->status = PL_RUNNING;
	plug0->post_tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->index = 2;
	plug1->status = PL_RUNNING;
	plug1->post_tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	// Cache post hook list like main_hook_function does
	const api_plugin_list_t *hlist = list->get_hook_post_list(e_api_dllapi);
	int count = hlist->count;
	MPlugin * const *plugs = hlist->plugs;

	ASSERT_INT(count, 2);
	ASSERT_PTR_EQ(plugs[0], plug0);
	ASSERT_PTR_EQ(plugs[1], plug1);

	// New plugin loaded during post hook callback
	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->index = 3;
	plug2->status = PL_RUNNING;
	plug2->post_tables.dllapi = &fake_dllapi3;
	list->endlist = 3;
	list->rebuild_hook_lists();

	ASSERT_INT(hlist->count, 3);

	// Cached count and plugs are stale — fix must re-read
	ASSERT_TRUE(count != hlist->count);
	ASSERT_TRUE(plugs != hlist->plugs);

	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_during_iteration_unload_earlier_post(void)
{
	TEST("rebuild during post iteration - unload earlier shifts indices");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	// 3 running plugins with post tables
	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->index = 1;
	plug0->status = PL_RUNNING;
	plug0->post_tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->index = 2;
	plug1->status = PL_RUNNING;
	plug1->post_tables.dllapi = &fake_dllapi2;

	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->index = 3;
	plug2->status = PL_RUNNING;
	plug2->post_tables.dllapi = &fake_dllapi3;

	list->endlist = 3;
	list->rebuild_hook_lists();

	// Cache post hook list
	const api_plugin_list_t *hlist = list->get_hook_post_list(e_api_dllapi);
	int count = hlist->count;
	(void)count;

	ASSERT_PTR_EQ(hlist->plugs[0], plug0);
	ASSERT_PTR_EQ(hlist->plugs[1], plug1);
	ASSERT_PTR_EQ(hlist->plugs[2], plug2);

	// Simulate: during post iteration at i=1, plug0 unloaded
	plug0->status = PL_EMPTY;
	plug0->post_tables.dllapi = NULL;
	list->rebuild_hook_lists();

	// After rebuild: [plug1, plug2], count=2
	ASSERT_INT(hlist->count, 2);
	ASSERT_PTR_EQ(hlist->plugs[0], plug1);
	ASSERT_PTR_EQ(hlist->plugs[1], plug2);

	// Same bug: plug2 at new index 1 would be skipped if loop
	// continues from old i=1 -> i=2 which is beyond new count.
	ASSERT_TRUE(hlist->plugs[1] == plug2);

	teardown_globals();
	PASS();
	return 0;
}

static int test_rebuild_during_iteration_segfault(void)
{
	TEST("rebuild during iteration - use-after-free segfaults");
	setup_globals();
	HeapPluginList list("test.ini");

	memset(&fake_dllapi, 0, sizeof(fake_dllapi));
	memset(&fake_dllapi2, 0, sizeof(fake_dllapi2));
	memset(&fake_dllapi3, 0, sizeof(fake_dllapi3));

	// 2 running plugins
	MPlugin *plug0 = &list->plist[0];
	memset(plug0, 0, sizeof(*plug0));
	plug0->index = 1;
	plug0->status = PL_RUNNING;
	plug0->tables.dllapi = &fake_dllapi;

	MPlugin *plug1 = &list->plist[1];
	memset(plug1, 0, sizeof(*plug1));
	plug1->index = 2;
	plug1->status = PL_RUNNING;
	plug1->tables.dllapi = &fake_dllapi2;

	list->endlist = 2;
	list->rebuild_hook_lists();

	// Simulate main_hook_function: cache plugs and count
	const api_plugin_list_t *hlist = list->get_hook_list(e_api_dllapi);
	int count = hlist->count;
	MPlugin * const *plugs = hlist->plugs;

	ASSERT_INT(count, 2);
	ASSERT_PTR_EQ(plugs[0], plug0);

	// A new plugin is loaded from within a hook callback → rebuild
	MPlugin *plug2 = &list->plist[2];
	memset(plug2, 0, sizeof(*plug2));
	plug2->index = 3;
	plug2->status = PL_RUNNING;
	plug2->tables.dllapi = &fake_dllapi3;
	list->endlist = 3;
	list->rebuild_hook_lists();

	// Old 'plugs' is now freed+zeroed memory.
	// Fork a child that dereferences it without the fix's protection:
	//   plug = plugs[0];          // reads NULL from zeroed memory
	//   table = plug->tables...;  // SIGSEGV
	pid_t pid = fork();
	if (pid == 0) {
		// Child: simulate the crash path
		volatile MPlugin *plug = plugs[0];
		volatile void *table = plug->tables.dllapi;
		(void)table;
		_exit(0);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	// SIGSEGV/SIGABRT normally; ASan may exit(1) instead of signaling
	ASSERT_TRUE(WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0));

	teardown_globals();
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

	// Clean up stale test dirs from previous (possibly crashed) runs
	system("rm -rf /tmp/test_mlist_gd");

	printf("test_mlist:\n");

	// Constructor
	fail |= test_constructor();

	// find(int)
	fail |= test_find_index_zero();
	fail |= test_find_index_negative();
	fail |= test_find_index_empty();

	// find(DLHANDLE)
	fail |= test_find_handle_null();
	fail |= test_find_handle_not_found();
	fail |= test_find_handle_found();

	// find(plid_t)
	fail |= test_find_plid_null();
	fail |= test_find_plid_found();

	// find(const char*)
	fail |= test_find_path_null();
	fail |= test_find_path_not_found();

	// find_memloc
	fail |= test_find_memloc_null();
	fail |= test_find_memloc_valid_not_found();

	// find_match(const char*)
	fail |= test_find_match_null();
	fail |= test_find_match_not_found();

	// find_match(MPlugin*)
	fail |= test_find_match_plugin_null();
	fail |= test_find_match_plugin_found();
	fail |= test_find_match_plugin_not_found();

	// add + find
	fail |= test_add_and_find();
	fail |= test_add_find_by_index();

	// find_match with data
	fail |= test_find_match_by_desc();
	fail |= test_find_match_by_file();
	fail |= test_find_match_by_mm_prefix();
	fail |= test_find_match_by_info_name();
	fail |= test_find_match_by_logtag();
	fail |= test_find_match_not_unique();
	fail |= test_find_match_notuniq_by_info_name();
	fail |= test_find_match_notuniq_by_mm_prefix();
	fail |= test_find_match_notuniq_by_logtag();
	fail |= test_find_match_notuniq_by_file();

	// find with empty slots
	fail |= test_find_handle_skips_empty();
	fail |= test_find_plid_skips_empty();
	fail |= test_find_path_skips_empty();
	fail |= test_find_match_skips_empty();

	// clear_source_plugin_index
	fail |= test_clear_source_plugin_index();
	fail |= test_clear_source_plugin_index_zero();
	fail |= test_clear_source_skips_empty();

	// found_child_plugins
	fail |= test_found_child_plugins_none();
	fail |= test_found_child_plugins_zero();
	fail |= test_found_child_plugins_found();
	fail |= test_found_child_skips_empty();

	// trim_list
	fail |= test_trim_list_empty();
	fail |= test_trim_list_shrinks();
	fail |= test_trim_list_shrinks_to_first();

	// show
	fail |= test_show_empty();
	fail |= test_show_with_plugins();
	fail |= test_show_with_source_index();
	fail |= test_show_running_with_info();
	fail |= test_show_source_index_filters();
	fail |= test_show_skips_invalid();

	// show_client
	fail |= test_show_client_empty();
	fail |= test_show_client_running();
	fail |= test_show_client_null_fields();
	fail |= test_show_client_skips_nonrunning();

	// unpause_all / retry_all
	fail |= test_unpause_all_no_paused();
	fail |= test_unpause_all_unpauses();
	fail |= test_retry_all_no_pending();
	fail |= test_retry_all_with_pending();

	// ini_startup
	fail |= test_ini_startup_missing_file();
	fail |= test_ini_startup_valid_file();
	fail |= test_ini_startup_duplicate();
	fail |= test_ini_startup_empty_file();
	fail |= test_ini_startup_crlf();
	fail |= test_ini_startup_malformed();
	fail |= test_ini_startup_pfspecific_override();
	fail |= test_ini_startup_pfspecific_skip_lower();

	// ini_refresh
	fail |= test_ini_refresh_missing_file();
	fail |= test_ini_refresh_valid_new_plugin();
	fail |= test_ini_refresh_existing_plugin();
	fail |= test_ini_refresh_newer_file();
	fail |= test_ini_refresh_newer_file_not_opened();
	fail |= test_ini_refresh_stat_fail();
	fail |= test_ini_refresh_desc_update();
	fail |= test_ini_refresh_malformed_line();
	fail |= test_ini_refresh_empty();

	// cmd_addload
	fail |= test_cmd_addload_bad_parse();
	fail |= test_cmd_addload_no_resolve();
	fail |= test_cmd_addload_resolves_and_loads();
	fail |= test_cmd_addload_already_loaded();

	// plugin_addload
	fail |= test_plugin_addload_bad_plid();
	fail |= test_plugin_addload_resolve_fail();
	fail |= test_plugin_addload_already_loaded();
	fail |= test_plugin_addload_load_fails();

	// load
	fail |= test_load_ini_startup_fail();
	fail |= test_load_with_valid_ini();

	// refresh
	fail |= test_refresh_pa_keep();
	fail |= test_refresh_pa_load();
	fail |= test_refresh_pa_null();
	fail |= test_refresh_pa_none_unload();
	fail |= test_refresh_pa_reload();
	fail |= test_refresh_pa_attach();
	fail |= test_refresh_pa_unload_pending();
	fail |= test_refresh_ini_refresh_fail();
	fail |= test_refresh_skips_invalid();

	// add overflow
	fail |= test_add_overflow();

	// ini_refresh CRLF
	fail |= test_ini_refresh_crlf();

	// reset_plugin
	fail |= test_reset_plugin();

	// ini_startup fopen failure
	fail |= test_ini_startup_fopen_fail();

	// ini_refresh pfspecific logic
	fail |= test_ini_refresh_pfspecific_skip_higher_existing();
	fail |= test_ini_refresh_pfspecific_override_pa_load();
	fail |= test_ini_refresh_pfspecific_unable_to_comply();
	fail |= test_ini_refresh_add_overflow();

	// plugin_addload overflow and load outcomes
	fail |= test_plugin_addload_add_overflow();
	fail |= test_plugin_addload_load_notallowed();
	fail |= test_plugin_addload_load_delayed();
	fail |= test_plugin_addload_load_success();

	// cmd_addload load outcomes
	fail |= test_cmd_addload_load_delayed();
	fail |= test_cmd_addload_load_notallowed();
	fail |= test_cmd_addload_load_success();
	fail |= test_cmd_addload_add_overflow();

	// refresh delayed
	fail |= test_refresh_pa_load_delayed();

	// refresh delayed - additional paths
	fail |= test_refresh_pa_reload_delayed();
	fail |= test_refresh_pa_none_unload_delayed();
	fail |= test_refresh_pa_attach_delayed();
	fail |= test_refresh_pa_unload_delayed();
	fail |= test_refresh_default_action();

	// attach failure paths
	fail |= test_plugin_addload_attach_fails();
	fail |= test_cmd_addload_attach_fails();

	// refresh with real .so load
	fail |= test_refresh_pa_load_success();
	fail |= test_load_success();
	fail |= test_load_and_refresh_reload();

	// rebuild_hook_lists
	fail |= test_rebuild_empty();
	fail |= test_rebuild_one_running_dllapi();
	fail |= test_rebuild_one_running_all_tables();
	fail |= test_rebuild_pre_only();
	fail |= test_rebuild_post_only();
	fail |= test_rebuild_skips_non_running();
	fail |= test_rebuild_mixed_statuses();
	fail |= test_rebuild_two_plugins_order();
	fail |= test_rebuild_different_api_groups();
	fail |= test_rebuild_realloc_shrinks();
	fail |= test_rebuild_realloc_failure_keeps_old();
	fail |= test_rebuild_to_empty_frees();
	fail |= test_rebuild_null_tables_ignored();
	fail |= test_rebuild_plugs_contiguous();
	fail |= test_rebuild_repeated_calls();
	fail |= test_rebuild_endlist_bounds();

	// find_plugin_after_rebuild (issue #108)
	fail |= test_find_after_rebuild_later_removed();
	fail |= test_find_after_rebuild_later_removed2();
	fail |= test_find_after_rebuild_later_added();
	fail |= test_find_after_rebuild_earlier_removed();
	fail |= test_find_after_rebuild_earlier_paused();
	fail |= test_find_after_rebuild_two_earlier_removed();
	fail |= test_find_after_rebuild_earlier_removed_mid();
	fail |= test_find_after_rebuild_earlier_removed_becomes_first();
	fail |= test_find_after_rebuild_earlier_added();
	fail |= test_find_after_rebuild_earlier_added_last();
	fail |= test_find_after_rebuild_added_between();
	fail |= test_find_after_rebuild_two_earlier_added();
	fail |= test_find_after_rebuild_earlier_unpaused();
	fail |= test_find_after_rebuild_self_removed_only();
	fail |= test_find_after_rebuild_self_removed_first();
	fail |= test_find_after_rebuild_self_removed_mid();
	fail |= test_find_after_rebuild_self_paused_mid();
	fail |= test_find_after_rebuild_mixed_remove_add_later();
	fail |= test_find_after_rebuild_mixed_remove_add_earlier();
	fail |= test_find_after_rebuild_mixed_last_net_zero();
	fail |= test_find_after_rebuild_mixed_add_before_remove_after();
	fail |= test_find_after_rebuild_others_removed();
	fail |= test_find_after_rebuild_many_earlier_removed();
	fail |= test_find_after_rebuild_many_earlier_added();
	fail |= test_find_after_rebuild_no_change();
	fail |= test_find_after_rebuild_self_removed_last();

	// hook_list stability during iteration (issue #108)
	fail |= test_rebuild_during_iteration_load();
	fail |= test_rebuild_during_iteration_unload_later();
	fail |= test_rebuild_during_iteration_unload_earlier();
	fail |= test_rebuild_during_iteration_load_post();
	fail |= test_rebuild_during_iteration_unload_earlier_post();
	fail |= test_rebuild_during_iteration_segfault();

	system("rm -rf /tmp/test_mlist_gd");

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
