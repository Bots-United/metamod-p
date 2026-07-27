//
// metamod-p - tests for game_support.cpp
//

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>

#include <extdll.h>

#include "metamod.h"
#include "game_support.h"
#include "conf_meta.h"

#include "engine_mock.h"
#include "test_common.h"

// install_gamedll is DLLINTERNAL, declare it for testing
extern mBOOL DLLINTERNAL install_gamedll(char *from, const char *to);

// ============================================================
// Temp directory helpers
// ============================================================

static char _tmp_dir[512];

static const char *make_tmp_dir(void)
{
	snprintf(_tmp_dir, sizeof(_tmp_dir), "/tmp/metamod_test_gs_XXXXXX");
	if (!mkdtemp(_tmp_dir)) return NULL;
	return _tmp_dir;
}

static void make_subdir(const char *base, const char *sub)
{
	char path[512];
	snprintf(path, sizeof(path), "%s/%s", base, sub);
	mkdir(path, 0755);
}

static void make_dummy_file(const char *base, const char *relpath)
{
	char path[512];
	snprintf(path, sizeof(path), "%s/%s", base, relpath);
	int fd = open(path, O_WRONLY | O_CREAT, 0644);
	if (fd >= 0) {
		write(fd, "ELF", 3);
		close(fd);
	}
}

static void cleanup_tmp_dir(void)
{
	if (!_tmp_dir[0]) return;
	char cmd[600];
	snprintf(cmd, sizeof(cmd), "rm -rf %s", _tmp_dir);
	system(cmd);
	_tmp_dir[0] = '\0';
}

// ============================================================
// Mock for LOAD_FILE_FOR_ME / FREE_FILE
// ============================================================

static byte mock_cache_data[] = "FAKEGAMEDLL";
static int mock_cache_len = (int)sizeof(mock_cache_data) - 1;
static int mock_load_called = 0;
static int mock_free_called = 0;

static byte *mock_pfnLoadFileForMe(char *name, int *length)
{
	(void)name;
	mock_load_called++;
	if (length) *length = mock_cache_len;
	return mock_cache_data;
}

static byte *mock_pfnLoadFileForMe_null(char *name, int *length)
{
	(void)name; (void)length;
	mock_load_called++;
	return NULL;
}

static void mock_pfnFreeFile(void *buf)
{
	(void)buf;
	mock_free_called++;
}

// ============================================================
// lookup_game tests
// ============================================================

static int test_lookup_game_known(void)
{
	TEST("lookup_game - finds known game 'cstrike'");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs.so");
	const game_modinfo_t *info = lookup_game("cstrike");
	ASSERT_PTR_NOT_NULL(info);
	ASSERT_STR(info->name, "cstrike");
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_lookup_game_unknown(void)
{
	TEST("lookup_game - unknown game returns NULL");
	mock_reset();
	const game_modinfo_t *info = lookup_game("nonexistent_game_xyz");
	ASSERT_PTR_NULL(info);
	PASS();
	return 0;
}

static int test_lookup_game_null(void)
{
	TEST("lookup_game - NULL returns NULL");
	mock_reset();
	const game_modinfo_t *info = lookup_game(NULL);
	ASSERT_PTR_NULL(info);
	PASS();
	return 0;
}

static int test_lookup_game_no_dll(void)
{
	TEST("lookup_game - known game but dll missing returns NULL");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));
	make_subdir(dir, "dlls");
	const game_modinfo_t *info = lookup_game("cstrike");
	ASSERT_PTR_NULL(info);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

// ============================================================
// install_gamedll tests
// ============================================================

static int test_install_gamedll_null_from(void)
{
	TEST("install_gamedll - NULL from returns mFALSE");
	mock_reset();
	mBOOL ret = install_gamedll(NULL, "/tmp/test.so");
	ASSERT_INT(ret, mFALSE);
	PASS();
	return 0;
}

static int test_install_gamedll_cache_not_found(void)
{
	TEST("install_gamedll - cache miss returns mFALSE");
	mock_reset();
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe_null;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;
	mock_load_called = 0;
	mBOOL ret = install_gamedll((char *)"dlls/fake.so", "/tmp/metamod_test_install.so");
	ASSERT_INT(ret, mFALSE);
	ASSERT_INT(mock_load_called, 1);
	PASS();
	return 0;
}

static int test_install_gamedll_success(void)
{
	TEST("install_gamedll - cache hit installs to file");
	mock_reset();
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	char dest[512];
	snprintf(dest, sizeof(dest), "%s/installed.so", dir);
	mock_load_called = 0;
	mock_free_called = 0;
	mBOOL ret = install_gamedll((char *)"dlls/cs.so", dest);
	ASSERT_INT(ret, mTRUE);
	ASSERT_INT(mock_load_called, 1);
	ASSERT_INT(mock_free_called, 1);
	// Verify file was created
	struct stat st;
	ASSERT_INT(stat(dest, &st), 0);
	ASSERT_INT((int)st.st_size, mock_cache_len);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_install_gamedll_file_exists(void)
{
	TEST("install_gamedll - fails if dest already exists (O_EXCL)");
	mock_reset();
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	char dest[512];
	snprintf(dest, sizeof(dest), "%s/existing.so", dir);
	int fd = open(dest, O_WRONLY | O_CREAT, 0644);
	if (fd >= 0) close(fd);
	mock_load_called = 0;
	mock_free_called = 0;
	mBOOL ret = install_gamedll((char *)"dlls/cs.so", dest);
	ASSERT_INT(ret, mFALSE);
	ASSERT_INT(mock_free_called, 1);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_install_gamedll_null_to(void)
{
	TEST("install_gamedll - NULL to uses from as dest");
	mock_reset();
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	char from_path[512];
	snprintf(from_path, sizeof(from_path), "%s/from_is_to.so", dir);
	mock_load_called = 0;
	mBOOL ret = install_gamedll(from_path, NULL);
	ASSERT_INT(ret, mTRUE);
	struct stat st;
	ASSERT_INT(stat(from_path, &st), 0);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

// ============================================================
// setup_gamedll tests
// ============================================================

static MConfig test_config;

static void init_test_config(const char *gamedir)
{
	memset(&test_config, 0, sizeof(test_config));
	Config = &test_config;
	if (gamedir)
		STRNCPY(GameDLL.gamedir, gamedir, sizeof(GameDLL.gamedir));
}

static int test_setup_gamedll_known_game(void)
{
	TEST("setup_gamedll - known game with existing dll");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_PTR_NOT_NULL(gd.desc);
	ASSERT_STR_CONTAINS(gd.pathname, "cs.so");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_override(void)
{
	TEST("setup_gamedll - override gamedll path");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "unknown_game_xyz", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));

	// Set override path (absolute)
	char override_path[512];
	snprintf(override_path, sizeof(override_path), "%s/custom.so", dir);
	make_dummy_file(dir, "custom.so");
	Config->gamedll = override_path;

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.pathname, "custom.so");
	ASSERT_STR_CONTAINS(gd.desc, "override");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_not_found(void)
{
	TEST("setup_gamedll - no known, no override, no autodetect");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "unknown_game_xyz", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mFALSE);

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_known_with_override(void)
{
	TEST("setup_gamedll - known game with override sets real_pathname");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs.so");

	char override_path[512];
	snprintf(override_path, sizeof(override_path), "%s/bot.so", dir);
	make_dummy_file(dir, "bot.so");
	Config->gamedll = override_path;

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.pathname, "bot.so");
	ASSERT_STR_CONTAINS(gd.real_pathname, "cs.so");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_relative_override(void)
{
	TEST("setup_gamedll - relative override path with cache install");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);
	Config->gamedll = (char *)"dlls/custom.so";

	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "unknown_game_xyz", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.desc, "override");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static void install_fake_gamedll(const char *base, const char *relpath)
{
	char dest[512];
	char cmd[600];
	snprintf(dest, sizeof(dest), "%s/%s", base, relpath);
	snprintf(cmd, sizeof(cmd), "cp fake_gamedll.so %s", dest);
	system(cmd);
}

// ============================================================
// setup_gamedll - stripped DLL name tests
// ============================================================

static int test_setup_gamedll_stripped_name_found(void)
{
	TEST("setup_gamedll - stripped name found (cs_i386.so -> cs.so)");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs_i386.so");
	make_dummy_file(dir, "dlls/cs.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.pathname, "cs.so");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_stripped_name_cache_install(void)
{
	TEST("setup_gamedll - stripped name not found, install from cache");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs_i386.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.pathname, "cs.so");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_stripped_name_cache_miss(void)
{
	TEST("setup_gamedll - stripped name cache miss, falls back to old name");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe_null;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs_i386.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.pathname, "cs_i386.so");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

// ============================================================
// setup_gamedll - old gamedll.txt warning
// ============================================================

static int test_setup_gamedll_old_gamedll_txt(void)
{
	TEST("setup_gamedll - warns about old metagame.ini");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);
	make_dummy_file(dir, "metagame.ini");

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

// ============================================================
// setup_gamedll - autodetect tests
// ============================================================

static int test_setup_gamedll_autodetect(void)
{
	TEST("setup_gamedll - autodetect unknown game");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);
	test_config.autodetect = 1;

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "unknown_game_xyz", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	install_fake_gamedll(dir, "dlls/game.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.pathname, "game.so");
	ASSERT_STR_CONTAINS(gd.desc, "autodetect");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_setup_gamedll_known_autodetect_override(void)
{
	TEST("setup_gamedll - known game dll invalid, autodetect overrides");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);
	test_config.autodetect = 1;

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "cstrike", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/cs.so");
	install_fake_gamedll(dir, "dlls/game.so");

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	ASSERT_STR_CONTAINS(gd.desc, "autodetect-override");

	Config = NULL;
	cleanup_tmp_dir();
	PASS();
	return 0;
}

// ============================================================
// install_gamedll - partial write failure (lines 120-125)
// ============================================================

static byte mock_large_data[4096];
static int mock_large_len = (int)sizeof(mock_large_data);
static int mock_large_free_called = 0;

static byte *mock_pfnLoadFileForMe_large(char *name, int *length)
{
	(void)name;
	mock_load_called++;
	if (length) *length = mock_large_len;
	return mock_large_data;
}

static void mock_pfnFreeFile_large(void *buf)
{
	(void)buf;
	mock_large_free_called++;
}

static int test_install_gamedll_partial_write(void)
{
	TEST("install_gamedll - partial write returns mFALSE and cleans up");
	mock_reset();
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe_large;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile_large;

	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	char dest[512];
	snprintf(dest, sizeof(dest), "%s/partial.so", dir);

	// Use RLIMIT_FSIZE to limit file size to 1 byte, causing partial write
	struct rlimit old_rlim, new_rlim;
	getrlimit(RLIMIT_FSIZE, &old_rlim);
	new_rlim.rlim_cur = 1;
	new_rlim.rlim_max = old_rlim.rlim_max;
	setrlimit(RLIMIT_FSIZE, &new_rlim);

	// Block SIGXFSZ so write returns error instead of killing us
	sigset_t block, old_mask;
	sigemptyset(&block);
	sigaddset(&block, SIGXFSZ);
	sigprocmask(SIG_BLOCK, &block, &old_mask);

	mock_load_called = 0;
	mock_large_free_called = 0;
	memset(mock_large_data, 'A', sizeof(mock_large_data));
	mBOOL ret = install_gamedll((char *)"dlls/cs.so", dest);
	ASSERT_INT(ret, mFALSE);

	// Restore rlimit and signal mask
	setrlimit(RLIMIT_FSIZE, &old_rlim);
	sigprocmask(SIG_SETMASK, &old_mask, NULL);

	// The file should have been cleaned up (unlinked)
	struct stat st;
	ASSERT_TRUE(stat(dest, &st) != 0);

	cleanup_tmp_dir();
	PASS();
	return 0;
}

// ============================================================
// setup_gamedll - override with bare filename (no '/')
// ============================================================

static int test_setup_gamedll_bare_override(void)
{
	TEST("setup_gamedll - override with bare filename (no '/')");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }

	init_test_config(dir);

	// Set override to a bare filename - no directory separators
	// install_gamedll will fail because cache is empty
	g_engfuncs.pfnLoadFileForMe = mock_pfnLoadFileForMe_null;
	g_engfuncs.pfnFreeFile = mock_pfnFreeFile;
	Config->gamedll = (char *)"custom_nodirpath.so";

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.name, "unknown_game_xyz", sizeof(gd.name));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));

	mBOOL ret = setup_gamedll(&gd);
	ASSERT_INT(ret, mTRUE);
	// The file pointer should equal the pathname itself (no '/' found)
	ASSERT_PTR_EQ((void *)gd.file, (void *)gd.pathname);

	Config = NULL;
	cleanup_tmp_dir();
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

	printf("test_game_support:\n");

	fail |= test_lookup_game_known();
	fail |= test_lookup_game_unknown();
	fail |= test_lookup_game_null();
	fail |= test_lookup_game_no_dll();

	fail |= test_install_gamedll_null_from();
	fail |= test_install_gamedll_cache_not_found();
	fail |= test_install_gamedll_success();
	fail |= test_install_gamedll_file_exists();
	fail |= test_install_gamedll_null_to();

	fail |= test_setup_gamedll_known_game();
	fail |= test_setup_gamedll_override();
	fail |= test_setup_gamedll_not_found();
	fail |= test_setup_gamedll_known_with_override();
	fail |= test_setup_gamedll_relative_override();
	fail |= test_setup_gamedll_stripped_name_found();
	fail |= test_setup_gamedll_stripped_name_cache_install();
	fail |= test_setup_gamedll_stripped_name_cache_miss();
	fail |= test_setup_gamedll_old_gamedll_txt();
	fail |= test_setup_gamedll_autodetect();
	fail |= test_setup_gamedll_known_autodetect_override();

	fail |= test_setup_gamedll_bare_override();
	fail |= test_install_gamedll_partial_write();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
