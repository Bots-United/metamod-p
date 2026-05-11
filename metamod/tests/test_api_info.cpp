//
// metamod-p - tests for api_caller trampoline functions (api_hook.cpp)
// and api_info tables (api_info.cpp)
//
// Each api_caller unpacks a pack_args_type_X struct and calls through
// a function pointer with the right signature. These tests verify all
// 71 trampolines correctly dispatch and pass return values.
//

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include <extdll.h>

#include "metamod.h"
#include "api_info.h"
#include "api_hook.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Global state for capturing mock calls
// ============================================================

static int g_called;
static intptr_t g_a[12];

#define RESET() do { g_called = 0; memset(g_a, 0, sizeof(g_a)); } while(0)

static float ret_to_float(void *p) {
	float f;
	memcpy(&f, &p, sizeof(f));
	return f;
}

// ============================================================
// Mock functions - void/ptr return (caller casts to void*(...))
// ============================================================

static void *mv_void(void) { g_called=1; return (void*)0xBEEF; }
static void *mv_i(int a) { g_called=1; g_a[0]=a; return (void*)0xBEEF; }
static void *mv_2i(int a, int b) { g_called=1; g_a[0]=a; g_a[1]=b; return (void*)0xBEEF; }
static void *mv_ui(unsigned int a) { g_called=1; g_a[0]=(intptr_t)a; return (void*)0xBEEF; }
static void *mv_f(float) { g_called=1; return (void*)0xBEEF; }
static void *mv_p(const void *a) { g_called=1; g_a[0]=(intptr_t)a; return (void*)0xBEEF; }
static void *mv_2p(const void *a, const void *b) { g_called=1; g_a[0]=(intptr_t)a; g_a[1]=(intptr_t)b; return (void*)0xBEEF; }
static void *mv_3p(const void *a, const void *, const void *c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=(intptr_t)c; return (void*)0xBEEF; }
static void *mv_4p(const void *a, const void *, const void *, const void *d) { g_called=1; g_a[0]=(intptr_t)a; g_a[3]=(intptr_t)d; return (void*)0xBEEF; }
static void *mv_ip(int a, const void *b) { g_called=1; g_a[0]=a; g_a[1]=(intptr_t)b; return (void*)0xBEEF; }
static void *mv_i2p(int a, const void *, const void *c) { g_called=1; g_a[0]=a; g_a[2]=(intptr_t)c; return (void*)0xBEEF; }
static void *mv_i3p(int a, const void *, const void *, const void *d) { g_called=1; g_a[0]=a; g_a[3]=(intptr_t)d; return (void*)0xBEEF; }
static void *mv_pi(const void *a, int b) { g_called=1; g_a[0]=(intptr_t)a; g_a[1]=b; return (void*)0xBEEF; }
static void *mv_2i2p(int a, int, const void *, const void *d) { g_called=1; g_a[0]=a; g_a[3]=(intptr_t)d; return (void*)0xBEEF; }
static void *mv_2i2pi2p(int a, int, const void *, const void *, int, const void *, const void *g) { g_called=1; g_a[0]=a; g_a[6]=(intptr_t)g; return (void*)0xBEEF; }
static void *mv_2p2f(const void *a, const void *, float, float) { g_called=1; g_a[0]=(intptr_t)a; return (void*)0xBEEF; }
static void *mv_2p2i2p(const void *a, const void *, int, int, const void *, const void *f) { g_called=1; g_a[0]=(intptr_t)a; g_a[5]=(intptr_t)f; return (void*)0xBEEF; }
static void *mv_2p3fus2uc(const void *a, const void *, float, float, float, unsigned short f, unsigned char, unsigned char h) { g_called=1; g_a[0]=(intptr_t)a; g_a[5]=(int)f; g_a[7]=(int)h; return (void*)0xBEEF; }
static void *mv_2pf(const void *a, const void *, float) { g_called=1; g_a[0]=(intptr_t)a; return (void*)0xBEEF; }
static void *mv_2pfi(const void *a, const void *, float, int d) { g_called=1; g_a[0]=(intptr_t)a; g_a[3]=d; return (void*)0xBEEF; }
static void *mv_2pi(const void *a, const void *, int c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=c; return (void*)0xBEEF; }
static void *mv_2pui(const void *a, const void *, unsigned int c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=(intptr_t)c; return (void*)0xBEEF; }
static void *mv_2pi2p(const void *a, const void *, int, const void *, const void *e) { g_called=1; g_a[0]=(intptr_t)a; g_a[4]=(intptr_t)e; return (void*)0xBEEF; }
static void *mv_2pif2p(const void *a, const void *, int, float, const void *, const void *f) { g_called=1; g_a[0]=(intptr_t)a; g_a[5]=(intptr_t)f; return (void*)0xBEEF; }
static void *mv_3p2f2i(const void *a, const void *, const void *, float, float, int, int g) { g_called=1; g_a[0]=(intptr_t)a; g_a[6]=g; return (void*)0xBEEF; }
static void *mv_4pi(const void *a, const void *, const void *, const void *, int e) { g_called=1; g_a[0]=(intptr_t)a; g_a[4]=e; return (void*)0xBEEF; }
static void *mv_pf(const void *a, float) { g_called=1; g_a[0]=(intptr_t)a; return (void*)0xBEEF; }
static void *mv_puc(const void *a, unsigned char b) { g_called=1; g_a[0]=(intptr_t)a; g_a[1]=(int)b; return (void*)0xBEEF; }
static void *mv_pfp(const void *a, float, const void *c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=(intptr_t)c; return (void*)0xBEEF; }
static void *mv_p2i(const void *a, int, int c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=c; return (void*)0xBEEF; }
static void *mv_p3i(const void *a, int, int, int d) { g_called=1; g_a[0]=(intptr_t)a; g_a[3]=d; return (void*)0xBEEF; }
static void *mv_p4i(const void *a, int, int, int, int e) { g_called=1; g_a[0]=(intptr_t)a; g_a[4]=e; return (void*)0xBEEF; }
static void *mv_p2f(const void *a, float, float) { g_called=1; g_a[0]=(intptr_t)a; return (void*)0xBEEF; }
static void *mv_pi2p(const void *a, int, const void *, const void *d) { g_called=1; g_a[0]=(intptr_t)a; g_a[3]=(intptr_t)d; return (void*)0xBEEF; }
static void *mv_pip(const void *a, int, const void *c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=(intptr_t)c; return (void*)0xBEEF; }
static void *mv_pip2f2i(const void *a, int, const void *, float, float, int, int g) { g_called=1; g_a[0]=(intptr_t)a; g_a[6]=g; return (void*)0xBEEF; }
static void *mv_pip2f4i2p(const void *a, int, const void *, float, float, int, int, int, int, const void *, const void *k) { g_called=1; g_a[0]=(intptr_t)a; g_a[10]=(intptr_t)k; return (void*)0xBEEF; }
static void *mv_ipusf2p2f4i(int a, const void *, unsigned short, float, const void *, const void *, float, float, int, int, int, int l) { g_called=1; g_a[0]=a; g_a[11]=l; return (void*)0xBEEF; }
static void *mv_ipV(int a, const void *b, ...) { g_called=1; g_a[0]=a; g_a[1]=(intptr_t)b; return (void*)0xBEEF; }
static void *mv_2pV(const void *a, const void *b, ...) { g_called=1; g_a[0]=(intptr_t)a; g_a[1]=(intptr_t)b; return (void*)0xBEEF; }

// ============================================================
// Mock functions - int return
// ============================================================

static int mi_void(void) { g_called=1; return 42; }
static int mi_i(int a) { g_called=1; g_a[0]=a; return 42; }
static int mi_2i(int a, int b) { g_called=1; g_a[0]=a; g_a[1]=b; return 42; }
static int mi_3i(int a, int, int c) { g_called=1; g_a[0]=a; g_a[2]=c; return 42; }
static int mi_p(const void *a) { g_called=1; g_a[0]=(intptr_t)a; return 42; }
static int mi_2p(const void *a, const void *b) { g_called=1; g_a[0]=(intptr_t)a; g_a[1]=(intptr_t)b; return 42; }
static int mi_3p(const void *a, const void *, const void *c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=(intptr_t)c; return 42; }
static int mi_4p(const void *a, const void *, const void *, const void *d) { g_called=1; g_a[0]=(intptr_t)a; g_a[3]=(intptr_t)d; return 42; }
static int mi_2pi(const void *a, const void *, int c) { g_called=1; g_a[0]=(intptr_t)a; g_a[2]=c; return 42; }
static int mi_ip(int a, const void *b) { g_called=1; g_a[0]=a; g_a[1]=(intptr_t)b; return 42; }
static int mi_i2p(int a, const void *, const void *c) { g_called=1; g_a[0]=a; g_a[2]=(intptr_t)c; return 42; }
static int mi_pi(const void *a, int b) { g_called=1; g_a[0]=(intptr_t)a; g_a[1]=b; return 42; }
static int mi_4pi(const void *a, const void *, const void *, const void *, int e) { g_called=1; g_a[0]=(intptr_t)a; g_a[4]=e; return 42; }
static int mi_3pi2p(const void *a, const void *, const void *, int, const void *, const void *f) { g_called=1; g_a[0]=(intptr_t)a; g_a[5]=(intptr_t)f; return 42; }
static int mi_pi2p2ip(const void *a, int, const void *, const void *, int, int, const void *g) { g_called=1; g_a[0]=(intptr_t)a; g_a[6]=(intptr_t)g; return 42; }
static int mi_p2fi(const void *a, float, float, int d) { g_called=1; g_a[0]=(intptr_t)a; g_a[3]=d; return 42; }

// ============================================================
// Mock functions - float return
// ============================================================

static float mf_void(void) { g_called=1; return 1.5f; }
static float mf_2f(float, float) { g_called=1; return 1.5f; }
static float mf_p(const void *a) { g_called=1; g_a[0]=(intptr_t)a; return 1.5f; }

// ============================================================
// Mock functions - uint/ulong/ushort/char return
// ============================================================

static unsigned int mu_ui(unsigned int a) { g_called=1; g_a[0]=(intptr_t)a; return 99; }
static unsigned int mu_p(const void *a) { g_called=1; g_a[0]=(intptr_t)a; return 99; }
static unsigned long mul_ul(unsigned long a) { g_called=1; g_a[0]=(intptr_t)a; return 0xDEADul; }
static unsigned short mus_ip(int a, const void *b) { g_called=1; g_a[0]=a; g_a[1]=(intptr_t)b; return 777; }
static char mc_p(const void *a) { g_called=1; g_a[0]=(intptr_t)a; return 'Z'; }

// ============================================================
// Tests
// ============================================================

static int test_callers_void_args(void)
{
	TEST("api_caller - void/ptr/int/float with void args");
	pack_args_type_void pa(0);
	void *r;

	RESET();
	api_caller_void_args_void((const void*)mv_void, &pa);
	ASSERT_TRUE(g_called == 1);

	RESET();
	r = api_caller_ptr_args_void((const void*)mv_void, &pa);
	ASSERT_TRUE(g_called == 1);
	ASSERT_TRUE(r == (void*)0xBEEF);

	RESET();
	r = api_caller_int_args_void((const void*)mi_void, &pa);
	ASSERT_TRUE(g_called == 1);
	ASSERT_TRUE((int)(intptr_t)r == 42);

	RESET();
	r = api_caller_float_args_void((const void*)mf_void, &pa);
	ASSERT_TRUE(g_called == 1);
	ASSERT_TRUE(ret_to_float(r) == 1.5f);

	PASS();
	return 0;
}

static int test_callers_int_float_args(void)
{
	TEST("api_caller - i/2i/3i/ui/ul/f/2f arg types");
	void *r;

	{
		pack_args_type_i pa(11);
		RESET(); api_caller_void_args_i((const void*)mv_i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11);
		RESET(); r = api_caller_int_args_i((const void*)mi_i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11);
		ASSERT_TRUE((int)(intptr_t)r == 42);
		RESET(); r = api_caller_ptr_args_i((const void*)mv_i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
	}
	{
		pack_args_type_2i pa(11, 22);
		RESET(); api_caller_void_args_2i((const void*)mv_2i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[1] == 22);
		RESET(); r = api_caller_int_args_2i((const void*)mi_2i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_3i pa(11, 22, 33);
		RESET(); r = api_caller_int_args_3i((const void*)mi_3i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11);
		ASSERT_TRUE(g_a[2] == 33); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_ui pa(99999u);
		RESET(); r = api_caller_uint_args_ui((const void*)mu_ui, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((unsigned int)(uintptr_t)r == 99);
		RESET(); r = api_caller_ptr_args_ui((const void*)mv_ui, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
	}
	{
		pack_args_type_ul pa(0xDEADul);
		RESET(); r = api_caller_ulong_args_ul((const void*)mul_ul, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((unsigned long)(uintptr_t)r == 0xDEADul);
	}
	{
		pack_args_type_f pa(1.25f);
		RESET(); api_caller_void_args_f((const void*)mv_f, &pa);
		ASSERT_TRUE(g_called);
	}
	{
		pack_args_type_2f pa(1.25f, 2.5f);
		RESET(); r = api_caller_float_args_2f((const void*)mf_2f, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(ret_to_float(r) == 1.5f);
	}

	PASS();
	return 0;
}

static int test_callers_p_args(void)
{
	TEST("api_caller - p arg type (void/ptr/char/int/uint/float)");
	pack_args_type_p pa((const void*)0x1001);
	void *r;

	RESET(); api_caller_void_args_p((const void*)mv_p, &pa);
	ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);

	RESET(); r = api_caller_ptr_args_p((const void*)mv_p, &pa);
	ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);

	RESET(); r = api_caller_char_args_p((const void*)mc_p, &pa);
	ASSERT_TRUE(g_called); ASSERT_TRUE((char)(intptr_t)r == 'Z');

	RESET(); r = api_caller_int_args_p((const void*)mi_p, &pa);
	ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);

	RESET(); r = api_caller_uint_args_p((const void*)mu_p, &pa);
	ASSERT_TRUE(g_called); ASSERT_TRUE((unsigned int)(uintptr_t)r == 99);

	RESET(); r = api_caller_float_args_p((const void*)mf_p, &pa);
	ASSERT_TRUE(g_called); ASSERT_TRUE(ret_to_float(r) == 1.5f);

	PASS();
	return 0;
}

static int test_callers_pi_pf_args(void)
{
	TEST("api_caller - pi/pf/puc/pfp/p2i/p3i/p4i/p2f/p2fi arg types");
	void *r;

	{
		pack_args_type_pi pa((const void*)0x1001, 11);
		RESET(); api_caller_void_args_pi((const void*)mv_pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[1] == 11);
		RESET(); r = api_caller_ptr_args_pi((const void*)mv_pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
		RESET(); r = api_caller_int_args_pi((const void*)mi_pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_pf pa((const void*)0x1001, 1.25f);
		RESET(); api_caller_void_args_pf((const void*)mv_pf, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
	}
	{
		pack_args_type_puc pa((const void*)0x1001, 0xAA);
		RESET(); api_caller_void_args_puc((const void*)mv_puc, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[1] == 0xAA);
	}
	{
		pack_args_type_pfp pa((const void*)0x1001, 1.25f, (const void*)0x2002);
		RESET(); api_caller_void_args_pfp((const void*)mv_pfp, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[2] == 0x2002);
	}
	{
		pack_args_type_p2i pa((const void*)0x1001, 11, 22);
		RESET(); api_caller_void_args_p2i((const void*)mv_p2i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[2] == 22);
	}
	{
		pack_args_type_p3i pa((const void*)0x1001, 11, 22, 33);
		RESET(); api_caller_void_args_p3i((const void*)mv_p3i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[3] == 33);
	}
	{
		pack_args_type_p4i pa((const void*)0x1001, 11, 22, 33, 44);
		RESET(); api_caller_void_args_p4i((const void*)mv_p4i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[4] == 44);
	}
	{
		pack_args_type_p2f pa((const void*)0x1001, 1.25f, 2.5f);
		RESET(); api_caller_void_args_p2f((const void*)mv_p2f, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
	}
	{
		pack_args_type_p2fi pa((const void*)0x1001, 1.25f, 2.5f, 11);
		RESET(); r = api_caller_int_args_p2fi((const void*)mi_p2fi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
		ASSERT_TRUE(g_a[3] == 11); ASSERT_TRUE((int)(intptr_t)r == 42);
	}

	PASS();
	return 0;
}

static int test_callers_ip_args(void)
{
	TEST("api_caller - ip/i2p/i3p arg types");
	void *r;

	{
		pack_args_type_ip pa(11, (const void*)0x1001);
		RESET(); api_caller_void_args_ip((const void*)mv_ip, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[1] == 0x1001);
		RESET(); r = api_caller_ushort_args_ip((const void*)mus_ip, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((unsigned short)(uintptr_t)r == 777);
		RESET(); r = api_caller_int_args_ip((const void*)mi_ip, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_i2p pa(11, (const void*)0x1001, (const void*)0x2002);
		RESET(); api_caller_void_args_i2p((const void*)mv_i2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[2] == 0x2002);
		RESET(); r = api_caller_int_args_i2p((const void*)mi_i2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_i3p pa(11, (const void*)0x1001, (const void*)0x2002, (const void*)0x3003);
		RESET(); api_caller_void_args_i3p((const void*)mv_i3p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[3] == 0x3003);
	}

	PASS();
	return 0;
}

static int test_callers_2p_args(void)
{
	TEST("api_caller - 2p and 2p-extended arg types");
	void *r;

	{
		pack_args_type_2p pa((const void*)0x1001, (const void*)0x2002);
		RESET(); api_caller_void_args_2p((const void*)mv_2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[1] == 0x2002);
		RESET(); r = api_caller_ptr_args_2p((const void*)mv_2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
		RESET(); r = api_caller_int_args_2p((const void*)mi_2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_2p2f pa((const void*)0x1001, (const void*)0x2002, 1.25f, 2.5f);
		RESET(); api_caller_void_args_2p2f((const void*)mv_2p2f, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
	}
	{
		pack_args_type_2pf pa((const void*)0x1001, (const void*)0x2002, 1.25f);
		RESET(); r = api_caller_ptr_args_2pf((const void*)mv_2pf, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
	}
	{
		pack_args_type_2pfi pa((const void*)0x1001, (const void*)0x2002, 1.25f, 11);
		RESET(); api_caller_void_args_2pfi((const void*)mv_2pfi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[3] == 11);
	}
	{
		pack_args_type_2pi pa((const void*)0x1001, (const void*)0x2002, 11);
		RESET(); api_caller_void_args_2pi((const void*)mv_2pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[2] == 11);
		RESET(); r = api_caller_int_args_2pi((const void*)mi_2pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_2pui pa((const void*)0x1001, (const void*)0x2002, 99999u);
		RESET(); api_caller_void_args_2pui((const void*)mv_2pui, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[2] == 99999);
	}
	{
		pack_args_type_2p2i2p pa((const void*)0x1001, (const void*)0x2002, 11, 22, (const void*)0x3003, (const void*)0x4004);
		RESET(); api_caller_void_args_2p2i2p((const void*)mv_2p2i2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[5] == 0x4004);
	}
	{
		pack_args_type_2p3fus2uc pa((const void*)0x1001, (const void*)0x2002, 1.25f, 2.5f, 3.75f, 7777, 0xAA, 0xBB);
		RESET(); api_caller_void_args_2p3fus2uc((const void*)mv_2p3fus2uc, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
		ASSERT_TRUE(g_a[5] == 7777); ASSERT_TRUE(g_a[7] == 0xBB);
	}
	{
		pack_args_type_2pi2p pa((const void*)0x1001, (const void*)0x2002, 11, (const void*)0x3003, (const void*)0x4004);
		RESET(); api_caller_void_args_2pi2p((const void*)mv_2pi2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[4] == 0x4004);
	}
	{
		pack_args_type_2pif2p pa((const void*)0x1001, (const void*)0x2002, 11, 1.25f, (const void*)0x3003, (const void*)0x4004);
		RESET(); api_caller_void_args_2pif2p((const void*)mv_2pif2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[5] == 0x4004);
	}

	PASS();
	return 0;
}

static int test_callers_multi_p_args(void)
{
	TEST("api_caller - 3p/3p-ext/4p/4pi arg types");
	void *r;

	{
		pack_args_type_3p pa((const void*)0x1001, (const void*)0x2002, (const void*)0x3003);
		RESET(); api_caller_void_args_3p((const void*)mv_3p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[2] == 0x3003);
		RESET(); r = api_caller_ptr_args_3p((const void*)mv_3p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
		RESET(); r = api_caller_int_args_3p((const void*)mi_3p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_3p2f2i pa((const void*)0x1001, (const void*)0x2002, (const void*)0x3003, 1.25f, 2.5f, 11, 22);
		RESET(); api_caller_void_args_3p2f2i((const void*)mv_3p2f2i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[6] == 22);
	}
	{
		pack_args_type_3pi2p pa((const void*)0x1001, (const void*)0x2002, (const void*)0x3003, 11, (const void*)0x4004, (const void*)0x5005);
		RESET(); r = api_caller_int_args_3pi2p((const void*)mi_3pi2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
		ASSERT_TRUE(g_a[5] == 0x5005); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_4p pa((const void*)0x1001, (const void*)0x2002, (const void*)0x3003, (const void*)0x4004);
		RESET(); api_caller_void_args_4p((const void*)mv_4p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[3] == 0x4004);
		RESET(); r = api_caller_int_args_4p((const void*)mi_4p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}
	{
		pack_args_type_4pi pa((const void*)0x1001, (const void*)0x2002, (const void*)0x3003, (const void*)0x4004, 11);
		RESET(); api_caller_void_args_4pi((const void*)mv_4pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[4] == 11);
		RESET(); r = api_caller_int_args_4pi((const void*)mi_4pi, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE((int)(intptr_t)r == 42);
	}

	PASS();
	return 0;
}

static int test_callers_pip_pi2p_args(void)
{
	TEST("api_caller - pip/pip-ext/pi2p/pi2p2ip arg types");
	void *r;

	{
		pack_args_type_pip pa((const void*)0x1001, 11, (const void*)0x2002);
		RESET(); api_caller_void_args_pip((const void*)mv_pip, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[2] == 0x2002);
		RESET(); r = api_caller_ptr_args_pip((const void*)mv_pip, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(r == (void*)0xBEEF);
	}
	{
		pack_args_type_pip2f2i pa((const void*)0x1001, 11, (const void*)0x2002, 1.25f, 2.5f, 22, 33);
		RESET(); api_caller_void_args_pip2f2i((const void*)mv_pip2f2i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[6] == 33);
	}
	{
		pack_args_type_pip2f4i2p pa((const void*)0x1001, 11, (const void*)0x2002, 1.25f, 2.5f, 22, 33, 44, 55, (const void*)0x3003, (const void*)0x4004);
		RESET(); api_caller_void_args_pip2f4i2p((const void*)mv_pip2f4i2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[10] == 0x4004);
	}
	{
		pack_args_type_pi2p pa((const void*)0x1001, 11, (const void*)0x2002, (const void*)0x3003);
		RESET(); api_caller_void_args_pi2p((const void*)mv_pi2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[3] == 0x3003);
	}
	{
		pack_args_type_pi2p2ip pa((const void*)0x1001, 11, (const void*)0x2002, (const void*)0x3003, 22, 33, (const void*)0x4004);
		RESET(); r = api_caller_int_args_pi2p2ip((const void*)mi_pi2p2ip, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001);
		ASSERT_TRUE(g_a[6] == 0x4004); ASSERT_TRUE((int)(intptr_t)r == 42);
	}

	PASS();
	return 0;
}

static int test_callers_2i_ext_args(void)
{
	TEST("api_caller - 2i2p/2i2pi2p arg types");

	{
		pack_args_type_2i2p pa(11, 22, (const void*)0x1001, (const void*)0x2002);
		RESET(); api_caller_void_args_2i2p((const void*)mv_2i2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[3] == 0x2002);
	}
	{
		pack_args_type_2i2pi2p pa(11, 22, (const void*)0x1001, (const void*)0x2002, 33, (const void*)0x3003, (const void*)0x4004);
		RESET(); api_caller_void_args_2i2pi2p((const void*)mv_2i2pi2p, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[6] == 0x4004);
	}

	PASS();
	return 0;
}

static int test_callers_varargs_complex(void)
{
	TEST("api_caller - varargs (ipV/2pV) and complex (ipusf2p2f4i)");

	{
		pack_args_type_ipV pa(11, (const void*)0x1001, (const void*)"test");
		RESET(); api_caller_void_args_ipV((const void*)mv_ipV, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[1] == 0x1001);
	}
	{
		pack_args_type_2pV pa((const void*)0x1001, (const void*)0x2002, (const void*)"test");
		RESET(); api_caller_void_args_2pV((const void*)mv_2pV, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 0x1001); ASSERT_TRUE(g_a[1] == 0x2002);
	}
	{
		pack_args_type_ipusf2p2f4i pa(11, (const void*)0x1001, 7777, 1.25f, (const void*)0x2002, (const void*)0x3003, 2.5f, 3.75f, 22, 33, 44, 55);
		RESET(); api_caller_void_args_ipusf2p2f4i((const void*)mv_ipusf2p2f4i, &pa);
		ASSERT_TRUE(g_called); ASSERT_TRUE(g_a[0] == 11); ASSERT_TRUE(g_a[11] == 55);
	}

	PASS();
	return 0;
}

static int test_api_info_tables(void)
{
	TEST("api_info tables - all named entries have non-NULL api_caller");
	const api_info_t *info;
	unsigned int i, count;

	info = (const api_info_t *)&dllapi_info;
	count = sizeof(dllapi_info) / sizeof(api_info_t);
	for (i = 0; i < count; i++) {
		if (info[i].name != NULL)
			ASSERT_TRUE(info[i].api_caller != NULL);
	}

	info = (const api_info_t *)&newapi_info;
	count = sizeof(newapi_info) / sizeof(api_info_t);
	for (i = 0; i < count; i++) {
		if (info[i].name != NULL)
			ASSERT_TRUE(info[i].api_caller != NULL);
	}

	info = (const api_info_t *)&engine_info;
	count = sizeof(engine_info) / sizeof(api_info_t);
	for (i = 0; i < count; i++) {
		if (info[i].name != NULL)
			ASSERT_TRUE(info[i].api_caller != NULL);
	}

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

	printf("test_api_info:\n");
	fail |= test_callers_void_args();
	fail |= test_callers_int_float_args();
	fail |= test_callers_p_args();
	fail |= test_callers_pi_pf_args();
	fail |= test_callers_ip_args();
	fail |= test_callers_2p_args();
	fail |= test_callers_multi_p_args();
	fail |= test_callers_pip_pi2p_args();
	fail |= test_callers_2i_ext_args();
	fail |= test_callers_varargs_complex();
	fail |= test_api_info_tables();

	printf("\n%d/%d tests passed\n", tests_passed, tests_run);
	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
