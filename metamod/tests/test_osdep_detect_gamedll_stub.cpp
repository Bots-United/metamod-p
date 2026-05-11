//
// metamod-p - stubbed tests for osdep_detect_gamedll_linux.cpp
//
// Uses --wrap=mmap to stub mmap, and crafted binary ELF files
// to exercise code paths unreachable with real .so files.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

#include <extdll.h>

#include "metamod.h"
#include "osdep_p.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// mmap wrapper (linked with -Wl,--wrap=mmap)
// ============================================================

static bool force_mmap_fail = false;
static bool force_mmap_sigsegv = false;

extern "C" void *__real_mmap(void *addr, size_t length, int prot,
                             int flags, int fd, off_t offset);

extern "C" void *__wrap_mmap(void *addr, size_t length, int prot,
                             int flags, int fd, off_t offset)
{
	if (force_mmap_fail)
		return MAP_FAILED;
	void *result = __real_mmap(addr, length, prot, flags, fd, offset);
	if (force_mmap_sigsegv && result != MAP_FAILED && length > 4096) {
		void *page2 = (void *)(((unsigned long)result + 4096) & ~4095UL);
		mprotect(page2, 4096, PROT_NONE);
		force_mmap_sigsegv = false;
	}
	return result;
}

// ============================================================
// Helper: write raw bytes to a temp file
// ============================================================

static char elf_tmp_path[256];

static const char *write_elf_file(const void *data, size_t size)
{
	snprintf(elf_tmp_path, sizeof(elf_tmp_path),
	         "/tmp/metamod_test_elf_XXXXXX");
	int fd = mkstemp(elf_tmp_path);
	if (fd < 0) return NULL;
	write(fd, data, size);
	close(fd);
	return elf_tmp_path;
}

static void cleanup_elf_file(void)
{
	if (elf_tmp_path[0])
		unlink(elf_tmp_path);
	elf_tmp_path[0] = '\0';
}

// ============================================================
// Helper: build minimal valid ELF with configurable properties
// ============================================================

struct mini_elf {
	Elf32_Ehdr ehdr;
	Elf32_Shdr shdr[4];   // NULL + optional sections
	Elf32_Sym  syms[8];
	char       strtab[128];
};

static void init_valid_elf(struct mini_elf *elf, int num_shdr)
{
	memset(elf, 0, sizeof(*elf));

	memcpy(elf->ehdr.e_ident, ELFMAG, SELFMAG);
	elf->ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	elf->ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	elf->ehdr.e_type = ET_DYN;
	elf->ehdr.e_machine = EM_386;
	elf->ehdr.e_shoff = offsetof(struct mini_elf, shdr);
	elf->ehdr.e_shnum = num_shdr;
	elf->ehdr.e_shentsize = sizeof(Elf32_Shdr);

	// shdr[0] is always SHT_NULL
	elf->shdr[0].sh_type = SHT_NULL;
}

// ============================================================
// Tests
// ============================================================

static int test_mmap_failure(void)
{
	TEST("is_gamedll - mmap failure returns mFALSE");
	mock_reset();

	// Need a file that passes fopen and size check but mmap fails
	const char *path = write_elf_file("", 0);
	ASSERT_PTR_NOT_NULL(path);
	// Write enough bytes to pass the size check
	FILE *f = fopen(path, "wb");
	ASSERT_PTR_NOT_NULL(f);
	char buf[256];
	memset(buf, 0, sizeof(buf));
	fwrite(buf, 1, sizeof(buf), f);
	fclose(f);

	force_mmap_fail = true;
	mBOOL ret = is_gamedll(path);
	force_mmap_fail = false;

	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_wrong_arch(void)
{
	TEST("is_gamedll - wrong ELF machine (EM_ARM) returns mFALSE");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 1);
	elf.ehdr.e_machine = EM_ARM;

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_wrong_elf_class(void)
{
	TEST("is_gamedll - wrong ELF class (64-bit) returns mFALSE");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 1);
	elf.ehdr.e_ident[EI_CLASS] = ELFCLASS64;

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_wrong_elf_type(void)
{
	TEST("is_gamedll - wrong ELF type (ET_EXEC) returns mFALSE");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 1);
	elf.ehdr.e_type = ET_EXEC;

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_no_symtab(void)
{
	TEST("is_gamedll - ELF with no symbol tables returns mFALSE");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 2);
	// shdr[1]: SHT_PROGBITS (not a symbol table)
	elf.shdr[1].sh_type = SHT_PROGBITS;
	elf.shdr[1].sh_offset = 0;
	elf.shdr[1].sh_size = 0;

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_not_gamedll_symbols(void)
{
	TEST("is_gamedll - ELF with non-game symbols returns mFALSE");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 3);

	// Build string table: \0 some_func \0
	elf.strtab[0] = '\0';
	strcpy(&elf.strtab[1], "some_func");
	unsigned long strtab_len = 1 + strlen("some_func") + 1;

	// shdr[1]: SHT_STRTAB
	elf.shdr[1].sh_type = SHT_STRTAB;
	elf.shdr[1].sh_offset = offsetof(struct mini_elf, strtab);
	elf.shdr[1].sh_size = strtab_len;

	// shdr[2]: SHT_DYNSYM linking to shdr[1]
	elf.shdr[2].sh_type = SHT_DYNSYM;
	elf.shdr[2].sh_offset = offsetof(struct mini_elf, syms);
	elf.shdr[2].sh_size = 2 * sizeof(Elf32_Sym);
	elf.shdr[2].sh_entsize = sizeof(Elf32_Sym);
	elf.shdr[2].sh_link = 1;

	// sym[0]: null entry
	memset(&elf.syms[0], 0, sizeof(Elf32_Sym));
	// sym[1]: STT_FUNC + STB_GLOBAL, name="some_func"
	memset(&elf.syms[1], 0, sizeof(Elf32_Sym));
	elf.syms[1].st_name = 1;
	elf.syms[1].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_gamedll_getentityapi_v1(void)
{
	TEST("is_gamedll - ELF with GiveFnptrsToDll+GetEntityAPI returns mTRUE");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 3);

	// Build string table
	elf.strtab[0] = '\0';
	unsigned long off = 1;
	strcpy(&elf.strtab[off], "GiveFnptrsToDll");
	unsigned long give_off = off;
	off += strlen("GiveFnptrsToDll") + 1;
	strcpy(&elf.strtab[off], "GetEntityAPI");
	unsigned long getapi_off = off;
	off += strlen("GetEntityAPI") + 1;

	// shdr[1]: SHT_STRTAB
	elf.shdr[1].sh_type = SHT_STRTAB;
	elf.shdr[1].sh_offset = offsetof(struct mini_elf, strtab);
	elf.shdr[1].sh_size = off;

	// shdr[2]: SHT_DYNSYM
	elf.shdr[2].sh_type = SHT_DYNSYM;
	elf.shdr[2].sh_offset = offsetof(struct mini_elf, syms);
	elf.shdr[2].sh_size = 3 * sizeof(Elf32_Sym);
	elf.shdr[2].sh_entsize = sizeof(Elf32_Sym);
	elf.shdr[2].sh_link = 1;

	// sym[0]: null
	memset(&elf.syms[0], 0, sizeof(Elf32_Sym));
	// sym[1]: GiveFnptrsToDll
	memset(&elf.syms[1], 0, sizeof(Elf32_Sym));
	elf.syms[1].st_name = give_off;
	elf.syms[1].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
	// sym[2]: GetEntityAPI
	memset(&elf.syms[2], 0, sizeof(Elf32_Sym));
	elf.syms[2].st_name = getapi_off;
	elf.syms[2].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mTRUE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_symtab_fallback(void)
{
	TEST("is_gamedll - SHT_SYMTAB fallback when no SHT_DYNSYM");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 3);

	// Build string table
	elf.strtab[0] = '\0';
	unsigned long off = 1;
	strcpy(&elf.strtab[off], "GiveFnptrsToDll");
	unsigned long give_off = off;
	off += strlen("GiveFnptrsToDll") + 1;
	strcpy(&elf.strtab[off], "GetEntityAPI2");
	unsigned long getapi2_off = off;
	off += strlen("GetEntityAPI2") + 1;

	// shdr[1]: SHT_STRTAB
	elf.shdr[1].sh_type = SHT_STRTAB;
	elf.shdr[1].sh_offset = offsetof(struct mini_elf, strtab);
	elf.shdr[1].sh_size = off;

	// shdr[2]: SHT_SYMTAB (NOT SHT_DYNSYM)
	elf.shdr[2].sh_type = SHT_SYMTAB;
	elf.shdr[2].sh_offset = offsetof(struct mini_elf, syms);
	elf.shdr[2].sh_size = 3 * sizeof(Elf32_Sym);
	elf.shdr[2].sh_entsize = sizeof(Elf32_Sym);
	elf.shdr[2].sh_link = 1;

	memset(&elf.syms[0], 0, sizeof(Elf32_Sym));
	memset(&elf.syms[1], 0, sizeof(Elf32_Sym));
	elf.syms[1].st_name = give_off;
	elf.syms[1].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
	memset(&elf.syms[2], 0, sizeof(Elf32_Sym));
	elf.syms[2].st_name = getapi2_off;
	elf.syms[2].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mTRUE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_invalid_stname(void)
{
	TEST("is_gamedll - symbol with st_name out of range is skipped");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 3);

	elf.strtab[0] = '\0';
	strcpy(&elf.strtab[1], "GiveFnptrsToDll");
	strcpy(&elf.strtab[17], "GetEntityAPI2");
	unsigned long strtab_len = 31;

	elf.shdr[1].sh_type = SHT_STRTAB;
	elf.shdr[1].sh_offset = offsetof(struct mini_elf, strtab);
	elf.shdr[1].sh_size = strtab_len;

	elf.shdr[2].sh_type = SHT_DYNSYM;
	elf.shdr[2].sh_offset = offsetof(struct mini_elf, syms);
	elf.shdr[2].sh_size = 4 * sizeof(Elf32_Sym);
	elf.shdr[2].sh_entsize = sizeof(Elf32_Sym);
	elf.shdr[2].sh_link = 1;

	memset(elf.syms, 0, sizeof(elf.syms));
	// sym[1]: invalid st_name (past strtab end)
	elf.syms[1].st_name = 999;
	elf.syms[1].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
	// sym[2]: valid GiveFnptrsToDll
	elf.syms[2].st_name = 1;
	elf.syms[2].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
	// sym[3]: valid GetEntityAPI2
	elf.syms[3].st_name = 17;
	elf.syms[3].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mTRUE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_dynsym_invalid_offset(void)
{
	TEST("is_gamedll - SHT_DYNSYM with invalid sh_offset triggers elf_error_exit");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 3);

	// shdr[1]: SHT_STRTAB (valid)
	elf.shdr[1].sh_type = SHT_STRTAB;
	elf.shdr[1].sh_offset = offsetof(struct mini_elf, strtab);
	elf.shdr[1].sh_size = 1;

	// shdr[2]: SHT_DYNSYM with sh_offset past EOF
	elf.shdr[2].sh_type = SHT_DYNSYM;
	elf.shdr[2].sh_offset = sizeof(struct mini_elf) + 9999;
	elf.shdr[2].sh_size = sizeof(Elf32_Sym);
	elf.shdr[2].sh_entsize = sizeof(Elf32_Sym);
	elf.shdr[2].sh_link = 1;

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_symtab_invalid_offset(void)
{
	TEST("is_gamedll - SHT_SYMTAB with invalid sh_offset triggers elf_error_exit");
	mock_reset();

	struct mini_elf elf;
	init_valid_elf(&elf, 3);

	// shdr[1]: SHT_STRTAB (valid)
	elf.shdr[1].sh_type = SHT_STRTAB;
	elf.shdr[1].sh_offset = offsetof(struct mini_elf, strtab);
	elf.shdr[1].sh_size = 1;

	// shdr[2]: SHT_SYMTAB (NOT DYNSYM) with sh_offset past EOF
	elf.shdr[2].sh_type = SHT_SYMTAB;
	elf.shdr[2].sh_offset = sizeof(struct mini_elf) + 9999;
	elf.shdr[2].sh_size = sizeof(Elf32_Sym);
	elf.shdr[2].sh_entsize = sizeof(Elf32_Sym);
	elf.shdr[2].sh_link = 1;

	const char *path = write_elf_file(&elf, sizeof(elf));
	ASSERT_PTR_NOT_NULL(path);

	mBOOL ret = is_gamedll(path);
	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
	PASS();
	return 0;
}

static int test_sigsegv_recovery(void)
{
	TEST("is_gamedll - SIGSEGV during ELF parsing triggers signal handler");
	mock_reset();

	// Create a large ELF (> 2 pages) with section headers in the 2nd page.
	// The mmap wrapper will mprotect the 2nd page to PROT_NONE.
	char buf[8192];
	memset(buf, 0, sizeof(buf));

	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buf;
	memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_CLASS] = ELFCLASS32;
	ehdr->e_type = ET_DYN;
	ehdr->e_machine = EM_386;
	// Place section headers in 2nd page — will be PROT_NONE
	ehdr->e_shoff = 4200;
	ehdr->e_shnum = 2;
	ehdr->e_shentsize = sizeof(Elf32_Shdr);

	const char *path = write_elf_file(buf, sizeof(buf));
	ASSERT_PTR_NOT_NULL(path);

	force_mmap_sigsegv = true;
	mBOOL ret = is_gamedll(path);
	force_mmap_sigsegv = false;

	ASSERT_TRUE(ret == mFALSE);
	cleanup_elf_file();
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

	printf("test_osdep_detect_gamedll_stub:\n");
	fail |= test_mmap_failure();
	fail |= test_wrong_arch();
	fail |= test_wrong_elf_class();
	fail |= test_wrong_elf_type();
	fail |= test_no_symtab();
	fail |= test_not_gamedll_symbols();
	fail |= test_gamedll_getentityapi_v1();
	fail |= test_symtab_fallback();
	fail |= test_invalid_stname();
	fail |= test_dynsym_invalid_offset();
	fail |= test_symtab_invalid_offset();
	fail |= test_sigsegv_recovery();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
