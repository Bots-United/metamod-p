// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// osdep.h - operating system dependencies

/*
 * Copyright (c) 2001-2005 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifndef OSDEP_H
#define OSDEP_H

#include <string.h>			// strerror()
#include <ctype.h>			// isupper, tolower
#include <errno.h>			// errno

// Various differences between WIN32 and Linux.

#include "comp_dep.h"
#include "types_meta.h"		// mBOOL
#include "mreg.h"			// REG_CMD_FN, etc
#include "log_meta.h"		// LOG_ERROR, etc

// String describing platform/DLL-type, for matching lines in plugins.ini.
#ifdef linux
	#define PLATFORM	"linux"
#  if defined(__x86_64__) || defined(__amd64__)
	#define PLATFORM_SPC	"lin64"
#  else
	#define PLATFORM_SPC	"lin32"
#  endif
	#define PLATFORM_DLEXT	".so"
#elif defined(_WIN32)
	#define PLATFORM	"mswin"
	#define PLATFORM_SPC	"win32"
	#define PLATFORM_DLEXT	".dll"
#else /* unknown */
	#error "OS unrecognized"
#endif /* unknown */

// Macro for function-exporting from DLL..
// from SDK dlls/cbase.h:
//! C functions for external declarations that call the appropriate C++ methods

// Windows uses "__declspec(dllexport)" to mark functions in the DLL that
// should be visible/callable externally.
//
// It also apparently requires WINAPI for GiveFnptrsToDll().
//
// See doc/notes_windows_coding for more information..

// Attributes to specify an "exported" function, visible from outside the
// DLL.
#undef DLLEXPORT
#ifdef _WIN32
	#define DLLEXPORT	__declspec(dllexport)
	// WINAPI should be provided in the windows compiler headers.
	// It's usually defined to something like "__stdcall".
#elif defined(linux)
	#define DLLEXPORT	/* */
	#define WINAPI		/* */
#endif /* linux */

// Simplified macro for declaring/defining exported DLL functions.  They
// need to be 'extern "C"' so that the C++ compiler enforces parameter
// type-matching, rather than considering routines with mis-matched
// arguments/types to be overloaded functions...
//
// AFAIK, this is os-independent, but it's included here in osdep.h where
// DLLEXPORT is defined, for convenience.
#define C_DLLEXPORT		extern "C" DLLEXPORT

//int64bit
typedef long long int int64bit;

// Functions & types for DLL open/close/etc operations.
extern mBOOL dlclose_handle_invalid DLLHIDDEN;
#ifdef linux
	#include <dlfcn.h>
	typedef void* DLHANDLE;
	typedef void* DLFUNC;
	inline DLHANDLE DLLINTERNAL DLOPEN(const char *filename) {
		return(dlopen(filename, RTLD_NOW));
	}
	inline DLFUNC DLLINTERNAL DLSYM(DLHANDLE handle, const char *string) {
		return(dlsym(handle, string));
	}
	//dlclose crashes if handle is null.
	inline int DLLINTERNAL DLCLOSE(DLHANDLE handle) {
		if(unlikely(!handle)) {
			dlclose_handle_invalid = mTRUE;
			return(1);
		}
		
		dlclose_handle_invalid = mFALSE;
		return(dlclose(handle));
	}
	inline const char * DLLINTERNAL DLERROR(void) {
		if(unlikely(dlclose_handle_invalid))
			return("Invalid handle.");
		return(dlerror());
	}
#elif defined(_WIN32)
	typedef HINSTANCE DLHANDLE;
	typedef FARPROC DLFUNC;
	inline DLHANDLE DLLINTERNAL DLOPEN(const char *filename) {
		return(LoadLibrary(filename));
	}
	inline DLFUNC DLLINTERNAL DLSYM(DLHANDLE handle, const char *string) {
		return(GetProcAddress(handle, string));
	}
	inline int DLLINTERNAL DLCLOSE(DLHANDLE handle) {
		if(unlikely(!handle)) {
			dlclose_handle_invalid = mTRUE;
			return(1);
		}
		
		dlclose_handle_invalid = mFALSE;
		
		// NOTE: Windows FreeLibrary returns success=nonzero, fail=zero,
		// which is the opposite of the unix convention, thus the '!'.
		return(!FreeLibrary(handle));
	}
	// Windows doesn't provide a function corresponding to dlerror(), so
	// we make our own.
	char * DLLINTERNAL str_GetLastError(void);
	inline const char * DLLINTERNAL DLERROR(void) {
		if(unlikely(dlclose_handle_invalid))
			return("Invalid handle.");
		return(str_GetLastError());
	}
#endif /* _WIN32 */
const char * DLLINTERNAL DLFNAME(void *memptr);
mBOOL DLLINTERNAL IS_VALID_PTR(void *memptr);


// Attempt to call the given function pointer, without segfaulting.
mBOOL DLLINTERNAL os_safe_call(REG_CMD_FN pfn);


// Windows doesn't have an strtok_r() routine, so we write our own.
#ifdef _WIN32
	#define strtok_r(s, delim, ptrptr)	my_strtok_r(s, delim, ptrptr)
	char * DLLINTERNAL my_strtok_r(char *s, const char *delim, char **ptrptr);
#endif /* _WIN32 */


// Special version that fixes vsnprintf bugs.
int DLLINTERNAL safe_vsnprintf(char* s,  size_t n,  const char *format, va_list ap);
int DLLINTERNAL safe_snprintf(char* s, size_t n, const char* format, ...);


// Linux doesn't have an strlwr() routine, so we write our own.
#ifdef linux
	#define strlwr(s) my_strlwr(s)
	char * DLLINTERNAL my_strlwr(char *s);
#endif /* _WIN32 */


// Set filename and pathname maximum lengths.  Note some windows compilers
// provide a <limits.h> which is incomplete and/or causes problems; see
// doc/windows_notes.txt for more information.
//
// Note that both OS's include room for null-termination:
//   linux:    "# chars in a path name including nul"
//   win32:    "note that the sizes include space for 0-terminator"
#ifdef linux
	#include <limits.h>
#elif defined(_WIN32)
	#include <stdlib.h>
	#define NAME_MAX	_MAX_FNAME
	#define PATH_MAX	_MAX_PATH
#endif /* _WIN32 */

// Various other windows routine differences.
#ifdef linux
	#include <unistd.h>	// sleep
	#ifndef O_BINARY
    	#define O_BINARY 0
	#endif	
#elif defined(_WIN32)
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define sleep(x)	Sleep(x*1000)
	#define strcasecmp	stricmp
	#define strncasecmp	_strnicmp
    #include <io.h>
    #define open _open
    #define read _read
    #define write _write
    #define close _close
#endif /* _WIN32 */

#include <unistd.h>	// getcwd

#include <sys/stat.h>
#ifndef S_ISREG
	// Linux gcc defines this; earlier mingw didn't, later mingw does;
	// MSVC doesn't seem to.
	#define S_ISREG(m)	((m) & S_IFREG)
#endif /* not S_ISREG */
#ifdef _WIN32
	// The following two are defined in mingw but not in MSVC
    #ifndef S_IRUSR
        #define S_IRUSR _S_IREAD
    #endif
    #ifndef S_IWUSR
        #define S_IWUSR _S_IWRITE
    #endif
	
	// The following two are defined neither in mingw nor in MSVC
    #ifndef S_IRGRP
        #define S_IRGRP S_IRUSR
    #endif
    #ifndef S_IWGRP
        #define S_IWGRP S_IWUSR
    #endif
#endif /* _WIN32 */


#ifdef UNFINISHED

// Thread handling...
#ifdef linux
	#include <pthread.h>
	typedef	pthread_t 	THREAD_T;
	// returns 0==success, non-zero==failure
	inline int DLLINTERNAL THREAD_CREATE(THREAD_T *tid, void (*func)(void)) {
		int ret;
		ret=pthread_create(tid, NULL, (void *(*)(void*)) func, NULL);
		if(ret != 0) {
			META_WARNING("Failure starting thread: %s", strerror(ret));
			return(ret);
		}
		ret=pthread_detach(*tid);
		if(ret != 0)
			META_WARNING("Failure detaching thread: %s", strerror(ret));
		return(ret);
	}
#elif defined(_WIN32)
	// See:
	//    http://msdn.microsoft.com/library/en-us/dllproc/prothred_4084.asp
	typedef	DWORD 		THREAD_T;
	// returns 0==success, non-zero==failure
	inline int DLLINTERNAL THREAD_CREATE(THREAD_T *tid, void (*func)(void)) {
		HANDLE ret;
		// win32 returns NULL==failure, non-NULL==success
		ret=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) func, NULL, 0, tid);
		if(ret==NULL)
			META_WARNING("Failure starting thread: %s", str_GetLastError());
		return(ret==NULL);
	}
#endif /* _WIN32 */
#define THREAD_OK	0


// Mutex handling...
#ifdef linux
	typedef pthread_mutex_t		MUTEX_T;
	inline int DLLINTERNAL MUTEX_INIT(MUTEX_T *mutex) {
		int ret;
		ret=pthread_mutex_init(mutex, NULL);
		if(ret!=THREAD_OK)
			META_WARNING("mutex_init failed: %s", strerror(ret));
		return(ret);
	}
	inline int DLLINTERNAL MUTEX_LOCK(MUTEX_T *mutex) {
		int ret;
		ret=pthread_mutex_lock(mutex);
		if(ret!=THREAD_OK)
			META_WARNING("mutex_lock failed: %s", strerror(ret));
		return(ret);
	}
	inline int DLLINTERNAL MUTEX_UNLOCK(MUTEX_T *mutex) {
		int ret;
		ret=pthread_mutex_unlock(mutex);
		if(ret!=THREAD_OK)
			META_WARNING("mutex_unlock failed: %s", strerror(ret));
		return(ret);
	}
#elif defined(_WIN32)
	// Win32 has "mutexes" as well, but CS's are simpler.
	// See:
	//    http://msdn.microsoft.com/library/en-us/dllproc/synchro_2a2b.asp
	typedef CRITICAL_SECTION	MUTEX_T;
	// Note win32 routines don't return any error (return void).
	inline int DLLINTERNAL MUTEX_INIT(MUTEX_T *mutex) {
		InitializeCriticalSection(mutex);
		return(THREAD_OK);
	}
	inline int DLLINTERNAL MUTEX_LOCK(MUTEX_T *mutex) {
		EnterCriticalSection(mutex);
		return(THREAD_OK);
	}
	inline int DLLINTERNAL MUTEX_UNLOCK(MUTEX_T *mutex) {
		LeaveCriticalSection(mutex);
		return(THREAD_OK);
	}
#endif /* _WIN32 (mutex) */


// Condition variables...
#ifdef linux
	typedef pthread_cond_t	COND_T;
	inline int DLLINTERNAL COND_INIT(COND_T *cond) {
		int ret;
		ret=pthread_cond_init(cond, NULL);
		if(ret!=THREAD_OK)
			META_WARNING("cond_init failed: %s", strerror(ret));
		return(ret);
	}
	inline int DLLINTERNAL COND_WAIT(COND_T *cond, MUTEX_T *mutex) {
		int ret;
		ret=pthread_cond_wait(cond, mutex);
		if(ret!=THREAD_OK)
			META_WARNING("cond_wait failed: %s", strerror(ret));
		return(ret);
	}
	inline int DLLINTERNAL COND_SIGNAL(COND_T *cond) {
		int ret;
		ret=pthread_cond_signal(cond);
		if(ret!=THREAD_OK)
			META_WARNING("cond_signal failed: %s", strerror(ret));
		return(ret);
	}
#elif defined(_WIN32)
	// Since win32 doesn't provide condition-variables, we have to model
	// them with mutex/critical-sections and win32 events.  This uses the
	// second (SetEvent) solution from:
	//
	//    http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	//
	// but without the waiters_count overhead, since we don't need
	// broadcast functionality anyway.  Or actually, I guess it's more like
	// the first (PulseEvent) solution, but with SetEven rather than
	// PulseEvent. :)
	//
	// See also:
	//    http://msdn.microsoft.com/library/en-us/dllproc/synchro_8ann.asp
	typedef HANDLE COND_T; 
	inline int DLLINTERNAL COND_INIT(COND_T *cond) {
		*cond = CreateEvent(NULL,	// security attributes (none)
							FALSE,	// manual-reset type (false==auto-reset)
							FALSE,	// initial state (unsignaled)
							NULL);	// object name (unnamed)
		// returns NULL on error
		if(unlikely(*cond==NULL)) {
			META_WARNING("cond_init failed: %s", str_GetLastError());
			return(-1);
		}
		else
			return(0);
	}
	inline int DLLINTERNAL COND_WAIT(COND_T *cond, MUTEX_T *mutex) {
		DWORD ret;
		LeaveCriticalSection(mutex);
		ret=WaitForSingleObject(*cond, INFINITE);
		EnterCriticalSection(mutex);
		// returns WAIT_OBJECT_0 if object was signaled; other return
		// values indicate errors.
		if(ret == WAIT_OBJECT_0)
			return(0);
		else {
			META_WARNING("cond_wait failed: %s", str_GetLastError());
			return(-1);
		}
	}
	inline int DLLINTERNAL COND_SIGNAL(COND_T *cond) {
		BOOL ret;
		ret=SetEvent(*cond);
		// returns zero on failure
		if(ret==0) {
			META_WARNING("cond_signal failed: %s", str_GetLastError());
			return(-1);
		}
		else
			return(0);
	}
#endif /* _WIN32 (condition variable) */

#endif /*NFINISHED*/


// Normalize/standardize a pathname.
//  - For win32, this involves:
//    - Turning backslashes (\) into slashes (/), so that config files and
//      Metamod internal code can be simpler and just use slashes (/).
//    - Turning upper/mixed case into lowercase, since windows is
//      non-case-sensitive.
//  - For linux, this requires no work, as paths uses slashes (/) natively,
//    and pathnames are case-sensitive.
#ifdef linux
#define normalize_pathname(a)
#elif defined(_WIN32)
void DLLINTERNAL normalize_pathname(char *path);
#endif /* _WIN32 */

// Indicate if pathname appears to be an absolute-path.  Under linux this
// is a leading slash (/).  Under win32, this can be:
//  - a drive-letter path (ie "D:blah" or "C:\blah")
//  - a toplevel path (ie "\blah")
//  - a UNC network address (ie "\\srv1\blah").
// Also, handle both native and normalized pathnames.
inline mBOOL DLLINTERNAL is_absolute_path(const char *path) {
	if(likely(path[0]=='/')) return(mTRUE);
#ifdef _WIN32
	if(likely(path[1]==':')) return(mTRUE);
	if(unlikely(path[0]=='\\')) return(mTRUE);
#endif /* _WIN32 */
	return(mFALSE);
}

#ifdef _WIN32
// Buffer pointed to by resolved_name is assumed to be able to store a
// string of PATH_MAX length.
char * DLLINTERNAL realpath(const char *file_name, char *resolved_name);
#endif /* _WIN32 */

// Generic "error string" from a recent OS call.  For linux, this is based
// on errno.  For win32, it's based on GetLastError.
inline const char * DLLINTERNAL str_os_error(void) {
#ifdef linux
	return(strerror(errno));
#elif defined(_WIN32)
	return(str_GetLastError());
#endif /* _WIN32 */
}


#endif /* OSDEP_H */
