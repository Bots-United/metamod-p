//
// metamod-p - tests for osdep_linkent_win32.cpp
//
// Compiles the Win32 PE export table combiner on Linux using
// win32_stubs.h/cpp. Tests construct fake PE modules in memory
// and verify the export table combination logic.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#include <extdll.h>

#include "metamod.h"
#include "linkent.h"

#include "engine_mock.h"
#include "test_common.h"

// Win32 stub control (defined in win32_stubs.cpp)
extern void win32_stub_reset(void);
extern void win32_stub_set_virtualprotect_fail(bool);
extern void win32_stub_free_virtualallocs(void);
extern void win32_stub_set_tracking_callocs(bool);
extern void win32_stub_free_tracked_callocs(void);

// ============================================================
// Helper: build a fake in-memory PE module with export table
//
// The "module" is an mmap'd region that looks like a loaded PE:
//  - DOS header at base
//  - NT headers at e_lfanew
//  - Section header
//  - Export directory with functions, names, ordinals
//
// All RVAs are relative to the module base (as with a loaded DLL).
// ============================================================

#define MODULE_SIZE 8192

struct fake_module {
	char *base;
	HMODULE handle;
};

static struct fake_module make_fake_module(int num_exports, const char **names)
{
	struct fake_module mod;
	mod.base = (char *)mmap(NULL, MODULE_SIZE, PROT_READ | PROT_WRITE,
	                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	mod.handle = (HMODULE)mod.base;

	memset(mod.base, 0, MODULE_SIZE);

	// DOS header
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)mod.base;
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = 64;

	// NT headers
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(mod.base + 64);
	nt->Signature = IMAGE_NT_SIGNATURE;
	nt->FileHeader.NumberOfSections = 0;
	nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);

	// Export directory at offset 0x400 (RVA 0x400)
	unsigned long export_rva = 0x400;
	nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = export_rva;
	nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = sizeof(IMAGE_EXPORT_DIRECTORY);

	IMAGE_EXPORT_DIRECTORY *exp = (IMAGE_EXPORT_DIRECTORY *)(mod.base + export_rva);
	exp->Base = 1;
	exp->NumberOfFunctions = num_exports;
	exp->NumberOfNames = num_exports;

	// Function RVAs at offset 0x500
	unsigned long func_offset = 0x500;
	exp->AddressOfFunctions = func_offset;
	unsigned long *func_rvas = (unsigned long *)(mod.base + func_offset);

	// Name RVAs at offset 0x600
	unsigned long names_offset = 0x600;
	exp->AddressOfNames = names_offset;
	unsigned long *name_rvas = (unsigned long *)(mod.base + names_offset);

	// Ordinals at offset 0x700
	unsigned long ord_offset = 0x700;
	exp->AddressOfNameOrdinals = ord_offset;
	unsigned short *ordinals = (unsigned short *)(mod.base + ord_offset);

	// Fake function bodies at offset 0x1000+
	// Name strings at offset 0x800+
	unsigned long str_pos = 0x800;

	for (int i = 0; i < num_exports; i++) {
		func_rvas[i] = 0x1000 + i * 16;
		ordinals[i] = (unsigned short)i;
		name_rvas[i] = str_pos;
		size_t len = strlen(names[i]) + 1;
		memcpy(mod.base + str_pos, names[i], len);
		str_pos += len;
	}

	return mod;
}

static void free_fake_module(struct fake_module *mod)
{
	if (mod->base) {
		munmap(mod->base, MODULE_SIZE);
		mod->base = NULL;
		mod->handle = NULL;
	}
}

// ============================================================
// Tests
// ============================================================

static int test_combine_basic(void)
{
	TEST("init_linkent_replacement(win32) - combines two export tables");
	mock_reset(); win32_stub_reset();

	const char *mm_names[] = { "GiveFnptrsToDll" };
	const char *gd_names[] = { "GetEntityAPI2" };

	struct fake_module mm = make_fake_module(1, mm_names);
	struct fake_module gd = make_fake_module(1, gd_names);

	win32_stub_set_tracking_callocs(true);
	int ret = init_linkent_replacement(mm.handle, gd.handle);
	win32_stub_set_tracking_callocs(false);
	ASSERT_INT(ret, 1);

	// Verify the combined export table
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(mm.base + 64);
	unsigned long exp_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	IMAGE_EXPORT_DIRECTORY *exp = (IMAGE_EXPORT_DIRECTORY *)(mm.base + exp_rva);

	ASSERT_INT((int)exp->NumberOfFunctions, 2);
	ASSERT_INT((int)exp->NumberOfNames, 2);

	win32_stub_free_tracked_callocs();
	win32_stub_free_virtualallocs();
	free_fake_module(&gd);
	free_fake_module(&mm);
	PASS();
	return 0;
}

static int test_combine_overlapping_names(void)
{
	TEST("init_linkent_replacement(win32) - duplicate names deduplicated");
	mock_reset(); win32_stub_reset();

	const char *mm_names[] = { "GiveFnptrsToDll", "SharedFunc" };
	const char *gd_names[] = { "GetEntityAPI2", "SharedFunc" };

	struct fake_module mm = make_fake_module(2, mm_names);
	struct fake_module gd = make_fake_module(2, gd_names);

	win32_stub_set_tracking_callocs(true);
	int ret = init_linkent_replacement(mm.handle, gd.handle);
	win32_stub_set_tracking_callocs(false);
	ASSERT_INT(ret, 1);

	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(mm.base + 64);
	unsigned long exp_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	IMAGE_EXPORT_DIRECTORY *exp = (IMAGE_EXPORT_DIRECTORY *)(mm.base + exp_rva);

	// 4 total functions (2+2), but only 3 names (SharedFunc deduplicated)
	ASSERT_INT((int)exp->NumberOfFunctions, 4);
	ASSERT_INT((int)exp->NumberOfNames, 3);

	win32_stub_free_tracked_callocs();
	win32_stub_free_virtualallocs();
	free_fake_module(&gd);
	free_fake_module(&mm);
	PASS();
	return 0;
}

static int test_combine_sorted_names(void)
{
	TEST("init_linkent_replacement(win32) - names are sorted alphabetically");
	mock_reset(); win32_stub_reset();

	const char *mm_names[] = { "Zebra" };
	const char *gd_names[] = { "Alpha", "Middle" };

	struct fake_module mm = make_fake_module(1, mm_names);
	struct fake_module gd = make_fake_module(2, gd_names);

	win32_stub_set_tracking_callocs(true);
	int ret = init_linkent_replacement(mm.handle, gd.handle);
	win32_stub_set_tracking_callocs(false);
	ASSERT_INT(ret, 1);

	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(mm.base + 64);
	unsigned long exp_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	IMAGE_EXPORT_DIRECTORY *exp = (IMAGE_EXPORT_DIRECTORY *)(mm.base + exp_rva);

	// Check names are sorted: Alpha < Middle < Zebra
	unsigned long *name_rvas = (unsigned long *)(mm.base +
		(unsigned long)exp->AddressOfNames);
	const char *n0 = (const char *)(mm.base + name_rvas[0]);
	const char *n1 = (const char *)(mm.base + name_rvas[1]);
	const char *n2 = (const char *)(mm.base + name_rvas[2]);

	ASSERT_TRUE(strcmp(n0, "Alpha") == 0);
	ASSERT_TRUE(strcmp(n1, "Middle") == 0);
	ASSERT_TRUE(strcmp(n2, "Zebra") == 0);

	win32_stub_free_tracked_callocs();
	win32_stub_free_virtualallocs();
	free_fake_module(&gd);
	free_fake_module(&mm);
	PASS();
	return 0;
}

static int test_null_mm_exports(void)
{
	TEST("init_linkent_replacement(win32) - NULL MM module returns 0");
	mock_reset(); win32_stub_reset();

	const char *gd_names[] = { "GetEntityAPI2" };
	struct fake_module gd = make_fake_module(1, gd_names);

	int ret = init_linkent_replacement(NULL, gd.handle);
	ASSERT_INT(ret, 0);

	free_fake_module(&gd);
	PASS();
	return 0;
}

static int test_null_game_exports(void)
{
	TEST("init_linkent_replacement(win32) - NULL game module returns 0");
	mock_reset(); win32_stub_reset();

	const char *mm_names[] = { "GiveFnptrsToDll" };
	struct fake_module mm = make_fake_module(1, mm_names);

	int ret = init_linkent_replacement(mm.handle, NULL);
	ASSERT_INT(ret, 0);

	free_fake_module(&mm);
	PASS();
	return 0;
}

static int test_virtualprotect_fail(void)
{
	TEST("init_linkent_replacement(win32) - VirtualProtect failure returns 0");
	mock_reset(); win32_stub_reset();
	win32_stub_set_virtualprotect_fail(true);

	const char *mm_names[] = { "GiveFnptrsToDll" };
	const char *gd_names[] = { "GetEntityAPI2" };

	struct fake_module mm = make_fake_module(1, mm_names);
	struct fake_module gd = make_fake_module(1, gd_names);

	win32_stub_set_tracking_callocs(true);
	int ret = init_linkent_replacement(mm.handle, gd.handle);
	win32_stub_set_tracking_callocs(false);
	ASSERT_INT(ret, 0);

	win32_stub_free_tracked_callocs();
	win32_stub_free_virtualallocs();
	free_fake_module(&gd);
	free_fake_module(&mm);
	PASS();
	return 0;
}

static int test_mm_no_exports(void)
{
	TEST("init_linkent_replacement(win32) - MM with no export dir returns 0");
	mock_reset(); win32_stub_reset();

	// Create MM module with no export directory
	struct fake_module mm;
	mm.base = (char *)mmap(NULL, MODULE_SIZE, PROT_READ | PROT_WRITE,
	                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	mm.handle = (HMODULE)mm.base;
	memset(mm.base, 0, MODULE_SIZE);

	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)mm.base;
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = 64;
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(mm.base + 64);
	nt->Signature = IMAGE_NT_SIGNATURE;
	nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
	// No export VirtualAddress → get_export_table returns 0

	const char *gd_names[] = { "GetEntityAPI2" };
	struct fake_module gd = make_fake_module(1, gd_names);

	int ret = init_linkent_replacement(mm.handle, gd.handle);
	ASSERT_INT(ret, 0);

	free_fake_module(&gd);
	free_fake_module(&mm);
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

	printf("test_osdep_linkent_win32:\n");
	fail |= test_combine_basic();
	fail |= test_combine_overlapping_names();
	fail |= test_combine_sorted_names();
	fail |= test_null_mm_exports();
	fail |= test_null_game_exports();
	fail |= test_virtualprotect_fail();
	fail |= test_mm_no_exports();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
