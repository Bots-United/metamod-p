//
// metamod-p - tests for osdep_linkent_linux.cpp
//
// Uses #include approach to test static functions directly,
// like jk_botti's test_bot_query_hook_linux.cpp.
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <valgrind/valgrind.h>

#include <extdll.h>

#include "metamod.h"

#include "engine_mock.h"
#include "test_common.h"

// Include the source under test — gives access to all statics
#include "../osdep_linkent_linux.cpp"

static void restore_linkent(void)
{
	if(dlsym_original)
		restore_original_dlsym();
	gamedll_module_handle = 0;
	metamod_module_handle = 0;
	is_original_restored = 0;
}

// ============================================================
// construct_jmp_instruction tests
// ============================================================

static int test_construct_jmp_basic(void)
{
	TEST("construct_jmp_instruction - jmp with correct offset");
	unsigned char buf[BYTES_SIZE];
	memset(buf, 0, sizeof(buf));

	char fake_place[16];
	unsigned long place_addr = (unsigned long)fake_place;
	unsigned long target_addr = place_addr + 0x1000;

	construct_jmp_instruction(buf, (void *)place_addr, (void *)target_addr);

	// jmp opcode
	ASSERT_TRUE(buf[0] == 0xe9);

	unsigned long encoded_offset;
	memcpy(&encoded_offset, buf + 1, sizeof(encoded_offset));
	unsigned long expected_offset = target_addr - (place_addr + BYTES_SIZE);
	ASSERT_TRUE(encoded_offset == expected_offset);

	PASS();
	return 0;
}

static int test_construct_jmp_backward(void)
{
	TEST("construct_jmp_instruction - backward jump has negative offset");
	unsigned char buf[BYTES_SIZE];
	memset(buf, 0, sizeof(buf));

	char fake_place[16];
	unsigned long place_addr = (unsigned long)fake_place;
	unsigned long target_addr = place_addr - 0x100;

	construct_jmp_instruction(buf, (void *)place_addr, (void *)target_addr);

	ASSERT_TRUE(buf[0] == 0xe9);

	unsigned long encoded_offset;
	memcpy(&encoded_offset, buf + 1, sizeof(encoded_offset));
	unsigned long expected_offset = target_addr - (place_addr + BYTES_SIZE);
	ASSERT_TRUE(encoded_offset == expected_offset);

	PASS();
	return 0;
}

static int test_construct_jmp_self(void)
{
	TEST("construct_jmp_instruction - jump to self (offset = -BYTES_SIZE)");
	unsigned char buf[BYTES_SIZE];
	memset(buf, 0, sizeof(buf));

	char fake_place[16];
	construct_jmp_instruction(buf, fake_place, fake_place);

	ASSERT_TRUE(buf[0] == 0xe9);

	unsigned long encoded_offset;
	memcpy(&encoded_offset, buf + 1, sizeof(encoded_offset));
	ASSERT_TRUE(encoded_offset == (unsigned long)-(long)BYTES_SIZE);

	PASS();
	return 0;
}

static int test_has_endbr32(void)
{
	TEST("has_endbr32 - detects landing pad");
	unsigned char with[]    = { 0xf3, 0x0f, 0x1e, 0xfb, 0x55 };
	unsigned char without[] = { 0x55, 0x89, 0xe5, 0x00, 0x00 };
	ASSERT_TRUE(has_endbr32(with) == true);
	ASSERT_TRUE(has_endbr32(without) == false);
	PASS();
	return 0;
}

// ============================================================
// is_code_trampoline_jmp_opcode tests
// ============================================================

static int test_trampoline_ff25(void)
{
	TEST("is_code_trampoline_jmp_opcode - detects FF 25 pattern");
	unsigned char code[] = { 0xff, 0x25, 0x00, 0x00, 0x00, 0x00 };
	ASSERT_TRUE(is_code_trampoline_jmp_opcode(code) == true);
	PASS();
	return 0;
}

static int test_trampoline_not_ff25(void)
{
	TEST("is_code_trampoline_jmp_opcode - rejects non-trampoline code");
	unsigned char code[] = { 0xe9, 0x00, 0x00, 0x00, 0x00 };
	// || bug: (0xe9 == 0xff) || (0x00 == 0x25) = false
	ASSERT_TRUE(is_code_trampoline_jmp_opcode(code) == false);
	PASS();
	return 0;
}

static int test_trampoline_only_ff(void)
{
	TEST("is_code_trampoline_jmp_opcode - FF without 25 returns false");
	unsigned char code[] = { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00 };
	ASSERT_TRUE(is_code_trampoline_jmp_opcode(code) == false);
	PASS();
	return 0;
}

static int test_trampoline_only_25(void)
{
	TEST("is_code_trampoline_jmp_opcode - 25 at byte[1] without FF returns false");
	unsigned char code[] = { 0x00, 0x25, 0x00, 0x00, 0x00, 0x00 };
	ASSERT_TRUE(is_code_trampoline_jmp_opcode(code) == false);
	PASS();
	return 0;
}

static int test_trampoline_zeros(void)
{
	TEST("is_code_trampoline_jmp_opcode - all zeros returns false");
	unsigned char code[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	ASSERT_TRUE(is_code_trampoline_jmp_opcode(code) == false);
	PASS();
	return 0;
}

// ============================================================
// extract_function_pointer_from_trampoline_jmp tests
// ============================================================

static int test_extract_pointer(void)
{
	TEST("extract_function_pointer_from_trampoline_jmp - reads indirect pointer");
	int dummy_target = 42;
	void *target_ptr = (void *)&dummy_target;

	unsigned char code[6];
	code[0] = 0xff;
	code[1] = 0x25;
	void **indirect = &target_ptr;
	memcpy(code + 2, &indirect, sizeof(indirect));

	void *result = extract_function_pointer_from_trampoline_jmp(code);
	ASSERT_TRUE(result == (void *)&dummy_target);
	PASS();
	return 0;
}

// ============================================================
// init_linkent_replacement integration tests (direct, no fork)
// ============================================================

static int test_linkent_init_and_hook(void)
{
	TEST("init_linkent_replacement - hooks dlsym to forward gamedll symbols");
	mock_reset();

	void *mm_handle = dlopen("./engine_test.so", RTLD_NOW);
	if (!mm_handle) {
		printf("SKIP (could not load engine_test.so)\n");
		tests_run--;
		return 0;
	}
	void *gd_handle = dlopen("./fake_gamedll.so", RTLD_NOW);
	if (!gd_handle) {
		dlclose(mm_handle);
		printf("SKIP (could not load fake_gamedll.so)\n");
		tests_run--;
		return 0;
	}

	int ret = init_linkent_replacement(mm_handle, gd_handle);
	ASSERT_INT(ret, 1);

	// The jmp forwarder is written at the patch site, not the call target.
	ASSERT_INT(dlsym_patch_addr[0], 0xe9);
	// The call target (dlsym_original) must stay the real entry so a CET/IBT
	// indirect call lands on its endbr32; the forwarder sits past the endbr32.
	unsigned char *entry = (unsigned char *)(void *)dlsym_original;
	if (has_endbr32(entry)) {
		ASSERT_INT(entry[0], 0xf3);
		ASSERT_TRUE(dlsym_patch_addr == entry + ENDBR32_SIZE);
	} else {
		ASSERT_TRUE(dlsym_patch_addr == entry);
	}

	// After hook: dlsym on mm_handle should find GiveFnptrsToDll from gamedll
	void *after = dlsym(mm_handle, "GiveFnptrsToDll");
	ASSERT_PTR_NOT_NULL(after);

	// The found symbol should match the gamedll's symbol
	void *direct = dlsym(gd_handle, "GiveFnptrsToDll");
	ASSERT_TRUE(after == direct);

	// Non-metamod module lookups still work normally
	void *libc_sym = dlsym(RTLD_DEFAULT, "printf");
	ASSERT_PTR_NOT_NULL(libc_sym);

	// Symbols not in either module return NULL
	void *missing = dlsym(mm_handle, "NoSuchSymbol_xyz_12345");
	ASSERT_TRUE(missing == NULL);

	// Restore original dlsym
	restore_linkent();

	// After restore: dlsym on mm_handle should NOT find GiveFnptrsToDll
	void *restored = dlsym(mm_handle, "GiveFnptrsToDll");
	ASSERT_TRUE(restored == NULL);

	dlclose(gd_handle);
	dlclose(mm_handle);
	PASS();
	return 0;
}

static int test_linkent_init_null_handles(void)
{
	TEST("init_linkent_replacement - NULL handles still hooks (no crash)");
	mock_reset();

	int ret = init_linkent_replacement(NULL, NULL);
	ASSERT_INT(ret, 1);

	// dlsym should still work normally (pass-through path)
	void *sym = dlsym(RTLD_DEFAULT, "printf");
	ASSERT_PTR_NOT_NULL(sym);

	restore_linkent();
	PASS();
	return 0;
}

static int test_linkent_symbol_in_metamod(void)
{
	TEST("init_linkent_replacement - symbol in metamod module found directly");
	mock_reset();

	void *mm_handle = dlopen("./engine_test.so", RTLD_NOW);
	void *gd_handle = dlopen("./fake_gamedll.so", RTLD_NOW);
	if (!mm_handle || !gd_handle) {
		if (mm_handle) dlclose(mm_handle);
		if (gd_handle) dlclose(gd_handle);
		printf("SKIP (could not load test .so files)\n");
		tests_run--;
		return 0;
	}

	int ret = init_linkent_replacement(mm_handle, gd_handle);
	ASSERT_INT(ret, 1);

	// fake_engine_symbol is in engine_test.so (mm_handle) — found directly
	void *sym = dlsym(mm_handle, "fake_engine_symbol");
	ASSERT_PTR_NOT_NULL(sym);

	restore_linkent();

	dlclose(gd_handle);
	dlclose(mm_handle);
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

	printf("test_osdep_linkent_linux:\n");

	// Helper function tests
	fail |= test_construct_jmp_basic();
	fail |= test_construct_jmp_backward();
	fail |= test_construct_jmp_self();
	fail |= test_has_endbr32();
	fail |= test_trampoline_ff25();
	fail |= test_trampoline_not_ff25();
	fail |= test_trampoline_only_ff();
	fail |= test_trampoline_only_25();
	fail |= test_trampoline_zeros();
	fail |= test_extract_pointer();

	// Integration tests — skip under valgrind (code patching is incompatible)
	if (!RUNNING_ON_VALGRIND) {
		fail |= test_linkent_init_and_hook();
		fail |= test_linkent_init_null_handles();
		fail |= test_linkent_symbol_in_metamod();
	} else {
		printf("  (skipping integration tests under valgrind)\n");
	}

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
