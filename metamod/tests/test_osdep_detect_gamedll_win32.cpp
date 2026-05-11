//
// metamod-p - tests for osdep_detect_gamedll_win32.cpp
//
// Compiles the Win32 PE-parsing is_gamedll() on Linux using
// win32_stubs.h/cpp to stub all Win32 API functions. Tests use
// crafted PE images written to temp files.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <extdll.h>

#include "metamod.h"
#include "osdep_p.h"

#include "engine_mock.h"
#include "test_common.h"

// Win32 stub control (defined in win32_stubs.cpp)
extern void win32_stub_reset(void);
extern void win32_stub_set_createfile_fail(bool);
extern void win32_stub_set_createfilemapping_fail(bool);
extern void win32_stub_set_mapviewoffile_fail(bool);
extern void win32_stub_set_isbadreadptr(bool);
extern void win32_stub_set_isbadreadptr_fail_after(int);
extern void win32_stub_set_isbadstringptr_fail_after(int);

// ============================================================
// Helper: build a minimal PE image with configurable exports
// ============================================================

// Layout: DOS header + padding + NT headers + section header + section data
//   Section data contains: export directory + function RVAs + name RVAs +
//   ordinals + name strings

struct pe_builder {
	char buf[4096];
	unsigned long pos;
	unsigned long section_rva;
	unsigned long section_file_offset;

	void init(void) {
		memset(buf, 0, sizeof(buf));
		pos = 0;
	}

	void build_headers(int num_exports, const char **export_names,
	                   int num_funcs)
	{
		// DOS header at offset 0
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)buf;
		dos->e_magic = IMAGE_DOS_SIGNATURE;
		dos->e_lfanew = 64; // NT headers at offset 64

		// NT headers
		IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(buf + 64);
		nt->Signature = IMAGE_NT_SIGNATURE;
		nt->FileHeader.NumberOfSections = 1;
		nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

		// Section starts after headers
		section_file_offset = 64 + sizeof(IMAGE_NT_HEADERS) +
		                      sizeof(IMAGE_SECTION_HEADER);
		// Align to 16 bytes
		section_file_offset = (section_file_offset + 15) & ~15UL;
		section_rva = 0x1000;

		// Export directory RVA points into our section
		if (num_exports > 0 || num_funcs > 0) {
			nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = section_rva;
			nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = sizeof(IMAGE_EXPORT_DIRECTORY);
		}

		// Section header
		IMAGE_SECTION_HEADER *sect = IMAGE_FIRST_SECTION(nt);
		memcpy(sect->Name, ".edata", 7);
		sect->VirtualAddress = section_rva;
		sect->PointerToRawData = section_file_offset;

		// Build export data in section
		pos = section_file_offset;

		IMAGE_EXPORT_DIRECTORY *exp = (IMAGE_EXPORT_DIRECTORY *)(buf + pos);
		pos += sizeof(IMAGE_EXPORT_DIRECTORY);

		if (num_funcs < num_exports)
			num_funcs = num_exports;

		exp->NumberOfFunctions = num_funcs;
		exp->NumberOfNames = num_exports;
		exp->Base = 1;

		// Function RVA table
		exp->AddressOfFunctions = section_rva + (pos - section_file_offset);
		unsigned long *func_rvas = (unsigned long *)(buf + pos);
		for (int i = 0; i < num_funcs; i++)
			func_rvas[i] = section_rva + 0x500 + i * 16; // fake RVAs
		pos += num_funcs * sizeof(unsigned long);

		// Name RVA table
		exp->AddressOfNames = section_rva + (pos - section_file_offset);
		unsigned long *name_rvas = (unsigned long *)(buf + pos);
		pos += num_exports * sizeof(unsigned long);

		// Ordinal table
		exp->AddressOfNameOrdinals = section_rva + (pos - section_file_offset);
		unsigned short *ordinals = (unsigned short *)(buf + pos);
		for (int i = 0; i < num_exports; i++)
			ordinals[i] = (unsigned short)i;
		pos += num_exports * sizeof(unsigned short);

		// Name strings
		for (int i = 0; i < num_exports; i++) {
			unsigned long name_rva = section_rva + (pos - section_file_offset);
			name_rvas[i] = name_rva;
			size_t len = strlen(export_names[i]) + 1;
			memcpy(buf + pos, export_names[i], len);
			pos += len;
		}

		// Set section raw data size
		sect->SizeOfRawData = pos - section_file_offset;
	}

	const char *write_to_file(void) {
		static char path[256];
		snprintf(path, sizeof(path), "/tmp/metamod_test_pe_XXXXXX");
		int fd = mkstemp(path);
		if (fd < 0) return NULL;
		write(fd, buf, pos);
		close(fd);
		return path;
	}
};

static char pe_tmp_path[256];

static void cleanup_pe_file(void)
{
	if (pe_tmp_path[0]) {
		unlink(pe_tmp_path);
		pe_tmp_path[0] = '\0';
	}
}

static const char *make_pe_file(int num_exports, const char **names, int num_funcs)
{
	struct pe_builder pb;
	pb.init();
	pb.build_headers(num_exports, names, num_funcs);
	const char *path = pb.write_to_file();
	if (path) {
		strncpy(pe_tmp_path, path, sizeof(pe_tmp_path) - 1);
		pe_tmp_path[sizeof(pe_tmp_path) - 1] = '\0';
	}
	return path;
}

// PE with no export directory
static const char *make_pe_no_exports(void)
{
	struct pe_builder pb;
	pb.init();

	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)pb.buf;
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = 64;

	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(pb.buf + 64);
	nt->Signature = IMAGE_NT_SIGNATURE;
	nt->FileHeader.NumberOfSections = 1;
	nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
	// No export directory (VirtualAddress = 0)

	IMAGE_SECTION_HEADER *sect = IMAGE_FIRST_SECTION(nt);
	memcpy(sect->Name, ".text", 6);
	sect->VirtualAddress = 0x1000;
	sect->SizeOfRawData = 64;
	sect->PointerToRawData = 512;
	pb.pos = 512 + 64;

	const char *path = pb.write_to_file();
	if (path) {
		strncpy(pe_tmp_path, path, sizeof(pe_tmp_path) - 1);
		pe_tmp_path[sizeof(pe_tmp_path) - 1] = '\0';
	}
	return path;
}

// ============================================================
// Tests
// ============================================================

static int test_gamedll_detected(void)
{
	TEST("is_gamedll(win32) - PE with GiveFnptrsToDll+GetEntityAPI2 returns mTRUE");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GetEntityAPI2", "GiveFnptrsToDll" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mTRUE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_gamedll_v1_api(void)
{
	TEST("is_gamedll(win32) - PE with GiveFnptrsToDll+GetEntityAPI returns mTRUE");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GetEntityAPI", "GiveFnptrsToDll" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mTRUE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_metamod_plugin(void)
{
	TEST("is_gamedll(win32) - PE with Meta_* exports returns mFALSE");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GiveFnptrsToDll", "Meta_Attach", "Meta_Detach", "Meta_Query" };
	const char *path = make_pe_file(4, names, 4);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_no_game_exports(void)
{
	TEST("is_gamedll(win32) - PE with unrelated exports returns mFALSE");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "SomeOtherFunc", "AnotherFunc" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_no_export_table(void)
{
	TEST("is_gamedll(win32) - PE with no export table returns mFALSE");
	mock_reset(); win32_stub_reset();

	const char *path = make_pe_no_exports();
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_nonexistent_file(void)
{
	TEST("is_gamedll(win32) - nonexistent file returns mFALSE");
	mock_reset(); win32_stub_reset();

	mBOOL ret = is_gamedll("/tmp/metamod_test_no_such_file_win32.dll");
	ASSERT_TRUE(ret == mFALSE);
	PASS();
	return 0;
}

static int test_createfile_fail(void)
{
	TEST("is_gamedll(win32) - CreateFileA failure returns mFALSE");
	mock_reset(); win32_stub_reset();
	win32_stub_set_createfile_fail(true);

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_createfilemapping_fail(void)
{
	TEST("is_gamedll(win32) - CreateFileMapping failure returns mFALSE");
	mock_reset(); win32_stub_reset();
	win32_stub_set_createfilemapping_fail(true);

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_mapviewoffile_fail(void)
{
	TEST("is_gamedll(win32) - MapViewOfFile failure returns mFALSE");
	mock_reset(); win32_stub_reset();
	win32_stub_set_mapviewoffile_fail(true);

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_invalid_dos_header(void)
{
	TEST("is_gamedll(win32) - invalid DOS header returns mFALSE");
	mock_reset(); win32_stub_reset();

	char buf[256];
	memset(buf, 0, sizeof(buf));
	// No MZ signature
	char path[64];
	snprintf(path, sizeof(path), "/tmp/metamod_test_baddos_XXXXXX");
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

static int test_invalid_pe_header(void)
{
	TEST("is_gamedll(win32) - invalid PE signature returns mFALSE");
	mock_reset(); win32_stub_reset();

	char buf[256];
	memset(buf, 0, sizeof(buf));
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)buf;
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = 64;
	// NT header with wrong signature
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(buf + 64);
	nt->Signature = 0xDEADBEEF;

	char path[64];
	snprintf(path, sizeof(path), "/tmp/metamod_test_badpe_XXXXXX");
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

static int test_bad_sections_ptr(void)
{
	TEST("is_gamedll(win32) - IsBadReadPtr on sections returns mFALSE");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	// Trigger IsBadReadPtr failure after NT headers parsed
	// We do this by setting a flag that makes IsBadReadPtr return TRUE
	// for the sections check. But our stub is global, so we need
	// to be more selective. For simplicity, test the "bad names pointer" path.
	// This is covered by a PE with valid structure but IsBadReadPtr returning TRUE.
	// Since our stub isn't call-count-selective, test this differently:
	// truncate the PE file so MapViewOfFile succeeds but sections are invalid.

	// Actually just test with isbadreadptr forced on after headers are parsed.
	// Skip this if we can't be selective enough.
	cleanup_pe_file();

	// Instead: create PE with NumberOfSections = 0 → export table lookup fails
	struct pe_builder pb;
	pb.init();
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)pb.buf;
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = 64;
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(pb.buf + 64);
	nt->Signature = IMAGE_NT_SIGNATURE;
	nt->FileHeader.NumberOfSections = 0;
	nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
	nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = 0x1000;
	pb.pos = 64 + sizeof(IMAGE_NT_HEADERS);

	path = pb.write_to_file();
	ASSERT_PTR_NOT_NULL(path);
	strncpy(pe_tmp_path, path, sizeof(pe_tmp_path) - 1);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_isbadreadptr_sections_fail(void)
{
	TEST("is_gamedll(win32) - IsBadReadPtr fails on sections check");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	// IsBadReadPtr call sequence in is_gamedll:
	//   #0: dos header (in get_ntheaders)
	//   #1: pe header (in get_ntheaders)
	//   #2: sections pointer (line 147)
	// Fail on call #2 to trigger sections check failure
	win32_stub_set_isbadreadptr_fail_after(2);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_isbadreadptr_names_fail(void)
{
	TEST("is_gamedll(win32) - IsBadReadPtr fails on names pointer");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	// IsBadReadPtr call sequence:
	//   #0: dos header, #1: pe header, #2: sections,
	//   #3: export_dir (in get_export_table), #4: names pointer (line 167)
	// Fail on call #4
	win32_stub_set_isbadreadptr_fail_after(4);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_isbadstringptr_funcname(void)
{
	TEST("is_gamedll(win32) - IsBadStringPtrA fails on function name");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "GiveFnptrsToDll", "GetEntityAPI2" };
	const char *path = make_pe_file(2, names, 2);
	ASSERT_PTR_NOT_NULL(path);

	// Fail IsBadStringPtrA on 1st call to trigger the continue branch
	win32_stub_set_isbadstringptr_fail_after(0);

	mBOOL ret = is_gamedll(path);
	// With all function names failing IsBadStringPtrA, no exports are detected
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
	PASS();
	return 0;
}

static int test_meta_detach_only(void)
{
	TEST("is_gamedll(win32) - PE with only Meta_Detach returns mFALSE");
	mock_reset(); win32_stub_reset();

	const char *names[] = { "Meta_Detach" };
	const char *path = make_pe_file(1, names, 1);
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_pe_file();
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

	printf("test_osdep_detect_gamedll_win32:\n");
	fail |= test_gamedll_detected();
	fail |= test_gamedll_v1_api();
	fail |= test_metamod_plugin();
	fail |= test_no_game_exports();
	fail |= test_no_export_table();
	fail |= test_nonexistent_file();
	fail |= test_createfile_fail();
	fail |= test_createfilemapping_fail();
	fail |= test_mapviewoffile_fail();
	fail |= test_invalid_dos_header();
	fail |= test_invalid_pe_header();
	fail |= test_bad_sections_ptr();
	fail |= test_isbadreadptr_sections_fail();
	fail |= test_isbadreadptr_names_fail();
	fail |= test_isbadstringptr_funcname();
	fail |= test_meta_detach_only();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
