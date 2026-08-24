// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// mlist.h - class and constants to describe a list of plugins

/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
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

#ifndef MLIST_H
#define MLIST_H

#include "types_meta.h"			// mBOOL
#include "mplugin.h"			// class MPlugin
#include "plinfo.h"			// plid_t, etc
#include "new_baseclass.h"

// Max number of plugins we can manage.  This is an arbitrary, fixed number,
// for convenience.  It would probably be better to dynamically grow the
// list as needed, but we do this for now.
#define MAX_PLUGINS 50
// Width required to printf above MAX, for show() functions.
#define WIDTH_MAX_PLUGINS	2

// Pre-filtered plugin list for a single API function + phase combination.
// Contains only running plugins that hook that one function.
struct api_plugin_list_t {
	int count;
	MPlugin **plugs;
};

// Hook lists are kept per API function rather than per API group.  A plugin
// that registers a table fills a handful of its slots, and the tables hold
// 159 (engine), 50 (dllapi) and 5 (newapi) of them, so one list per group
// leaves every dispatch walking over the plugins that hook some other
// function of the same table just to skip them.
//
// A function is addressed by its slot in its own table, which is what a
// func_offset already is: every slot in these tables is pointer sized, and
// the dispatcher relies on that when it reaches a table entry by byte
// offset.  Each group gets the same power-of-two stride so that a dispatch
// can reach its list by shifting func_offset rather than by loading a per
// group base - one dependent load fewer on the hot path, which the shortest
// wrappers do notice.  It costs the padding between HOOK_SLOTS_* and the
// stride, all of it untouched.
//
// HOOK_SLOTS_* is how many slots a plugin's table has, which is what may be
// scanned, and that is not sizeof() for the engine: mplugin.cpp allocates a
// plugin's engine table without the extra_functions tail, so the reserved
// slots are not there to read.
#define HOOK_SLOTS_ENGINE	((int)((sizeof(enginefuncs_t) \
					- sizeof(((enginefuncs_t *)0)->extra_functions)) / sizeof(void *)))
#define HOOK_SLOTS_DLLAPI	((int)(sizeof(DLL_FUNCTIONS) / sizeof(void *)))
#define HOOK_SLOTS_NEWAPI	((int)(sizeof(NEW_DLL_FUNCTIONS) / sizeof(void *)))
#define HOOK_SLOTS_STRIDE	256

// Slots per group, indexed by enum_api_t.
extern const int DLLHIDDEN api_hook_slot_count[3];

// Total slots a plugin can fill, across all three groups, for the shadow
// copy below.
#define HOOK_SLOTS_TOTAL	(HOOK_SLOTS_ENGINE + HOOK_SLOTS_DLLAPI + HOOK_SLOTS_NEWAPI)

// Set by rebuild_hook_lists() to signal main_hook_function that cached
// pointers are stale and need to be refreshed.
extern mBOOL DLLHIDDEN hook_list_tables_updated;

// A list of plugins.
class MPluginList : public class_metamod_new {
	public:
	// data:
		api_plugin_list_t hook_lists[3][HOOK_SLOTS_STRIDE][2];	// pre-filtered per-function [pre,post] plugin arrays
		MPlugin **hook_list_data;			// backing allocation for hook_lists plugs pointers
		// Copy of what every running plugin had in its tables when the
		// hook lists were last built.  A plugin owns the table metamod
		// handed it and some edit it while running - AMX Mod X's fakemeta
		// register_forward() installs hooks that way - and the per-function
		// lists have to follow.  Compared once per frame; see
		// refresh_hook_lists_if_changed().
		void *hook_shadow[MAX_PLUGINS][2][HOOK_SLOTS_TOTAL];
		int size;					// size of list, ie MAX_PLUGINS
		int endlist;					// index of last used entry
		MPlugin plist[MAX_PLUGINS];			// array of plugins
		char inifile[PATH_MAX];				// full pathname

		// Both phases of one function: [0] is pre, [1] is post, on the
		// same cache line.  A dispatch looks each phase up where it uses
		// it: hoisting the pair to the top of main_hook_function costs
		// more than the second index calculation, because the pointer
		// then has to survive the plugin calls and gets spilled.
		inline DLLINTERNAL const api_plugin_list_t * get_hook_lists(enum_api_t api, unsigned int func_offset) {
			return(hook_lists[api][func_offset / sizeof(void *)]);
		}
		inline DLLINTERNAL const api_plugin_list_t * get_hook_list(enum_api_t api, unsigned int func_offset) {
			return(&get_hook_lists(api, func_offset)[0]);
		}
		inline DLLINTERNAL const api_plugin_list_t * get_hook_post_list(enum_api_t api, unsigned int func_offset) {
			return(&get_hook_lists(api, func_offset)[1]);
		}
		inline DLLINTERNAL int find_plugin_after_rebuild(
			enum_api_t api, unsigned int func_offset, mBOOL post, MPlugin *current_plugin,
			int &out_count, MPlugin * const * &out_plugs)
		{
			const api_plugin_list_t *list = &get_hook_lists(api, func_offset)[post ? 1 : 0];
			out_plugs = list->plugs;
			out_count = list->count;
			for(int j = 0; j < out_count; j++) {
				if(out_plugs[j] == current_plugin)
					return j;
				if(out_plugs[j] > current_plugin)
					return j - 1;
			}
			return out_count - 1;
		}

	// constructor/destructor:
		MPluginList(const char *ifile) DLLINTERNAL;
		// Not DLLINTERNAL: __cxa_atexit calls a static object's destructor
		// as plain cdecl, where regparm would look for `this` in a register.
		~MPluginList(void);

	// functions:
		void DLLINTERNAL reset_plugin(MPlugin *pl_find);
		MPlugin * DLLINTERNAL find(int pindex);			// find by index
		MPlugin * DLLINTERNAL find(const char *findpath); 	// find by pathname
		MPlugin * DLLINTERNAL find(plid_t id);			// find by plid_t
		MPlugin * DLLINTERNAL find(DLHANDLE handle);		// find by handle
		MPlugin * DLLINTERNAL find_memloc(void *memptr);	// find by memory location
		MPlugin * DLLINTERNAL find_match(const char *prefix);	// find by partial prefix match
		MPlugin * DLLINTERNAL find_match(MPlugin *pmatch);	// find by platform_match()
		MPlugin * DLLINTERNAL add(MPlugin *padd);
				
		mBOOL DLLINTERNAL found_child_plugins(int source_index);
		void DLLINTERNAL clear_source_plugin_index(int source_index);
		void DLLINTERNAL trim_list(void);
				
		mBOOL DLLINTERNAL ini_startup(void);			// read inifile at startup
		mBOOL DLLINTERNAL ini_refresh(void);			// re-read inifile
		mBOOL DLLINTERNAL cmd_addload(const char *args);	// load from console command
		MPlugin * DLLINTERNAL plugin_addload(plid_t plid, const char *fname, PLUG_LOADTIME now); //load from plugin

		mBOOL DLLINTERNAL load(void);				// load the list, at startup
		mBOOL DLLINTERNAL refresh(PLUG_LOADTIME now);		// update from re-read inifile
		void DLLINTERNAL rebuild_hook_lists(void);
		void DLLINTERNAL snapshot_hook_tables(void);
		mBOOL DLLINTERNAL hook_tables_changed(void);
		void DLLINTERNAL refresh_hook_lists_if_changed(void);
		void DLLINTERNAL unpause_all(void);			// unpause any paused plugins
		void DLLINTERNAL retry_all(PLUG_LOADTIME now);		// retry any pending plugin actions
		void DLLINTERNAL show(int source_index);		// list plugins to console
		void DLLINTERNAL show(void) { show(-1); };		// list plugins to console
		void DLLINTERNAL show_client(edict_t *pEntity);		// list plugins to player client
};

#endif /* MLIST_H */
