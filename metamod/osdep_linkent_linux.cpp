/*
 * Copyright (c) 2004-2006 Jussi Kivilinna
 *
 *    This file is part of "Metamod All-Mod-Support"-patch for Metamod.
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

#include <extdll.h>		// always

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <link.h>

#include "osdep.h"
#include "osdep_p.h"
#include "log_meta.h"			// META_LOG, etc
#include "support_meta.h"

//
// Linux code for dynamic linkents
//  -- by Jussi Kivilinna
//

// 0xff,0x25 is opcode for our function forwarder
#define JMP_SIZE 2

//pointer size on x86-32: 4 bytes
//pointer size on x86-64: 8 bytes
#define PTR_SIZE sizeof(void*)

#ifdef __x86_64__
	//checks if pointer x points to jump forwarder
	#define is_code_jmp_opcode(x) (((unsigned char *)x)[0] == 0xff || ((unsigned char *)x)[1] == 0x25)
	
	/*AMD64-linux runs in "Small position independent code model" for processes that use dynamic linking*/
	
	//extracts pointer from "jmp dword ptr[pointer]"
	inline void * DLLINTERNAL extract_function_pointer_from_jmp(void * x) {
		// "Small position independent code model"
		// see: http://www.x86-64.org/documentation/abi.pdf
		unsigned char * bytes = (unsigned char *)x;
		long end_of_instruction = (long)x + JMP_SIZE + 4;
		
		// Jump over opcode
		bytes += JMP_SIZE;
		
		// read function address
		long func = *(long *)(end_of_instruction + (long)*(int *)bytes);
		return((void *)func);
	}
	
	//constructs new jmp forwarder
	#define construct_jmp_instruction(x, target) { \
		((unsigned char *)x)[0] = 0xff; \
		((unsigned char *)x)[1] = 0x25; \
		((unsigned char *)x)[2] = 0x0; \
		((unsigned char *)x)[3] = 0x0; \
		((unsigned char *)x)[4] = 0x0; \
		((unsigned char *)x)[5] = 0x0; \
		void *__target = (void *)(target); \
		memcpy(((char *)x + JMP_SIZE + 4), &__target, sizeof(__target)); \
	}
	
	//opcode + sizeof 32bit offset + sizeof pointer
	#define BYTES_SIZE (JMP_SIZE + 4 + PTR_SIZE)
#else /*i386*/
	//checks if pointer x points to jump forwarder
	#define is_code_jmp_opcode(x) (((unsigned char *)x)[0] == 0xff || ((unsigned char *)x)[1] == 0x25)
	
	//extracts pointer from "jmp dword ptr[pointer]"
	#define extract_function_pointer_from_jmp(x) (**(void***)((char *)(x) + JMP_SIZE))
	
	//constructs new jmp forwarder
	#define construct_jmp_instruction(x, target) { \
		((unsigned char *)(x))[0] = 0xff; \
		((unsigned char *)(x))[1] = 0x25; \
		static void * __target = (void*)(target); \
		void ** ___target = &__target; \
		memcpy(((char *)(x) + 2), &___target, sizeof(___target)); \
	}
	
	//opcode + sizeof pointer
	#define BYTES_SIZE (JMP_SIZE + PTR_SIZE)
#endif

typedef void * (*dlsym_func)(void * module, const char * funcname);

static void * gamedll_module_handle = 0;
static void * metamod_module_handle = 0;

//pointer to original dlsym
static dlsym_func dlsym_original;

//contains jmp to replacement_dlsym @dlsym_original
static unsigned char dlsym_new_bytes[BYTES_SIZE];

//contains original bytes of dlsym
static unsigned char dlsym_old_bytes[BYTES_SIZE];

//Mutex for our protection
static pthread_mutex_t mutex_replacement_dlsym = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//
static struct link_map * DLLINTERNAL_NOVIS get_link_map(void) {
	struct link_map * l_map;
	
	l_map = _r_debug.r_map;
	while(l_map && l_map->l_prev) {
		l_map = l_map->l_prev;
	}
	
	return(l_map);
}

//
static int DLLINTERNAL_NOVIS get_tables(struct link_map * l_map, ElfW(Sym) *& symtab, char *& strtab, int & nchains) {
	ElfW(Dyn) * dyn;
	
	dyn = l_map->l_ld;
	
	strtab = 0;
	symtab = 0;
	nchains = 0;
	
	for(int i = 0; dyn[i].d_tag != DT_NULL; i++) {
		if(dyn[i].d_tag == DT_HASH) {
			Elf32_Word * hash_table;
			
			// some linux versions seem to have this already relocated while other don't(?)
			if(dyn[i].d_un.d_ptr < l_map->l_addr)
				hash_table = (Elf32_Word*)(l_map->l_addr + dyn[i].d_un.d_ptr);
			else
				hash_table = (Elf32_Word*)(dyn[i].d_un.d_ptr);
			
			//nbuckets = hash_table[0];
			nchains = hash_table[1];
			//buckets = &hash_table[2];
			//chains = &buckets[nbuckets];
		} 
		else if(dyn[i].d_tag == DT_STRTAB) {
			strtab = (char*)dyn[i].d_un.d_ptr;
		}
		else if(dyn[i].d_tag == DT_SYMTAB) {
			symtab = (ElfW(Sym)*)dyn[i].d_un.d_ptr;
		}
		
		if(nchains && strtab && symtab)
			break;
	}
	
	return(nchains && strtab && symtab);
}

//
static void * DLLINTERNAL_NOVIS find_symbol(struct link_map * l_map, const char * name, int stt_type, int stb_type, ElfW(Sym) * symtab, char * strtab, int nchains) {
	char * str;
	size_t name_len;
	
	name_len = strlen(name);
	
	for(int i = 0; i < nchains; i++) {
#ifdef __x86_64__
		if(ELF64_ST_TYPE(symtab[i].st_info) != stt_type || ELF64_ST_BIND(symtab[i].st_info) != stb_type)
			continue;
#else
		if(ELF32_ST_TYPE(symtab[i].st_info) != stt_type || ELF32_ST_BIND(symtab[i].st_info) != stb_type)
			continue;
#endif		
		str = strtab + symtab[i].st_name;
		
		if(mm_strncmp(str, name, name_len) == 0) {
			if(str[name_len]==0) {
				return((void*)(l_map->l_addr + symtab[i].st_value));
			}
		}
	}
	
	return(0);
}

// 
static void * DLLINTERNAL_NOVIS get_real_func_ptr(const char * lib, const char * name, int * errorcode) {
	struct link_map * l_map;
	ElfW(Sym) * symtab;
	char * strtab;
	int nchains;
	void * sym_ptr;
	
	if(errorcode)
		*errorcode = 0;
	
	l_map = get_link_map();
	if(!l_map) {
		if(errorcode)
			*errorcode = 1;
		return(0);
	}
	
	do {
		// skip all except 'lib'
		if(!strstr(l_map->l_name, lib))
			continue;
		
		if(!get_tables(l_map, symtab, strtab, nchains)) {
			continue;
		}
		
		sym_ptr = find_symbol(l_map, name, STT_FUNC, STB_GLOBAL, symtab, strtab, nchains);
		if(sym_ptr && *(unsigned short*)sym_ptr != 0x25ff) {
			return(sym_ptr);
		}
	} while((l_map = l_map->l_next));
	
	if(errorcode)
		*errorcode = 2;
	
	return(0);
}

//
//restores old dlsym
//
inline void DLLINTERNAL restore_original_dlsym(void)
{
	//Copy old dlsym bytes back
	memcpy((void*)dlsym_original, dlsym_old_bytes, BYTES_SIZE);
}

//
//resets new dlsym
//
inline void DLLINTERNAL reset_dlsym_hook(void)
{
	//Copy new dlsym bytes back
	memcpy((void*)dlsym_original, dlsym_new_bytes, BYTES_SIZE);
}

//
// Replacement dlsym function
//
static void * __replacement_dlsym(void * module, const char * funcname)
{
	//these are needed in case dlsym calls dlsym, default one doesn't do
	//it but some LD_PRELOADed library that hooks dlsym might actually
	//do so.
	static int is_original_restored = 0;
	int was_original_restored = is_original_restored;
	
	//Lock before modifing original dlsym
	pthread_mutex_lock(&mutex_replacement_dlsym);
	
	//restore old dlsym
	if(!is_original_restored)
	{
		restore_original_dlsym();
		
		is_original_restored = 1;
	}
		
	//check if we should hook this call
	if(module != metamod_module_handle || !metamod_module_handle || !gamedll_module_handle)
	{
		//no metamod/gamedll module? should we remove hook now?
		void * retval = dlsym_original(module, funcname);
		
		if(metamod_module_handle && gamedll_module_handle)
		{
			if(!was_original_restored)
			{
				//reset dlsym hook
				reset_dlsym_hook();
				
				is_original_restored = 0;
			}
		}
		else
		{
			//no metamod/gamedll module? should we remove hook now by not reseting it back?
		}
		
		//unlock
		pthread_mutex_unlock(&mutex_replacement_dlsym);
		
		return(retval);
	}
	
	//dlsym on metamod module
	void * func = dlsym_original(module, funcname);
	
	if(!func)
	{
		//function not in metamod module, try gamedll
		func = dlsym_original(gamedll_module_handle, funcname);
	}
	
	if(!was_original_restored)
	{
		//reset dlsym hook
		reset_dlsym_hook();
		
		is_original_restored = 0;
	}
	
	//unlock
	pthread_mutex_unlock(&mutex_replacement_dlsym);
	
	return(func);
}

//
// Initialize
//
int DLLINTERNAL init_linkent_replacement(DLHANDLE MetamodHandle, DLHANDLE GameDllHandle)
{
	int errorcode = 0;
	
	metamod_module_handle = MetamodHandle;
	gamedll_module_handle = GameDllHandle;
	
	dlsym_original = (dlsym_func)get_real_func_ptr("/libdl.so", "dlsym", &errorcode);
	if(!dlsym_original) {
		//whine loud and exit
		META_ERROR("Couldn't initialize dynamic linkents, get_real_func_ptr(libdl.so, dlsym) failed with errorcode: %d", errorcode);
		return(0);
	}
	
	//Backup old bytes of "dlsym" function
	memcpy(dlsym_old_bytes, (void*)dlsym_original, BYTES_SIZE);
	
	//Construct new bytes: "jmp dword ptr[replacement_dlsym]"
	construct_jmp_instruction((void*)&dlsym_new_bytes[0], (void*)&__replacement_dlsym);
	
	//Check if bytes overlap page border.
	const int pagesize = getpagesize();
	long align = (long)dlsym_original % pagesize;
	long start_of_page = (long)dlsym_original - align;
	long number_of_pages = ((align + BYTES_SIZE) + (pagesize - 1)) / pagesize;
	
	//Remove PROT_READ restriction
	if(mprotect((void*)start_of_page, number_of_pages * pagesize, PROT_READ|PROT_WRITE|PROT_EXEC))
	{
		META_ERROR("Couldn't initialize dynamic linkents, mprotect failed: %i.  Exiting...", errno);
		return(0);
	}
	
	//Write our own jmp-forwarder on "dlsym"
	reset_dlsym_hook();
	
	//done
	return(1);
}
