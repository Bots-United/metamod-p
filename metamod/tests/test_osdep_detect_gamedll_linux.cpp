//
// metamod-p - tests for osdep_detect_gamedll_linux.cpp
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <extdll.h>

#include "metamod.h"
#include "osdep_p.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// is_gamedll tests
// ============================================================

static int test_gamedll_real_gamedll(void)
{
	TEST("is_gamedll - fake_gamedll.so detected as game DLL");
	mock_reset();
	mBOOL ret = is_gamedll("./fake_gamedll.so");
	ASSERT_TRUE(ret == mTRUE);
	PASS();
	return 0;
}

static int test_gamedll_metamod_plugin(void)
{
	TEST("is_gamedll - fake_mm_plugin.so detected as NOT game DLL (metamod plugin)");
	mock_reset();
	mBOOL ret = is_gamedll("./fake_mm_plugin.so");
	ASSERT_TRUE(ret == mFALSE);
	PASS();
	return 0;
}

static int test_gamedll_engine_so(void)
{
	TEST("is_gamedll - engine_test.so is NOT game DLL (no required exports)");
	mock_reset();
	mBOOL ret = is_gamedll("./engine_test.so");
	ASSERT_TRUE(ret == mFALSE);
	PASS();
	return 0;
}

static int test_gamedll_nonexistent_file(void)
{
	TEST("is_gamedll - nonexistent file returns mFALSE");
	mock_reset();
	mBOOL ret = is_gamedll("/tmp/metamod_test_no_such_file.so");
	ASSERT_TRUE(ret == mFALSE);
	PASS();
	return 0;
}

static int test_gamedll_too_small(void)
{
	TEST("is_gamedll - file too small for ELF header returns mFALSE");
	mock_reset();
	const char *path = make_tmp_file("ABC");
	ASSERT_PTR_NOT_NULL(path);
	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_tmp_file();
	PASS();
	return 0;
}

static int test_gamedll_not_elf(void)
{
	TEST("is_gamedll - non-ELF file with enough bytes returns mFALSE");
	mock_reset();
	// Write enough bytes to pass the size check but fail ELF magic
	FILE *f = tmpfile();
	ASSERT_PTR_NOT_NULL(f);
	char buf[256];
	memset(buf, 'X', sizeof(buf));
	fwrite(buf, 1, sizeof(buf), f);
	fflush(f);
	// Need a named file for is_gamedll, so use make_tmp_file approach
	cleanup_tmp_file();

	char path[64];
	snprintf(path, sizeof(path), "/tmp/metamod_test_notelf_XXXXXX");
	int fd = mkstemp(path);
	ASSERT_TRUE(fd >= 0);
	write(fd, buf, sizeof(buf));
	close(fd);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	unlink(path);
	PASS();
	return 0;
}

static int test_gamedll_truncated_elf(void)
{
	TEST("is_gamedll - truncated ELF (valid header, bad sections) returns mFALSE");
	mock_reset();
	// Read the real gamedll header, then truncate most of the file
	FILE *src = fopen("./fake_gamedll.so", "rb");
	if (!src) {
		printf("SKIP (could not open fake_gamedll.so)\n");
		tests_run--;
		return 0;
	}
	// Read ELF header, pad to 512 bytes (enough for valgrind's debuginfo
	// reader but still truncated — section headers point past EOF)
	char buf[512];
	memset(buf, 0, sizeof(buf));
	size_t n = fread(buf, 1, sizeof(buf), src);
	fclose(src);
	(void)n;

	char path[64];
	snprintf(path, sizeof(path), "/tmp/metamod_test_trunc_XXXXXX");
	int fd = mkstemp(path);
	ASSERT_TRUE(fd >= 0);
	write(fd, buf, sizeof(buf));
	close(fd);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	unlink(path);
	PASS();
	return 0;
}

static int test_gamedll_test_binary(void)
{
	TEST("is_gamedll - executable (not shared lib) returns mFALSE");
	mock_reset();
	// Our own test binary is ET_EXEC, not ET_DYN
	mBOOL ret = is_gamedll("./test_osdep_detect_gamedll_linux");
	ASSERT_TRUE(ret == mFALSE);
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

	printf("test_osdep_detect_gamedll_linux:\n");
	fail |= test_gamedll_real_gamedll();
	fail |= test_gamedll_metamod_plugin();
	fail |= test_gamedll_engine_so();
	fail |= test_gamedll_nonexistent_file();
	fail |= test_gamedll_too_small();
	fail |= test_gamedll_not_elf();
	fail |= test_gamedll_truncated_elf();
	fail |= test_gamedll_test_binary();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
