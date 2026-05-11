//
// metamod-p - tests for game_autodetect.cpp
//

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <extdll.h>

#include "metamod.h"
#include "game_autodetect.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Temp directory helpers
// ============================================================

static char _tmp_dir[512];

static const char *make_tmp_dir(void)
{
	snprintf(_tmp_dir, sizeof(_tmp_dir), "/tmp/metamod_test_gad_XXXXXX");
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

static void install_fake_gamedll(const char *base, const char *relpath)
{
	char dest[512];
	char cmd[600];
	snprintf(dest, sizeof(dest), "%s/%s", base, relpath);
	snprintf(cmd, sizeof(cmd), "cp fake_gamedll.so %s", dest);
	system(cmd);
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
// autodetect_gamedll tests
// ============================================================

static int test_autodetect_dir_not_found(void)
{
	TEST("autodetect_gamedll - dlls dir doesn't exist returns NULL");
	mock_reset();
	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, "/tmp/nonexistent_metamod_test_xyz", sizeof(gd.gamedir));
	STRNCPY(GameDLL.gamedir, gd.gamedir, sizeof(GameDLL.gamedir));

	const char *result = autodetect_gamedll(&gd, NULL);
	ASSERT_PTR_NULL(result);
	PASS();
	return 0;
}

static int test_autodetect_knownfn_valid(void)
{
	TEST("autodetect_gamedll - knownfn is valid gamedll returns NULL");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	install_fake_gamedll(dir, "dlls/game.so");

	const char *result = autodetect_gamedll(&gd, "game.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_finds_gamedll(void)
{
	TEST("autodetect_gamedll - finds gamedll in dlls dir");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	install_fake_gamedll(dir, "dlls/game.so");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NOT_NULL(result);
	ASSERT_STR(result, "game.so");
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_empty_dir(void)
{
	TEST("autodetect_gamedll - empty dlls dir returns NULL");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_skips_metamod(void)
{
	TEST("autodetect_gamedll - skips metamod*.so");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	install_fake_gamedll(dir, "dlls/metamod_p.so");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_skips_bot(void)
{
	TEST("autodetect_gamedll - skips bot.so and bot_i386.so");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	install_fake_gamedll(dir, "dlls/bot.so");
	install_fake_gamedll(dir, "dlls/bot_i386.so");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_skips_non_so(void)
{
	TEST("autodetect_gamedll - skips non-.so files");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/readme.txt");
	make_dummy_file(dir, "dlls/config.cfg");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_skips_short_name(void)
{
	TEST("autodetect_gamedll - skips filenames shorter than .so extension");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/.so");
	make_dummy_file(dir, "dlls/ab");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_skips_invalid_gamedll(void)
{
	TEST("autodetect_gamedll - skips .so that isn't valid gamedll");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	make_dummy_file(dir, "dlls/invalid.so");

	const char *result = autodetect_gamedll(&gd, "nonexistent.so");
	ASSERT_PTR_NULL(result);
	cleanup_tmp_dir();
	PASS();
	return 0;
}

static int test_autodetect_knownfn_null(void)
{
	TEST("autodetect_gamedll - NULL knownfn, finds gamedll");
	mock_reset();
	const char *dir = make_tmp_dir();
	if (!dir) { printf("SKIP (no tmpdir)\n"); tests_run--; return 0; }
	STRNCPY(GameDLL.gamedir, dir, sizeof(GameDLL.gamedir));

	gamedll_t gd;
	memset(&gd, 0, sizeof(gd));
	STRNCPY(gd.gamedir, dir, sizeof(gd.gamedir));
	make_subdir(dir, "dlls");
	install_fake_gamedll(dir, "dlls/game.so");

	const char *result = autodetect_gamedll(&gd, NULL);
	ASSERT_PTR_NOT_NULL(result);
	ASSERT_STR(result, "game.so");
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

	printf("test_game_autodetect:\n");

	fail |= test_autodetect_dir_not_found();
	fail |= test_autodetect_knownfn_valid();
	fail |= test_autodetect_finds_gamedll();
	fail |= test_autodetect_empty_dir();
	fail |= test_autodetect_skips_metamod();
	fail |= test_autodetect_skips_bot();
	fail |= test_autodetect_skips_non_so();
	fail |= test_autodetect_skips_short_name();
	fail |= test_autodetect_skips_invalid_gamedll();
	fail |= test_autodetect_knownfn_null();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
