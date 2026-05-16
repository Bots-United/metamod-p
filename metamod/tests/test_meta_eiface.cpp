//
// metamod-p - tests for meta_eiface.cpp
//

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <extdll.h>

#include "metamod.h"
#include "meta_eiface.h"
#include "engine_t.h"
#include "dllapi.h"

#include "engine_mock.h"
#include "test_common.h"

// Helper to reset the static sm_version in meta_enginefuncs_t (protected)
struct engfuncs_version_resetter : public meta_enginefuncs_t {
	static void reset() { sm_version = 0; }
	static void set(int v) { sm_version = v; }
};

// Fake engine .so for valid code pointer tests
static void *fake_engine_handle = NULL;
static void *fake_engine_sym = NULL;

static int load_fake_engine(void)
{
	fake_engine_handle = dlopen("./engine_test.so", RTLD_NOW);
	if (!fake_engine_handle) return 1;
	fake_engine_sym = dlsym(fake_engine_handle, "fake_engine_symbol");
	return fake_engine_sym ? 0 : 1;
}

static void unload_fake_engine(void)
{
	if (fake_engine_handle) {
		dlclose(fake_engine_handle);
		fake_engine_handle = NULL;
		fake_engine_sym = NULL;
	}
}

// Set signature function pointers in enginefuncs_t to a valid address
static void set_sig_pointers(enginefuncs_t *ef, void *valid_addr)
{
	typedef void (*voidfn)(void);
	voidfn va = (voidfn)valid_addr;

	ef->pfnGetPlayerAuthId = (const char *(*)(edict_t *))va;
	ef->pfnSequenceGet = (sequenceEntry_s *(*)(const char *, const char *))va;
	ef->pfnSequencePickSentence = (sentenceEntry_s *(*)(const char *, int, int *))va;
	ef->pfnGetFileSize = (int (*)(char *))va;
	ef->pfnGetApproxWavePlayLen = (unsigned int (*)(const char *))va;
	ef->pfnIsCareerMatch = (int (*)(void))va;
	ef->pfnGetLocalizedStringLength = (int (*)(const char *))va;
	ef->pfnRegisterTutorMessageShown = (void (*)(int))va;
	ef->pfnGetTimesTutorMessageShown = (int (*)(int))va;
	ef->pfnProcessTutorMessageDecayBuffer = (void (*)(int *, int))va;
	ef->pfnConstructTutorMessageDecayBuffer = (void (*)(int *, int))va;
	ef->pfnResetTutorMessageDecayData = (void (*)(void))va;
	ef->pfnQueryClientCvarValue = (void (*)(const edict_t *, const char *))va;
	ef->pfnQueryClientCvarValue2 = (void (*)(const edict_t *, const char *, int))va;
	ef->pfnEngCheckParm = (int (*)(const char *, char **))va;
	ef->pfnPEntityOfEntIndexAllEntities = (edict_t *(*)(int))va;
}

// ============================================================
// meta_new_dll_functions_t tests
// ============================================================

static void dummy_OnFreeEntPrivateData(edict_t *) {}
static void dummy_GameShutdown(void) {}
static int dummy_ShouldCollide(edict_t *, edict_t *) { return 0; }
static void dummy_CvarValue(const edict_t *, const char *) {}
static void dummy_CvarValue2(const edict_t *, int, const char *, const char *) {}

static int test_meta_new_dll_funcs_constructor(void)
{
	TEST("meta_new_dll_functions_t - parameterized constructor");
	meta_new_dll_functions_t ndf(
		dummy_OnFreeEntPrivateData,
		dummy_GameShutdown,
		dummy_ShouldCollide,
		dummy_CvarValue,
		dummy_CvarValue2
	);
	ASSERT_PTR_EQ((void *)ndf.pfnOnFreeEntPrivateData, (void *)dummy_OnFreeEntPrivateData);
	ASSERT_PTR_EQ((void *)ndf.pfnGameShutdown, (void *)dummy_GameShutdown);
	ASSERT_PTR_EQ((void *)ndf.pfnShouldCollide, (void *)dummy_ShouldCollide);
	ASSERT_PTR_EQ((void *)ndf.pfnCvarValue, (void *)dummy_CvarValue);
	ASSERT_PTR_EQ((void *)ndf.pfnCvarValue2, (void *)dummy_CvarValue2);
	PASS();
	return 0;
}

static int test_meta_new_dll_funcs_default(void)
{
	TEST("meta_new_dll_functions_t - default constructor zeroes");
	meta_new_dll_functions_t ndf;
	ASSERT_PTR_NULL(ndf.pfnOnFreeEntPrivateData);
	ASSERT_PTR_NULL(ndf.pfnGameShutdown);
	PASS();
	return 0;
}

static int test_meta_new_dll_funcs_copy(void)
{
	TEST("meta_new_dll_functions_t - copy constructor");
	meta_new_dll_functions_t ndf1(
		dummy_OnFreeEntPrivateData, dummy_GameShutdown,
		dummy_ShouldCollide, dummy_CvarValue, dummy_CvarValue2
	);
	meta_new_dll_functions_t ndf2(ndf1);
	ASSERT_PTR_EQ((void *)ndf2.pfnGameShutdown, (void *)dummy_GameShutdown);
	PASS();
	return 0;
}

static int test_meta_new_dll_funcs_assign(void)
{
	TEST("meta_new_dll_functions_t - assignment operator");
	meta_new_dll_functions_t ndf1(
		dummy_OnFreeEntPrivateData, dummy_GameShutdown,
		dummy_ShouldCollide, dummy_CvarValue, dummy_CvarValue2
	);
	meta_new_dll_functions_t ndf2;
	ndf2 = ndf1;
	ASSERT_PTR_EQ((void *)ndf2.pfnOnFreeEntPrivateData, (void *)dummy_OnFreeEntPrivateData);
	PASS();
	return 0;
}

static int test_meta_new_dll_funcs_set_from(void)
{
	TEST("meta_new_dll_functions_t - set_from copies fields");
	NEW_DLL_FUNCTIONS src;
	memset(&src, 0, sizeof(src));
	src.pfnGameShutdown = dummy_GameShutdown;
	meta_new_dll_functions_t ndf;
	ndf.set_from(&src);
	ASSERT_PTR_EQ((void *)ndf.pfnGameShutdown, (void *)dummy_GameShutdown);
	PASS();
	return 0;
}

// ============================================================
// meta_enginefuncs_t tests
// ============================================================

static int test_meta_engfuncs_default(void)
{
	TEST("meta_enginefuncs_t - default constructor zeroes");
	meta_enginefuncs_t ef;
	ASSERT_PTR_NULL(ef.pfnPrecacheModel);
	ASSERT_PTR_NULL(ef.pfnAlertMessage);
	PASS();
	return 0;
}

static int test_meta_engfuncs_copy(void)
{
	TEST("meta_enginefuncs_t - copy constructor");
	meta_enginefuncs_t ef1;
	ef1.pfnPrecacheModel = (int (*)(char *))0x12345678;
	meta_enginefuncs_t ef2(ef1);
	ASSERT_PTR_EQ((void *)ef2.pfnPrecacheModel, (void *)0x12345678);
	PASS();
	return 0;
}

static int test_meta_engfuncs_assign(void)
{
	TEST("meta_enginefuncs_t - assignment operator");
	meta_enginefuncs_t ef1;
	ef1.pfnAlertMessage = (void (*)(ALERT_TYPE, char *, ...))0xAABBCCDD;
	meta_enginefuncs_t ef2;
	ef2 = ef1;
	ASSERT_PTR_EQ((void *)ef2.pfnAlertMessage, (void *)0xAABBCCDD);
	PASS();
	return 0;
}

static int test_meta_engfuncs_set_from_copy_to(void)
{
	TEST("meta_enginefuncs_t - set_from/copy_to roundtrip");
	enginefuncs_t src;
	memset(&src, 0, sizeof(src));
	src.pfnPrecacheModel = (int (*)(char *))0xDEADBEEF;
	meta_enginefuncs_t ef;
	ef.set_from(&src);
	ASSERT_PTR_EQ((void *)ef.pfnPrecacheModel, (void *)0xDEADBEEF);
	enginefuncs_t dst;
	memset(&dst, 0, sizeof(dst));
	ef.copy_to(&dst);
	ASSERT_PTR_EQ((void *)dst.pfnPrecacheModel, (void *)0xDEADBEEF);
	PASS();
	return 0;
}

// ============================================================
// HL_enginefuncs_t / determine_engine_interface_version tests
// ============================================================

static int test_hl_engfuncs_init_version_138(void)
{
	TEST("HL_enginefuncs_t::initialise - all NULL gives version 138");
	mock_reset();
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 138);
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_144(void)
{
	TEST("HL_enginefuncs_t::initialise - auth only gives version 144");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	ef.pfnGetPlayerAuthId = (const char *(*)(edict_t *))fake_engine_sym;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 144);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_147(void)
{
	TEST("HL_enginefuncs_t::initialise - through GetFileSize gives 147");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	ef.pfnGetPlayerAuthId = (const char *(*)(edict_t *))fake_engine_sym;
	ef.pfnSequenceGet = (sequenceEntry_s *(*)(const char *, const char *))fake_engine_sym;
	ef.pfnSequencePickSentence = (sentenceEntry_s *(*)(const char *, int, int *))fake_engine_sym;
	ef.pfnGetFileSize = (int (*)(char *))fake_engine_sym;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 147);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_155(void)
{
	TEST("HL_enginefuncs_t::initialise - through ResetTutor gives 155");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	set_sig_pointers(&ef, fake_engine_sym);
	ef.pfnQueryClientCvarValue = NULL;
	ef.pfnQueryClientCvarValue2 = NULL;
	ef.pfnEngCheckParm = NULL;
	ef.pfnPEntityOfEntIndexAllEntities = NULL;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 155);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_156(void)
{
	TEST("HL_enginefuncs_t::initialise - through QueryCvar gives 156");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	set_sig_pointers(&ef, fake_engine_sym);
	ef.pfnQueryClientCvarValue2 = NULL;
	ef.pfnEngCheckParm = NULL;
	ef.pfnPEntityOfEntIndexAllEntities = NULL;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 156);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_157(void)
{
	TEST("HL_enginefuncs_t::initialise - through QueryCvar2 gives 157");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	set_sig_pointers(&ef, fake_engine_sym);
	ef.pfnEngCheckParm = NULL;
	ef.pfnPEntityOfEntIndexAllEntities = NULL;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 157);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_158(void)
{
	TEST("HL_enginefuncs_t::initialise - through EngCheckParm gives 158");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	set_sig_pointers(&ef, fake_engine_sym);
	ef.pfnPEntityOfEntIndexAllEntities = NULL;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 158);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_init_version_159(void)
{
	TEST("HL_enginefuncs_t::initialise - all valid gives version 159");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	set_sig_pointers(&ef, fake_engine_sym);
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 159);
	unload_fake_engine();
	PASS();
	return 0;
}

static int test_hl_engfuncs_cached_version(void)
{
	TEST("HL_enginefuncs_t::initialise - cached version skips detect");
	mock_reset();
	engfuncs_version_resetter::set(999);
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 999);
	engfuncs_version_resetter::reset();
	PASS();
	return 0;
}

// ============================================================
// fixup_engine_interface verification
// ============================================================

static int test_fixup_version_138(void)
{
	TEST("fixup - version 138 clears all sig ptrs");
	mock_reset();
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	ef.pfnGetPlayerAuthId = (const char *(*)(edict_t *))0x1;
	ef.pfnSequenceGet = (sequenceEntry_s *(*)(const char *, const char *))0x1;
	ef.pfnQueryClientCvarValue = (void (*)(const edict_t *, const char *))0x1;
	ef.pfnEngCheckParm = (int (*)(const char *, char **))0x1;
	ef.pfnPEntityOfEntIndexAllEntities = (edict_t *(*)(int))0x1;
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 138);
	ASSERT_PTR_NULL(hlef.pfnGetPlayerAuthId);
	ASSERT_PTR_NULL(hlef.pfnSequenceGet);
	ASSERT_PTR_NULL(hlef.pfnQueryClientCvarValue);
	ASSERT_PTR_NULL(hlef.pfnEngCheckParm);
	ASSERT_PTR_NULL(hlef.pfnPEntityOfEntIndexAllEntities);
	PASS();
	return 0;
}

static int test_fixup_version_159_keeps_ptrs(void)
{
	TEST("fixup - version 159 keeps all sig ptrs");
	mock_reset();
	if (load_fake_engine()) {
		printf("SKIP\n"); tests_run--; return 0;
	}
	Engine.info.initialise(NULL);
	engfuncs_version_resetter::reset();
	enginefuncs_t ef;
	memset(&ef, 0, sizeof(ef));
	set_sig_pointers(&ef, fake_engine_sym);
	HL_enginefuncs_t hlef;
	hlef.initialise_interface(&ef);
	ASSERT_INT(meta_enginefuncs_t::version(), 159);
	ASSERT_PTR_NOT_NULL(hlef.pfnGetPlayerAuthId);
	ASSERT_PTR_NOT_NULL(hlef.pfnEngCheckParm);
	ASSERT_PTR_NOT_NULL(hlef.pfnPEntityOfEntIndexAllEntities);
	unload_fake_engine();
	PASS();
	return 0;
}

// ============================================================
// meta_new_dll determine_interface_version and copy_to
// ============================================================

static int test_new_dll_version_from_engfuncs(void)
{
	TEST("meta_new_dll_functions_t::version - derives from engfuncs");
	// meta_new_dll sm_version is static, first call caches it.
	// With engfuncs version 138 (< 156), dll version becomes 1.
	engfuncs_version_resetter::set(138);
	meta_new_dll_functions_t ndf;
	int v = ndf.version();
	ASSERT_INT(v, 1);
	PASS();
	return 0;
}

static int test_new_dll_copy_to(void)
{
	TEST("meta_new_dll_functions_t::copy_to - copies fields");
	meta_new_dll_functions_t ndf(
		dummy_OnFreeEntPrivateData, dummy_GameShutdown,
		dummy_ShouldCollide, dummy_CvarValue, dummy_CvarValue2
	);
	NEW_DLL_FUNCTIONS dst;
	memset(&dst, 0, sizeof(dst));
	ndf.copy_to(&dst);
	ASSERT_PTR_EQ((void *)dst.pfnOnFreeEntPrivateData, (void *)dummy_OnFreeEntPrivateData);
	ASSERT_PTR_EQ((void *)dst.pfnGameShutdown, (void *)dummy_GameShutdown);
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

	printf("test_meta_eiface:\n");

	// meta_new_dll_functions_t basics
	fail |= test_meta_new_dll_funcs_constructor();
	fail |= test_meta_new_dll_funcs_default();
	fail |= test_meta_new_dll_funcs_copy();
	fail |= test_meta_new_dll_funcs_assign();
	fail |= test_meta_new_dll_funcs_set_from();

	// meta_enginefuncs_t basics
	fail |= test_meta_engfuncs_default();
	fail |= test_meta_engfuncs_copy();
	fail |= test_meta_engfuncs_assign();
	fail |= test_meta_engfuncs_set_from_copy_to();

	// meta_new_dll version/copy_to - MUST run before HL version tests
	// so that meta_new_dll_functions_t::sm_version is set from known state
	fail |= test_new_dll_version_from_engfuncs();
	fail |= test_new_dll_copy_to();

	// HL_enginefuncs_t version detection
	fail |= test_hl_engfuncs_init_version_138();
	fail |= test_hl_engfuncs_init_version_144();
	fail |= test_hl_engfuncs_init_version_147();
	fail |= test_hl_engfuncs_init_version_155();
	fail |= test_hl_engfuncs_init_version_156();
	fail |= test_hl_engfuncs_init_version_157();
	fail |= test_hl_engfuncs_init_version_158();
	fail |= test_hl_engfuncs_init_version_159();
	fail |= test_hl_engfuncs_cached_version();

	// fixup verification
	fail |= test_fixup_version_138();
	fail |= test_fixup_version_159_keeps_ptrs();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
