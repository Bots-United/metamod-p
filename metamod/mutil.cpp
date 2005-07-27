// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// mutil.cpp - utility functions to provide to plugins

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

#include <stdio.h>			// vsnprintf(), etc
#include <stdarg.h>			// vs_start(), etc

#include <extdll.h>			// always

#include "meta_api.h"		// 
#include "mutil.h"			// me
#include "mhook.h"			// class MHookList, etc
#include "linkent.h"		// ENTITY_FN, etc
#include "metamod.h"		// Hooks, etc
#include "types_meta.h"		// mBOOL
#include "osdep.h"			// win32 vsnprintf, etc
#include "sdk_util.h"		// ALERT, etc

static hudtextparms_t default_csay_tparms = {
	-1, 0.25,			// x, y
	2,					// effect
	0, 255, 0,	0,		// r, g, b,  a1
	0, 0, 0,	0,		// r2, g2, b2,  a2
	0, 0, 10, 10,		// fadein, fadeout, hold, fxtime
	1					// channel
};

// Log to console; newline added.
static void mutil_LogConsole(plid_t /* plid */, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	// end msg with newline
	len=strlen(buf);
	if(likely(len < sizeof(buf)-2))		// -1 null, -1 for newline
		strcat(buf, "\n");
	else
		buf[len-1] = '\n';

	SERVER_PRINT(buf);
}

// Log regular message to logs; newline added.
static void mutil_LogMessage(plid_t plid, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	plugin_info_t *plinfo;

	plinfo=(plugin_info_t *)plid;
	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[%s] %s\n", plinfo->logtag, buf);
}

// Log an error message to logs; newline added.
static void mutil_LogError(plid_t plid, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	plugin_info_t *plinfo;

	plinfo=(plugin_info_t *)plid;
	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[%s] ERROR: %s\n", plinfo->logtag, buf);
}

// Log a message only if cvar "developer" set; newline added.
static void mutil_LogDeveloper(plid_t plid, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	plugin_info_t *plinfo;

	if(likely((int)CVAR_GET_FLOAT("developer") == 0))
		return;

	plinfo=(plugin_info_t *)plid;
	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[%s] dev: %s\n", plinfo->logtag, buf);
}

// Print a center-message, with text parameters and varargs.  Provides
// functionality to the above center_say interfaces.
static void mutil_CenterSayVarargs(plid_t plid, hudtextparms_t tparms, 
		const char *fmt, va_list ap) 
{
	char buf[MAX_LOGMSG_LEN];
	int n;
	edict_t *pEntity;

	safe_vsnprintf(buf, sizeof(buf), fmt, ap);

	mutil_LogMessage(plid, "(centersay) %s", buf);
	for(n=1; likely(n <= gpGlobals->maxClients); n++) {
		pEntity=INDEXENT(n);
		META_UTIL_HudMessage(pEntity, tparms, buf);
	}
}

// Print message on center of all player's screens.  Uses default text
// parameters (color green, 10 second fade-in).
static void mutil_CenterSay(plid_t plid, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	mutil_CenterSayVarargs(plid, default_csay_tparms, fmt, ap);
	va_end(ap);
}

// Print a center-message, with given text parameters.
static void mutil_CenterSayParms(plid_t plid, hudtextparms_t tparms, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	mutil_CenterSayVarargs(plid, tparms, fmt, ap);
	va_end(ap);
}

// Allow plugins to call the entity functions in the GameDLL.  In
// particular, calling "player()" as needed by most Bots.  Suggested by
// Jussi Kivilinna.
static qboolean mutil_CallGameEntity(plid_t plid, const char *entStr, entvars_t *pev) {
	plugin_info_t *plinfo;
	ENTITY_FN pfnEntity;

	plinfo=(plugin_info_t *)plid;
	META_DEBUG(8, ("Looking up game entity '%s' for plugin '%s'", entStr,
				plinfo->name));
	pfnEntity = (ENTITY_FN) DLSYM(GameDLL.handle, entStr);
	if(unlikely(!pfnEntity)) {
		META_WARNING("Couldn't find game entity '%s' in game DLL '%s' for plugin '%s'", entStr, GameDLL.name, plinfo->name);
		return(false);
	}
	META_DEBUG(7, ("Calling game entity '%s' for plugin '%s'", entStr,
				plinfo->name));
	(*pfnEntity)(pev);
	return(true);
}

// Find a usermsg, registered by the gamedll, with the corresponding
// msgname, and return remaining info about it (msgid, size).
static int mutil_GetUserMsgID(plid_t plid, const char *msgname, int *size) {
	plugin_info_t *plinfo;
	MRegMsg *umsg;

	plinfo=(plugin_info_t *)plid;
	META_DEBUG(8, ("Looking up usermsg name '%s' for plugin '%s'", msgname,
				plinfo->name));
	umsg=RegMsgs->fast_find(msgname);
	if(likely(umsg)) {
		if(unlikely(size))
			*size=umsg->size;
		return(umsg->msgid);
	}
	else
		return(0);
}

// Find a usermsg, registered by the gamedll, with the corresponding
// msgid, and return remaining info about it (msgname, size).
static const char *mutil_GetUserMsgName(plid_t plid, int msgid, int *size) {
	plugin_info_t *plinfo;
	MRegMsg *umsg;

	plinfo=(plugin_info_t *)plid;
	META_DEBUG(8, ("Looking up usermsg id '%d' for plugin '%s'", msgid,
				plinfo->name));
	// Guess names for any built-in Engine messages mentioned in the SDK;
	// from dlls/util.h.
	if(unlikely(msgid < 64)) {
		switch(msgid) {
			case SVC_TEMPENTITY:
				if(size) *size=-1;
				return("tempentity?");
			case SVC_INTERMISSION:
				if(size) *size=-1;
				return("intermission?");
			case SVC_CDTRACK:
				if(size) *size=-1;
				return("cdtrack?");
			case SVC_WEAPONANIM:
				if(size) *size=-1;
				return("weaponanim?");
			case SVC_ROOMTYPE:
				if(size) *size=-1;
				return("roomtype?");
			case SVC_DIRECTOR:
				if(size) *size=-1;
				return("director?");
		}
	}
	umsg=RegMsgs->find(msgid);
	if(likely(umsg)) {
		if(unlikely(size))
			*size=umsg->size;
		// 'name' is assumed to be a constant string, allocated in the
		// gamedll.
		return(umsg->name);
	}
	else
		return(NULL);
}

// Return the full path of the plugin's loaded dll/so file.
static const char *mutil_GetPluginPath(plid_t plid) {
	static char buf[PATH_MAX];
	MPlugin *plug;

	plug=Plugins->find(plid);
	if(unlikely(!plug)) {
		META_WARNING("GetPluginPath: couldn't find plugin '%s'",
				plid->name);
		return(NULL);
	}
	STRNCPY(buf, plug->pathname, sizeof(buf));
	return(buf);
}

// Return various string-based info about the game/MOD/gamedll.
static const char *mutil_GetGameInfo(plid_t plid, ginfo_t type) {
	static char buf[MAX_STRBUF_LEN];
	const char *cp;
	switch(type) {
		case GINFO_NAME:
			cp=GameDLL.name;
			break;
		case GINFO_DESC:
			cp=GameDLL.desc;
			break;
		case GINFO_GAMEDIR:
			cp=GameDLL.gamedir;
			break;
		case GINFO_DLL_FULLPATH:
			cp=GameDLL.pathname;
			break;
		case GINFO_DLL_FILENAME:
			cp=GameDLL.file;
			break;
		case GINFO_REALDLL_FULLPATH:
			cp=GameDLL.real_pathname;
			break;
		default:
			META_WARNING("GetGameInfo: invalid request '%d' from plugin '%s'",
					type, plid->name);
			return(NULL);
	}
	STRNCPY(buf, cp, sizeof(buf));
	return(buf);
}

// Load plugin by filename. Works like "meta load <plugin>" (multiple plugins with one call not supported).
static int mutil_LoadMetaPluginByName(plid_t plid, const char *cmdline, PLUG_LOADTIME now, void **plugin_handle) {
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
static int mutil_UnloadMetaPluginByName(plid_t plid, const char *cmdline, PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
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
static int mutil_UnloadMetaPluginByHandle(plid_t plid, void *plugin_handle, PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
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

#ifdef UNFINISHED
static int mutil_HookGameEvent(plid_t plid, game_event_t event, 
		event_func_t pfnHandle) 
{
	return(Hooks->add(plid, event, pfnHandle));
}

static int mutil_HookLogTrigger(plid_t plid, const char *trigger, 
		logmatch_func_t pfnHandle) 
{
	return(Hooks->add(plid, H_TRIGGER, trigger, pfnHandle));
}

static int mutil_HookLogString(plid_t plid, const char *string, 
		logmatch_func_t pfnHandle) 
{
	return(Hooks->add(plid, H_STRING, string, pfnHandle));
}

static int mutil_HookLogRegex(plid_t plid, const char *pattern, 
		logmatch_func_t pfnHandle) 
{
	return(Hooks->add(plid, H_STRING, pattern, pfnHandle));
}

static qboolean mutil_RemoveHookID(plid_t plid, int hookid) {
	mBOOL ret;
	ret=Hooks->remove(plid, hookid);
	if(likely(ret==mTRUE)) return(true);
	else return(false);
}

static int mutil_RemoveHookAll(plid_t plid) {
	return(Hooks->remove_all(plid));
}
#endif /* UNFINISHED */

// Meta Utility Function table.
mutil_funcs_t MetaUtilFunctions = {
	mutil_LogConsole,		// pfnLogConsole
	mutil_LogMessage,		// pfnLogMessage
	mutil_LogError,			// pfnLogError
	mutil_LogDeveloper,		// pfnLogDeveloper
	mutil_CenterSay,		// pfnCenterSay
	mutil_CenterSayParms,	// pfnCenterSayParms
	mutil_CenterSayVarargs,	// pfnCenterSayVarargs
	mutil_CallGameEntity,	// pfnCallGameEntity
	mutil_GetUserMsgID,		// pfnGetUserMsgID
	mutil_GetUserMsgName,	// pfnGetUserMsgName
	mutil_GetPluginPath,	// pfnGetPluginPath
	mutil_GetGameInfo,		// pfnGetGameInfo
	mutil_LoadMetaPluginByName, // pfnLoadPlugin
	mutil_UnloadMetaPluginByName, // pfnUnloadPlugin
	mutil_UnloadMetaPluginByHandle, // pfnUnloadPluginByHandle
#ifdef UNFINISHED
	mutil_HookGameEvent,	// pfnGameEvent
	mutil_HookLogTrigger,	// pfnLogTrigger
	mutil_HookLogString,	// pfnLogString
	mutil_HookLogRegex,		// pfnLogRegex
	mutil_RemoveHookID,		// pfnRemoveHookID
	mutil_RemoveHookAll,	// pfnRemoveHookAll
#endif /* UNFINISHED */
};
