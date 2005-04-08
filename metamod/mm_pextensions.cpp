/*
 * Copyright (c) 2004-2005 Jussi Kivilinna
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
 
#include <extdll.h>			// always

#include "mm_pextensions.h"		// me?
#include "mplugin.h"			// MPlugin
#include "metamod.h"			// Plugins

//
// New all-mod-support-patch extensions
//
//  Check if plugin has "Meta_P_Extensions"-export and calls it with
//  pointer to extension function table.
//
//  -Jussi Kivilinna
//

// Load plugin by filename. Works like "meta load <plugin>" (multiple plugins with one call not supported).
static int pextension_LoadMetaPluginByName(plid_t plid, const char *cmdline, PLUG_LOADTIME now, void **plugin_handle) {
	MPlugin * pl_loaded;
	
	//try load plugin
	meta_errno = ME_NOERROR;
	if(unlikely(!(pl_loaded=Plugins->plugin_addload(plid, cmdline, now)))) {
		if(likely(plugin_handle))
			*plugin_handle = NULL;
		
		return((int)meta_errno);
	} else {
		if(likely(plugin_handle))
			*plugin_handle = (void*)pl_loaded->handle;
		
		return(0);
	}
}

// Unload plugin by filename. Works like "meta unload <plugin>" (multiple plugins with one call not supported).
// If loading fails, plugin is still loaded.
static int pextension_UnloadMetaPluginByName(plid_t plid, const char *cmdline, PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
	MPlugin *findp = 0;
	
	// try to match plugin id first
	if(unlikely(isdigit(cmdline[0])))
		findp=Plugins->find(atoi(cmdline));
	// else try to match some string (prefix)
	else
		findp=Plugins->find_match(cmdline);
	
	if(unlikely(!findp))
		return((int)ME_NOTUNIQ);
	
	//try unload plugin
	meta_errno = ME_NOERROR;
	if(likely(findp->plugin_unload(plid, now, reason)))
		return(0);
	else
		return((int)meta_errno);
}

// Unload plugin by handle. If loading fails, plugin is still loaded.
static int pextension_UnloadMetaPluginByHandle(plid_t plid, void *plugin_handle, PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
	MPlugin *findp;
	
	// try to match plugin handle
	if(unlikely(!(findp=Plugins->find((DLHANDLE)plugin_handle))))
		return((int)ME_NOTUNIQ);
	
	//try unload plugin
	meta_errno = ME_NOERROR;
	if(likely(findp->plugin_unload(plid, now, reason)))
		return(0);
	else
		return((int)meta_errno);
}

// All-Mod-Support-patch extensions table.
pextension_funcs_t MetaPExtensionFunctions = {
	pextension_LoadMetaPluginByName,	//pfnLoadMetaPluginByName
	pextension_UnloadMetaPluginByName,	//pfnUnloadMetaPluginByName
	pextension_UnloadMetaPluginByHandle,	//pfnUnloadMetaPluginByHandle
};
