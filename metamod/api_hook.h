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
#ifndef API_HOOK_H
#define API_HOOK_H

#include "ret_type.h"
#include "api_info.h"
#include "meta_api.h"
#include "osdep.h"		//OPEN_ARGS

// Compine 4 parts for single name
#define _COMBINE4(w,x,y,z) w##x##y##z
#define _COMBINE2(x,y) x##y

// simplified 'void' version of main hook function
void DLLINTERNAL main_hook_function_void(unsigned long api_info_offset, enum_api_t api, unsigned long func_offset, const void * packed_args);

// full return typed version of main hook function
void * DLLINTERNAL main_hook_function(const class_ret_t ret_init, unsigned long api_info_offset, enum_api_t api, unsigned long func_offset, const void * packed_args);

//
// API function args structures
//
#define API_PACK_ARGS(type, args) \
	_COMBINE2(pack_args_type_, type) packed_args = { OPEN_ARGS args }

typedef struct {                                } pack_args_type_void;
typedef struct { int i1; 			} pack_args_type_i;
typedef struct { int i1,i2; 			} pack_args_type_2i;
typedef struct { int i1,i2,i3; 			} pack_args_type_3i;
typedef struct { unsigned int ui1;		} pack_args_type_ui;
typedef struct { unsigned long ul1;		} pack_args_type_ul;
typedef struct { float f1;			} pack_args_type_f;
typedef struct { float f1,f2;			} pack_args_type_2f;
typedef struct { const void *p1;		} pack_args_type_p;
typedef struct { const void *p1,*p2;		} pack_args_type_2p;
typedef struct { const void *p1,*p2,*p3;	} pack_args_type_3p;
typedef struct { const void *p1,*p2,*p3,*p4;	} pack_args_type_4p;
typedef struct { const void *p1,*p2,*str;	} pack_args_type_2pV;
typedef struct { int i1;
		 const void *p1,*str;		} pack_args_type_ipV;
typedef struct { int i1,i2; 
		 const void *p1,*p2;		} pack_args_type_2i2p;
typedef struct { const void *p1,*p2;
		 float f1,f2;	 		} pack_args_type_2p2f;
typedef struct { const void *p1,*p2;
		 int i1,i2; 
		 const void *p3,*p4;		} pack_args_type_2p2i2p;
typedef struct { const void *p1,*p2;
		 float f1,f2,f3;
		 unsigned /*short*/int us1;
		 unsigned /*char*/int uc1,uc2;	} pack_args_type_2p3fus2uc;
typedef struct { const void *p1,*p2;
		 float f1;			} pack_args_type_2pf;
typedef struct { const void *p1,*p2;
		 float f1;
		 int i1;			} pack_args_type_2pfi;
typedef struct { const void *p1,*p2;
		 int i1;			} pack_args_type_2pi;
typedef struct { const void *p1,*p2;
		 int i1;
		 const void *p3,*p4;		} pack_args_type_2pi2p;
typedef struct { const void *p1,*p2;
		 int i1;
		 float f1;
		 const void *p3,*p4;		} pack_args_type_2pifp2;
typedef struct { const void *p1,*p2,*p3;	
		 float f1,f2;
		 int i1,i2;			} pack_args_type_3p2f2i;
typedef struct { const void *p1,*p2,*p3;
		 int i1;
		 const void *p4,*p5;		} pack_args_type_3pi2p;
typedef struct { int i1;
		 const void *p1,*p2,*p3;	} pack_args_type_i3p;
typedef struct { int i1;
		 const void *p1;		} pack_args_type_ip;
typedef struct { int i1;
		 const void *p1;
		 unsigned /*short*/int us1;
		 float f1;
		 const void *p2,*p3;
		 float f2,f3;
		 int i2,i3,i4,i5;		} pack_args_type_ipusf2p2f4i;
typedef struct { const void *p1,*p2,*p3;
		 int i1;			} pack_args_type_3pi;
typedef struct { const void *p1,*p2,*p3,*p4;
		 int i1;			} pack_args_type_4pi;
typedef struct { const void *p1;
		 float f1;			} pack_args_type_pf;
typedef struct { const void *p1;
		 float f1;
		 const void *p2;		} pack_args_type_pfp;
typedef struct { const void *p1;
		 int i1;			} pack_args_type_pi;
typedef struct { const void *p1;
		 int i1;
		 const void *p2,*p3;		} pack_args_type_pi2p;
typedef struct { const void *p1;
		 int i1;
		 const void *p2;		} pack_args_type_pip;
typedef struct { const void *p1;
		 int i1;
		 const void *p2;
		 float f1,f2;
		 int i2,i3;			} pack_args_type_pip2f2i;
typedef struct { const void *p1;
		 int i1;
		 const void *p2;
		 float f1,f2;
		 int i2,i3,i4,i5;
		 const void *p3,*p4;		} pack_args_type_pip2f4i2p;
typedef struct { const void *p1;
		 unsigned /*char*/int uc1;	} pack_args_type_puc;
typedef struct { int i1,i2; 
		 const void *p1,*p2;
		 int i3;
		 const void *p3,*p4;		} pack_args_type_2i2pi2p;
typedef struct { const void *p1,*p2;
		 int i1;
		 float f1;
		 const void *p3,*p4;		} pack_args_type_2pif2p;
typedef struct { const void *p1,*p2;
		 unsigned int ui1;		} pack_args_type_2pui;
typedef struct { int i1;
		 const void *p1,*p2;		} pack_args_type_i2p;
typedef struct { const void *p1;
		 float f1,f2;			} pack_args_type_p2f;
typedef struct { const void *p1;
		 float f1,f2;
		 int i1;			} pack_args_type_p2fi;
typedef struct { const void *p1;
		 int i1,i2;			} pack_args_type_p2i;
typedef struct { const void *p1;
		 int i1,i2,i3;			} pack_args_type_p3i;
typedef struct { const void *p1;
		 int i1,i2,i3,i4;		} pack_args_type_p4i;
typedef struct { const void *p1;
		 int i1;
		 const void *p2,*p3;
		 int i2,i3;
		 const void *p4;		} pack_args_type_pi2p2ip;

//
// API function callers.
//
#ifdef __METAMOD_BUILD__
	#define EXTERN_API_CALLER_FUNCTION(ret_type, args_code) \
		void * DLLINTERNAL _COMBINE4(api_caller_, ret_type, _args_, args_code)(const void * func, const void * packed_args)
#else
	#define EXTERN_API_CALLER_FUNCTION(ret_type, args_code) \
		static const api_caller_func_t _COMBINE4(api_caller_, ret_type, _args_, args_code) DLLHIDDEN = (api_caller_func_t)0
#endif

EXTERN_API_CALLER_FUNCTION(void, ipV);
EXTERN_API_CALLER_FUNCTION(void, 2pV);
EXTERN_API_CALLER_FUNCTION(void, void);
EXTERN_API_CALLER_FUNCTION(ptr, void);
EXTERN_API_CALLER_FUNCTION(int, void);
EXTERN_API_CALLER_FUNCTION(float, void);
EXTERN_API_CALLER_FUNCTION(float, 2f);
EXTERN_API_CALLER_FUNCTION(void, 2i);
EXTERN_API_CALLER_FUNCTION(int, 2i);
EXTERN_API_CALLER_FUNCTION(void, 2i2p);
EXTERN_API_CALLER_FUNCTION(void, 2i2pi2p);
EXTERN_API_CALLER_FUNCTION(void, 2p);
EXTERN_API_CALLER_FUNCTION(ptr, 2p);
EXTERN_API_CALLER_FUNCTION(int, 2p);
EXTERN_API_CALLER_FUNCTION(void, 2p2f);
EXTERN_API_CALLER_FUNCTION(void, 2p2i2p);
EXTERN_API_CALLER_FUNCTION(void, 2p3fus2uc);
EXTERN_API_CALLER_FUNCTION(ptr, 2pf);
EXTERN_API_CALLER_FUNCTION(void, 2pfi);
EXTERN_API_CALLER_FUNCTION(void, 2pi);
EXTERN_API_CALLER_FUNCTION(int, 2pi);
EXTERN_API_CALLER_FUNCTION(void, 2pui);
EXTERN_API_CALLER_FUNCTION(void, 2pi2p);
EXTERN_API_CALLER_FUNCTION(void, 2pif2p);
EXTERN_API_CALLER_FUNCTION(int, 3i);
EXTERN_API_CALLER_FUNCTION(void, 3p);
EXTERN_API_CALLER_FUNCTION(ptr, 3p);
EXTERN_API_CALLER_FUNCTION(int, 3p);
EXTERN_API_CALLER_FUNCTION(void, 3p2f2i);
EXTERN_API_CALLER_FUNCTION(int, 3pi2p);
EXTERN_API_CALLER_FUNCTION(void, 4p);
EXTERN_API_CALLER_FUNCTION(int, 4p);
EXTERN_API_CALLER_FUNCTION(void, 4pi);
EXTERN_API_CALLER_FUNCTION(int, 4pi);
EXTERN_API_CALLER_FUNCTION(void, f);
EXTERN_API_CALLER_FUNCTION(void, i);
EXTERN_API_CALLER_FUNCTION(ptr, i);
EXTERN_API_CALLER_FUNCTION(int, i);
EXTERN_API_CALLER_FUNCTION(ptr, ui);
EXTERN_API_CALLER_FUNCTION(uint, ui);
EXTERN_API_CALLER_FUNCTION(ulong, ul);
EXTERN_API_CALLER_FUNCTION(void, i2p);
EXTERN_API_CALLER_FUNCTION(int, i2p);
EXTERN_API_CALLER_FUNCTION(void, i3p);
EXTERN_API_CALLER_FUNCTION(void, ip);
EXTERN_API_CALLER_FUNCTION(ushort, ip);
EXTERN_API_CALLER_FUNCTION(int, ip);
EXTERN_API_CALLER_FUNCTION(void, ipusf2p2f4i);
EXTERN_API_CALLER_FUNCTION(void, p);
EXTERN_API_CALLER_FUNCTION(ptr, p);
EXTERN_API_CALLER_FUNCTION(char, p);
EXTERN_API_CALLER_FUNCTION(int, p);
EXTERN_API_CALLER_FUNCTION(uint, p);
EXTERN_API_CALLER_FUNCTION(float, p);
EXTERN_API_CALLER_FUNCTION(void, p2f);
EXTERN_API_CALLER_FUNCTION(int, p2fi);
EXTERN_API_CALLER_FUNCTION(void, p2i);
EXTERN_API_CALLER_FUNCTION(void, p3i);
EXTERN_API_CALLER_FUNCTION(void, p4i);
EXTERN_API_CALLER_FUNCTION(void, puc);
EXTERN_API_CALLER_FUNCTION(void, pf);
EXTERN_API_CALLER_FUNCTION(void, pfp);
EXTERN_API_CALLER_FUNCTION(void, pi);
EXTERN_API_CALLER_FUNCTION(ptr, pi);
EXTERN_API_CALLER_FUNCTION(int, pi);
EXTERN_API_CALLER_FUNCTION(void, pi2p);
EXTERN_API_CALLER_FUNCTION(int, pi2p2ip);
EXTERN_API_CALLER_FUNCTION(void, pip);
EXTERN_API_CALLER_FUNCTION(ptr, pip);
EXTERN_API_CALLER_FUNCTION(void, pip2f2i);
EXTERN_API_CALLER_FUNCTION(void, pip2f4i2p);

#endif /*API_HOOK_H*/
