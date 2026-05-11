//
// Win32 API function stubs for testing osdep_*_win32.cpp on Linux
//
// These stubs implement Win32 PE file mapping APIs using POSIX equivalents
// (fopen/mmap/malloc), allowing the Win32 code paths to be exercised in
// unit tests compiled on Linux.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "win32_stubs.h"

// ============================================================
// Stub state
// ============================================================

static DWORD stub_last_error = 0;
static bool stub_force_createfile_fail = false;
static bool stub_force_createfilemapping_fail = false;
static bool stub_force_mapviewoffile_fail = false;
static bool stub_force_virtualprotect_fail = false;
static bool stub_force_isbadreadptr = false;
static int stub_isbadreadptr_fail_after = -1;
static int stub_isbadreadptr_call_count = 0;
static int stub_isbadstringptr_fail_after = -1;
static int stub_isbadstringptr_call_count = 0;

void win32_stub_reset(void)
{
	stub_last_error = 0;
	stub_force_createfile_fail = false;
	stub_force_createfilemapping_fail = false;
	stub_force_mapviewoffile_fail = false;
	stub_force_virtualprotect_fail = false;
	stub_force_isbadreadptr = false;
	stub_isbadreadptr_fail_after = -1;
	stub_isbadreadptr_call_count = 0;
	stub_isbadstringptr_fail_after = -1;
	stub_isbadstringptr_call_count = 0;
}

void win32_stub_set_createfile_fail(bool fail) { stub_force_createfile_fail = fail; }
void win32_stub_set_createfilemapping_fail(bool fail) { stub_force_createfilemapping_fail = fail; }
void win32_stub_set_mapviewoffile_fail(bool fail) { stub_force_mapviewoffile_fail = fail; }
void win32_stub_set_virtualprotect_fail(bool fail) { stub_force_virtualprotect_fail = fail; }
void win32_stub_set_isbadreadptr(bool bad) { stub_force_isbadreadptr = bad; }
void win32_stub_set_isbadreadptr_fail_after(int n) { stub_isbadreadptr_fail_after = n; stub_isbadreadptr_call_count = 0; }
void win32_stub_set_isbadstringptr_fail_after(int n) { stub_isbadstringptr_fail_after = n; stub_isbadstringptr_call_count = 0; }

// ============================================================
// File handle tracking (maps HANDLE → fd/mmap info)
// ============================================================

struct handle_entry {
	int fd;
	void *map_addr;
	size_t map_size;
	int in_use;
};

#define MAX_HANDLES 32
static struct handle_entry handles[MAX_HANDLES];

static HANDLE alloc_handle(void)
{
	for (int i = 0; i < MAX_HANDLES; i++) {
		if (!handles[i].in_use) {
			memset(&handles[i], 0, sizeof(handles[i]));
			handles[i].in_use = 1;
			handles[i].fd = -1;
			return (HANDLE)(long)(i + 1);
		}
	}
	return INVALID_HANDLE_VALUE;
}

static struct handle_entry *get_handle(HANDLE h)
{
	int idx = (int)(long)h - 1;
	if (idx < 0 || idx >= MAX_HANDLES || !handles[idx].in_use)
		return NULL;
	return &handles[idx];
}

static void free_handle(HANDLE h)
{
	struct handle_entry *e = get_handle(h);
	if (e) e->in_use = 0;
}

// ============================================================
// Win32 API implementations
// ============================================================

BOOL IsBadReadPtr(const void *lp, unsigned int ucb)
{
	if (stub_force_isbadreadptr)
		return TRUE;
	if (stub_isbadreadptr_fail_after >= 0 &&
	    stub_isbadreadptr_call_count++ >= stub_isbadreadptr_fail_after)
		return TRUE;
	return (lp == NULL) ? TRUE : FALSE;
}

BOOL IsBadStringPtrA(const char *lpsz, unsigned int ucchMax)
{
	if (stub_force_isbadreadptr)
		return TRUE;
	if (stub_isbadstringptr_fail_after >= 0 &&
	    stub_isbadstringptr_call_count++ >= stub_isbadstringptr_fail_after)
		return TRUE;
	return (lpsz == NULL) ? TRUE : FALSE;
}

HANDLE CreateFileA(const char *lpFileName, DWORD dwDesiredAccess,
                   DWORD dwShareMode, void *lpSecurityAttributes,
                   DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                   HANDLE hTemplateFile)
{
	(void)dwDesiredAccess; (void)dwShareMode; (void)lpSecurityAttributes;
	(void)dwCreationDisposition; (void)dwFlagsAndAttributes; (void)hTemplateFile;

	if (stub_force_createfile_fail) {
		stub_last_error = 2; // ERROR_FILE_NOT_FOUND
		return INVALID_HANDLE_VALUE;
	}

	int fd = open(lpFileName, O_RDONLY);
	if (fd < 0) {
		stub_last_error = 2;
		return INVALID_HANDLE_VALUE;
	}

	HANDLE h = alloc_handle();
	if (h == INVALID_HANDLE_VALUE) {
		close(fd);
		return INVALID_HANDLE_VALUE;
	}

	struct handle_entry *e = get_handle(h);
	e->fd = fd;
	return h;
}

HANDLE CreateFileMapping(HANDLE hFile, void *lpAttributes, DWORD flProtect,
                         DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow,
                         const char *lpName)
{
	(void)lpAttributes; (void)flProtect; (void)dwMaximumSizeHigh;
	(void)dwMaximumSizeLow; (void)lpName;

	if (stub_force_createfilemapping_fail) {
		stub_last_error = 8; // ERROR_NOT_ENOUGH_MEMORY
		return (HANDLE)0;
	}

	struct handle_entry *fe = get_handle(hFile);
	if (!fe || fe->fd < 0)
		return (HANDLE)0;

	struct stat st;
	if (fstat(fe->fd, &st) < 0)
		return (HANDLE)0;

	HANDLE h = alloc_handle();
	if (h == INVALID_HANDLE_VALUE)
		return (HANDLE)0;

	struct handle_entry *e = get_handle(h);
	e->fd = fe->fd;
	e->map_size = st.st_size;
	return h;
}

void *MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess,
                    DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow,
                    unsigned long dwNumberOfBytesToMap)
{
	(void)dwDesiredAccess; (void)dwFileOffsetHigh; (void)dwFileOffsetLow;
	(void)dwNumberOfBytesToMap;

	if (stub_force_mapviewoffile_fail)
		return NULL;

	struct handle_entry *e = get_handle(hFileMappingObject);
	if (!e || e->fd < 0)
		return NULL;

	void *addr = mmap(NULL, e->map_size, PROT_READ | PROT_WRITE,
	                  MAP_PRIVATE, e->fd, 0);
	if (addr == MAP_FAILED)
		return NULL;

	e->map_addr = addr;
	return addr;
}

BOOL UnmapViewOfFile(const void *lpBaseAddress)
{
	for (int i = 0; i < MAX_HANDLES; i++) {
		if (handles[i].in_use && handles[i].map_addr == lpBaseAddress) {
			munmap(handles[i].map_addr, handles[i].map_size);
			handles[i].map_addr = NULL;
			return TRUE;
		}
	}
	if (lpBaseAddress)
		munmap((void *)lpBaseAddress, 4096);
	return TRUE;
}

BOOL CloseHandle(HANDLE hObject)
{
	struct handle_entry *e = get_handle(hObject);
	if (!e) return FALSE;

	if (e->map_addr) {
		munmap(e->map_addr, e->map_size);
		e->map_addr = NULL;
	}
	if (e->fd >= 0) {
		// Don't close if another handle shares this fd
		int fd = e->fd;
		e->fd = -1;
		int shared = 0;
		for (int i = 0; i < MAX_HANDLES; i++)
			if (handles[i].in_use && handles[i].fd == fd)
				shared = 1;
		if (!shared)
			close(fd);
	}
	free_handle(hObject);
	return TRUE;
}

extern "C" void *__real_calloc(size_t, size_t) __attribute__((weak));
extern "C" void __real_free(void *) __attribute__((weak));

#define MAX_VALLOCS 64
static void *valloc_ptrs[MAX_VALLOCS];
static int valloc_count = 0;

void *VirtualAlloc(void *lpAddress, unsigned long dwSize, DWORD flAllocationType,
                   DWORD flProtect)
{
	(void)lpAddress; (void)flAllocationType; (void)flProtect;
	void *p = __real_calloc ? __real_calloc(1, dwSize) : calloc(1, dwSize);
	if (p && valloc_count < MAX_VALLOCS)
		valloc_ptrs[valloc_count++] = p;
	return p;
}

void win32_stub_free_virtualallocs(void)
{
	for (int i = 0; i < valloc_count; i++) {
		if (__real_free)
			__real_free(valloc_ptrs[i]);
		else
			free(valloc_ptrs[i]);
	}
	valloc_count = 0;
}

// ============================================================
// Optional calloc/free tracking via --wrap=calloc,--wrap=free
// ============================================================

#define MAX_TRACKED_CALLOCS 64
static void *tracked_calloc_ptrs[MAX_TRACKED_CALLOCS];
static int tracked_calloc_count = 0;
static bool tracking_callocs_enabled = false;

void win32_stub_set_tracking_callocs(bool enable)
{
	tracking_callocs_enabled = enable;
}

void win32_stub_free_tracked_callocs(void)
{
	for (int i = 0; i < tracked_calloc_count; i++) {
		if (__real_free)
			__real_free(tracked_calloc_ptrs[i]);
		else
			free(tracked_calloc_ptrs[i]);
	}
	tracked_calloc_count = 0;
}

extern "C" void *__wrap_calloc(size_t nmemb, size_t size)
{
	void *p = __real_calloc(nmemb, size);
	if (tracking_callocs_enabled && p && tracked_calloc_count < MAX_TRACKED_CALLOCS)
		tracked_calloc_ptrs[tracked_calloc_count++] = p;
	return p;
}

extern "C" void __wrap_free(void *ptr)
{
	for (int i = 0; i < tracked_calloc_count; i++) {
		if (tracked_calloc_ptrs[i] == ptr) {
			tracked_calloc_ptrs[i] = tracked_calloc_ptrs[--tracked_calloc_count];
			break;
		}
	}
	__real_free(ptr);
}

BOOL VirtualProtect(void *lpAddress, unsigned long dwSize, DWORD flNewProtect,
                    DWORD *lpflOldProtect)
{
	(void)lpAddress; (void)dwSize; (void)flNewProtect;
	if (stub_force_virtualprotect_fail) {
		stub_last_error = 87;
		return FALSE;
	}
	if (lpflOldProtect)
		*lpflOldProtect = PAGE_READWRITE;
	return TRUE;
}

DWORD GetLastError(void)
{
	return stub_last_error;
}

HINSTANCE LoadLibraryA(const char *lpLibFileName)
{
	(void)lpLibFileName;
	return NULL;
}

FARPROC GetProcAddress(HINSTANCE hModule, const char *lpProcName)
{
	(void)hModule; (void)lpProcName;
	return NULL;
}

BOOL FreeLibrary(HINSTANCE hLibModule)
{
	(void)hLibModule;
	return TRUE;
}
