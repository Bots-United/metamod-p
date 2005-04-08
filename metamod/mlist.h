// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// mlist.h - class and constants to describe a list of plugins

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


// A list of plugins.
class MPluginList : public class_metamod_new {
	public:
	// data:
		MPlugin plist[MAX_PLUGINS];			// array of plugins
		int size;					// size of list, ie MAX_PLUGINS
		int endlist;					// index of last used entry
		char inifile[PATH_MAX];				// full pathname

	// constructor:
		MPluginList(const char *ifile);

	// functions:
		void reset_plugin(MPlugin *pl_find);
		MPlugin *find(int pindex);			// find by index
		MPlugin *find(const char *findpath); 		// find by pathname
		MPlugin *find(plid_t id);			// find by plid_t
		MPlugin *find(DLHANDLE handle);			// find by handle
		MPlugin *find_memloc(void *memptr);		// find by memory location
		MPlugin *find_match(const char *prefix);	// find by partial prefix match
		MPlugin *find_match(MPlugin *pmatch);		// find by platform_match()
		MPlugin *add(MPlugin *padd);
				
		mBOOL found_child_plugins(int source_index);
		void clear_source_plugin_plugin(int source_index);
		void trim_list(void);
				
		mBOOL ini_startup(void);			// read inifile at startup
		mBOOL ini_refresh(void);			// re-read inifile
		mBOOL cmd_addload(const char *args);		// load from console command
		MPlugin *plugin_addload(plid_t plid, const char *fname, PLUG_LOADTIME now); //load from plugin

		mBOOL load(void);				// load the list, at startup
		mBOOL refresh(PLUG_LOADTIME now);		// update from re-read inifile
		void unpause_all(void);				// unpause any paused plugins
		void retry_all(PLUG_LOADTIME now);		// retry any pending plugin actions
		void show(int source_index);			// list plugins to console
		void show(void) { show(-1); };			// list plugins to console
		void show_client(edict_t *pEntity);		// list plugins to player client
};

#endif /* MLIST_H */
