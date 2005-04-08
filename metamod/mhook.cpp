// vi: set ts=4 sw=4 :
// vim: set tw=75 :

#ifdef UNFINISHED

// mhook.cpp - functions for list of hooked events (class MHookList)

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

#include <errno.h>		// strerror, etc

#include <extdll.h>		// always

#include "mhook.h"		// me
#include "mplugin.h"	// class MPlugin, etc
#include "mqueue.h"		// class MFuncQueue, etc
#include "metamod.h"	// global Plugins, etc
#include "log_meta.h"	// META_ERROR, etc
#include "osdep.h"		// MUTEX_INIT, etc
#include "types_meta.h"	// mBOOL


///// class MHook:

// Note: values are assigned with member initialization lists rather than
// using assignment statements, to satisfy -Weffc++, and as described in
// O'Reilly "C++: The Core Language" page 109.

// Constructor, generic
MHook::MHook(void)
	: index(-1), type(H_NONE), plid(NULL), pl_index(0), pfnHandle(NULL),
	  event(EV_NONE), match(NULL)
{
	return;
}

// Constructor for Game Event
MHook::MHook(int idx, plid_t pid, game_event_t evt, event_func_t pfn)
	: index(idx), type(H_EVENT), plid(pid), pl_index(0), pfnHandle(pfn),
	  event(evt), match(NULL)
{
	MPlugin *plug;
	if((plug=Plugins->find(pid))) {
		pl_index=plug->index;
	}
	return;
}

// Constructor for Log matches
MHook::MHook(int idx, plid_t pid, hook_t tp, const char *cp, logmatch_func_t pfn)
	: index(idx), type(tp), plid(pid), pl_index(0), pfnHandle(pfn),
	  event(EV_NONE), match(cp)
{
	MPlugin *plug;
	if((plug=Plugins->find(pid))) {
		pl_index=plug->index;
	}
	return;
}

// Call the hook directly.  This should be done only from the main
// thread.
//
// meta_errno values:
//  - ME_BADREQ		missing necessary info
//  - ME_NOTFOUND	couldn't find plugin for this hook ?
//  - ME_SKIPPED	plugin isn't running (paused, etc)
//  - ME_ARGUMENT	unrecognized hook type
mBOOL MHook::call(event_args_t *args, const char *logline) {
	MPlugin *plug;
	// missing necessary data
	if(type==H_NONE || !pfnHandle)
		RETURN_ERRNO(mFALSE, ME_BADREQ);
	// no event for event hook
	if(type==H_EVENT && event==EV_NONE)
		RETURN_ERRNO(mFALSE, ME_BADREQ);
	// no match string for logline hook
	if(type!=H_EVENT && !match)
		RETURN_ERRNO(mFALSE, ME_BADREQ);

	if(!(plug=Plugins->find(pl_index))) {
		META_WARNING("Couldn't find plugin index '%d' for hook", pl_index);
		RETURN_ERRNO(mFALSE, ME_NOTFOUND);
	}

	// Skip any paused plugins, for instance...
	if(plug->status!=PL_RUNNING)
		RETURN_ERRNO(mFALSE, ME_SKIPPED);

	switch(type) {
		case H_EVENT:
			{
				event_func_t pfn;
				pfn=(event_func_t) pfnHandle;
				pfn(event, args, logline);
			}
			break;
		case H_TRIGGER:
		case H_STRING:
		case H_REGEX:
			{
				logmatch_func_t pfn;
				pfn=(logmatch_func_t) pfnHandle;
				pfn(match, args, logline);
			}
			break;
		default:
			META_WARNING("Unrecognized hook type (%d) from plugin '%s'",
					type, plid->name);
			RETURN_ERRNO(mFALSE, ME_ARGUMENT);
			break;
	}
	return(mTRUE);
}

// Place function in the queue, to be subsequently called by the main
// thread.  This should be done only from the log parsing thread.
//
// meta_errno values:
//  - ME_ARGUMENT	null queue, event args, or logline
//  - ME_NOMEM		malloc failed for new entry to queue
mBOOL MHook::enqueue(MFuncQueue *mfq, event_args_t *args, const char *logline) {
	func_item_t *fp;

	if(!mfq || !args || !logline)
		RETURN_ERRNO(mFALSE, ME_ARGUMENT);

	fp = (func_item_t *) calloc(1, sizeof(func_item_t));
	if(!fp) {
		META_WARNING("malloc failed for func_item_t");
		RETURN_ERRNO(mFALSE, ME_NOMEM);
	}
	fp->hook=this;
	fp->evargs=args;
	fp->logline=logline;
	mfq->push(fp);
	return(mTRUE);
}


///// class MHookList:

// Constructor
MHookList::MHookList(void)
	: size(MAX_HOOKS), endlist(0), mx_hlist()
{
	// initialize array
	memset(hlist, 0, sizeof(hlist));
	endlist=0;
	MUTEX_INIT(&mx_hlist);
}

// Add a GameEvent hook to the list.
// Returns index of hook (hookid); zero on failure.
// meta_errno values:
//  - ME_ARGUMENT		null plid or pfn
//  - ME_BADREQ			no event specified
//  - ME_MAXREACHED		reached max hooks
//  - ME_NOMEM			couldn't malloc to create MHook entry
int MHookList::add(plid_t plid, game_event_t event, event_func_t pfnHandle)
{
	int i;

	if(!plid || !pfnHandle)
		RETURN_ERRNO(0, ME_ARGUMENT);
	if(event == EV_NONE)
		RETURN_ERRNO(0, ME_BADREQ);

	// any open slots?
	if(endlist==size) {
		plugin_info_t *plinfo;
		plinfo=(plugin_info_t *) plid;
		META_WARNING("Couldn't add game event hook for plugin '%s'; reached max hooks (%d)", plinfo->name, size);
		RETURN_ERRNO(0, ME_MAXREACHED);
	}

	// Find either:
	//  - a slot in the list that's not being used (malloc'd)
	//  - the end of the list
	for(i=0; hlist[i]; i++);
	if(i==endlist)
		endlist++;

	MXlock();
	hlist[i] = new MHook(i+1, plid, event, pfnHandle);
	MXunlock();

	// malloc failed?
	if(!hlist[i]) {
		plugin_info_t *plinfo;
		plinfo=(plugin_info_t *) plid;
		META_WARNING("Couldn't add game event hook for plugin '%s'; malloc failed: %s", plinfo->name, strerror(errno));
		RETURN_ERRNO(0, ME_NOMEM);
	}

	return(i);
}

// Add a LogEvent hook (trigger, string, or regex) to the list.
// Returns index of hook (hookid); zero on failure.
// meta_errno values:
//  - ME_ARGUMENT		null plid, match string, or pfn
//  - ME_BADREQ			hook type is none or event (should be trigger, string, 
//  					or regex)
//  - ME_MAXREACHED		reached max hooks
//  - ME_NOMEM			couldn't malloc to create MHook entry
int MHookList::add(plid_t plid, hook_t type, const char *match, 
		logmatch_func_t pfnHandle)
{
	int i;

	if(!plid || !match || !pfnHandle)
		RETURN_ERRNO(0, ME_ARGUMENT);
	if(type == H_NONE || type == H_EVENT)
		RETURN_ERRNO(0, ME_BADREQ);

	// any open slots?
	if(endlist==size) {
		plugin_info_t *plinfo;
		plinfo=(plugin_info_t *) plid;
		META_WARNING("Couldn't add log hook '%s' for plugin '%s'; reached max hooks (%d)", match, plinfo->name, size);
		RETURN_ERRNO(0, ME_MAXREACHED);
	}

	// Find either:
	//  - a slot in the list that's not being used (malloc'd)
	//  - the end of the list
	for(i=0; hlist[i]; i++);
	if(i==endlist)
		endlist++;

	MXlock();
	hlist[i] = new MHook(i+1, plid, type, match, pfnHandle);
	MXunlock();

	// malloc failed?
	if(!hlist[i]) {
		plugin_info_t *plinfo;
		plinfo=(plugin_info_t *) plid;
		META_WARNING("Couldn't add game event hook for plugin '%s'; malloc failed: %s", plinfo->name, strerror(errno));
		RETURN_ERRNO(0, ME_NOMEM);
	}

	return(i);
}

// Remove hook at the given index in list.
//
// meta_errno values:
//  - ME_NOTFOUND	no hook at that index
//  - ME_BADREQ		plugin trying to remove another plugin's hook?
mBOOL MHookList::remove(plid_t plid, int hindex) {
	int i=hindex;
	if(!hlist[i])
		RETURN_ERRNO(mFALSE, ME_NOTFOUND);
	if(hlist[i]->plid != plid)
		RETURN_ERRNO(mFALSE, ME_BADREQ);

	MXlock();
	delete(hlist[i]);
	hlist[i]=NULL;
	MXunlock();
	return(mTRUE);
}

// Remove any hooks for the specified plugin.
int MHookList::remove_all(plid_t plid) {
	int i;
	int count=0;

	for(i=0; i < endlist && hlist[i]; i++) {
		if(hlist[i]->plid == plid) {
			MXlock();
			delete(hlist[i]);
			hlist[i]=NULL;
			MXunlock();
			count++;
		}
	}
	return(count);
}

// Call a hook directly.  This should only be done from the main thread,
// and then only for pre-determined-event hooks.  Note logline is allowed
// to be empty, as some events aren't logmsg-detected.
//
// meta_errno values:
//  - ME_ARGUMENT	null args
//  - ME_BADREQ		args doesn't correspond to an Event hook
mBOOL MHookList::call(event_args_t *evargs, const char *logline) 
{
	int i;

	if(!evargs)
		RETURN_ERRNO(mFALSE, ME_ARGUMENT);
	if(evargs->evtype==EV_NONE)
		RETURN_ERRNO(mFALSE, ME_BADREQ);

	MXlock();
	for(i=0; i < endlist && hlist[i]; i++) {
		if(hlist[i]->event == evargs->evtype)
			hlist[i]->call(evargs, logline);
	}
	MXunlock();
	return(mTRUE);
}

// Find any matching hooks, and place them in the queue, to be subsequently
// called by the main thread.  This should be done only from the log
// parsing thread.
// meta_errno values:
//  - ME_ARGUMENT	null args
//  - ME_BADREQ		invalid hook type
mBOOL MHookList::enqueue(MFuncQueue *mfq, hook_t htype, event_args_t *evargs,
		const char *logline) 
{
	int i;
	META_ERRNO ret=ME_NOERROR;

	if(!mfq || !evargs || !logline)
		RETURN_ERRNO(mFALSE, ME_ARGUMENT);
	if(htype==H_NONE)
		RETURN_ERRNO(mFALSE, ME_BADREQ);

	MXlock();
	for(i=0; i < endlist && hlist[i] && hlist[i]->type==htype; i++) {
		MHook *ihook;
		ihook=hlist[i];
		switch(ihook->type) {
			case H_EVENT:
				if(ihook->event == evargs->evtype)
					ihook->enqueue(mfq, evargs, logline);
				break;
			case H_TRIGGER:
				if(strmatch(ihook->match, evargs->action))
					ihook->enqueue(mfq, evargs, logline);
				break;
			case H_STRING:
				if(strstr(logline, ihook->match))
					ihook->enqueue(mfq, evargs, logline);
				break;
			case H_REGEX:
				META_LOG("Not currently handling REGEX hooks...");
				ret=ME_BADREQ;
				break;
			default:
				META_WARNING("Invalid hook type (%d) encountered", ihook->type);
				ret=ME_BADREQ;
				break;
		}
	}
	MXunlock();
	if(ret==ME_NOERROR)
		return(mTRUE);
	else
		RETURN_ERRNO(mFALSE, ret);
}

// Return a string corresponding to the hook type.
char *MHookList::str_htype(hook_t htype) {
	switch(htype) {
		case H_NONE:	return("NONE");
		case H_EVENT:	return("EVENT");
		case H_TRIGGER:	return("TRIGGER");
		case H_STRING:	return("STRING");
		case H_REGEX:	return("REGEX");
		default:		return("(unknown)");
	}
}

#endif /* UNFINISHED */
