// vi: set ts=4 sw=4 :
// vim: set tw=75 :

#ifdef UNFINISHED

// thread_logparse.cpp - thread to handle log parsing

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

#include <extdll.h>				// always

#include "thread_logparse.h"	// me
#include "mqueue.h"				// Queue template
#include "metamod.h"			// Hooks, etc
#include "support_meta.h"		// do_exit, etc
#include "log_meta.h"			// META_LOG, etc
#include "osdep.h"				// THREAD_T, etc


// Global queues
MLogmsgQueue *LogQueue;
MFuncQueue *HookQueue;

// thread id; not actually used at present
THREAD_T logparse_thread_id;


// Start the log parsing thread.
void DLLINTERNAL startup_logparse_thread(void) {
	int ret;

	LogQueue = new MLogmsgQueue(MAX_QUEUE_SIZE);
	HookQueue = new MFuncQueue(MAX_QUEUE_SIZE);

	ret=THREAD_CREATE(&logparse_thread_id, logparse_handler);
	if(ret != THREAD_OK) {
		// For now, we just exit if this fails.
		META_ERROR("Couldn't start thread for log parsing!  Exiting...");
		do_exit(1);
	}
}

// The main() routine for the log parsing thread.
void WINAPI logparse_handler(void) {
	const char *msg;
	event_args_t *evargs;
	while(1) {
		// Pull a logmsg from the queue; note that this blocks until a
		// message is available.
		msg=LogQueue->pop();

		// Pre-parse logmsg line for args and to determine some possible 
		// type of event from the logmsg.
		evargs=parse_event_args(msg);

		// Match any direct event hooks, as these are fastest.
		if(evargs && evargs->evtype != EV_NONE)
			Hooks->enqueue(HookQueue, H_EVENT, evargs, msg);

		// Match any trigger hooks, as these are less fast.
		if(evargs && evargs->action)
			Hooks->enqueue(HookQueue, H_TRIGGER, evargs, msg);

		// Match any substring hooks, as these are slower.
		Hooks->enqueue(HookQueue, H_STRING, evargs, msg);

#if 0
		// Match any regex hooks last, as these are slowest.
		Hooks->enqueue(HookQueue, H_REGEX, evargs, msg);
#endif
	}
}

// Pre-parse logmsg line for args and to determine some possible type of
// event.
//
// Returns an allocated event_args_t struct, which should be freed by the
// main thread after it's no longer needed.  Note that this struct gets
// passed to (possibly) multiple hooks.
event_args_t * DLLINTERNAL parse_event_args(const char *logline) {
	event_args_t *args;
	char *cp, *begin, *end;
	int len;

	// Create and init the event_args struct.
	args=(event_args_t *) calloc(1, sizeof(event_args_t));
	if(!args) 
		return(NULL);
	memset(args, 0, sizeof(*args));

	// Make a copy of the logline, as we're going to be setting NULLs in
	// the string to mark ends of various substrings.
	args->buf=strdup(logline);
	if(!args->buf) 
		return(NULL);
	cp=args->buf;

	// Grab the player name and attributes, or "world" or "team" name, ie:
	//    Joe<15><785><CT>
	//    World
	//    Team "CT"
	args->player=parse_player(cp, &len);
	cp+=len;

	// Look for one of several pre-determined actions that we recognize.

	// ... triggered "some action" ...
	// ie:   "Joe<15><785><CT>" triggered "Killed_A_Hostage"
	if(strnmatch(cp, "triggered ", 10)) {
		cp+=10;
		args->action=parse_quoted(cp, &len);
		cp+=len;
	}
	// ... killed Joe<15><785><CT> ...
	// ie:    "Joe<15><785><CT>" killed "Sam<17><197><TERRORIST>" with "sg552"
	else if(strnmatch(cp, "killed ", 7)) {
		cp+=7;
		args->target=parse_player(cp, &len);
		if(strmatch(args->player->team, args->target->team)) {
			args->action="team_kill";
			args->evtype=EV_TEAM_KILL;
		}
		else {
			args->action="kill";
		}
		cp+=len;
	}
	// ... committed suicide ...
	// ie:   "Joe<15><785><CT>" committed suicide with "worldspawn"
	else if(strnmatch(cp, "committed suicide", 17)) {
		cp+=17;
		args->action="suicide";
		cp+=strspn(cp, " ");
		args->evtype=EV_PLAYER_SUICIDE;
	}
	// ... joined team "someteam" ...
	// ie:   "Joe<15><785><>" joined team "CT"
	else if(strnmatch(cp, "joined team ", 12)) {
		cp+=12;
		args->action="join_team";
		args->target=(event_player_t *) calloc(1, sizeof(event_player_t));
		// Note:
		//    old team is in:   args->player->team
		//    new team is in:   args->target->team
		if(args->target) {
			memset(args->target, 0, sizeof(event_player_t));
			args->target->team=parse_quoted(cp, &len);
			cp+=len;
		}
		args->evtype=EV_PLAYER_JOIN_TEAM;
	}
	// ... changed role to "something" ...
	// from TFC, ie:   "Joe<15><785><Red>" changed role to "Pyro"
	else if(strnmatch(cp, "changed role to ", 16)) {
		cp+=16;
		args->action="change_role";
		// Note:
		//    new role is in:   args->with
		args->with=parse_quoted(cp, &len);
		cp+=len;
		args->evtype=EV_PLAYER_CHANGE_ROLE;
	}
	// ... scored ...
	// ie: Team "CT" scored "7" with "2" players
	else if(strnmatch(cp, "scored ", 7)) {
		cp+=7;
		if(args->player && args->player->team && !args->player->name) {
			args->action="team_score";
			args->evtype=EV_TEAM_SCORE;
		}
		else {
			args->action="score";
		}
		cp+=strspn(cp, " ");
	}
	// anything else...
	// Consider any words up to a quote (") to be the 'action'.
	else {
		begin=cp;
		end=strchr(begin, '"')-1;
		len=end-begin;
		if(len > 0) {
			*end='\0';
			args->action=begin;
			cp=end+1;
			cp+=strspn(cp, " ");
		}
	}

	// Look for associated phrases...

	// ... against ...
	// ie:   "Joe<15><785><Red>" triggered "Medic_Heal" against "Bob<27><954><Red>"
	if(strnmatch(cp, "against ", 8)) {
		cp+=8;
		args->target=parse_player(cp, &len);
		cp+=len;
	}

	// ... with ...
	// ie:    "Joe<15><785><CT>" killed "Sam<17><197><TERRORIST>" with "sg552"
	// or:    "Joe<15><785><Blue>" triggered "Sentry_Destroyed" against "Sam<17><197><Red>" with "mirvgrenade"
	if(strnmatch(cp, "with ", 5)) {
		cp+=5;
		args->with=parse_quoted(cp, &len);
		if(strmatch(args->action, "kill")) {
			args->action="weapon_kill";
			args->evtype=EV_WEAPON_KILL;
		}
		cp+=len;
	}

	return(args);
}

// Parse a player name, userid, wonid, and team,
//    or a Team name,
//    or "World".
//
// Returns an allocated event_player_t struct, which should be freed by the
// main thread after it's no longer needed.  But note that the strings
// contained in the struct refer to the "buf" in the parent event_args_t
// and should _not_ be freed directly; instead, the buf itself should be
// freed when the event_args_t struct is freed by the main thread.
event_player_t * DLLINTERNAL parse_player(char *start, int *retlen) {
	char *cp, *begin, *end;
	int len;
	event_player_t *pl;

	// Create and init the event_player struct
	pl=(event_player_t *) calloc(1, sizeof(event_player_t));
	if(!pl) 
		return(NULL);
	memset(pl, 0, sizeof(event_player_t));

	cp=start;
	*retlen=0;
	cp+=strspn(cp, " ");

	// ... World ...
	if(strnmatch(cp, "World", 5)) {
		// store team "World"
		begin=cp;
		end=begin+5;
		len=end-begin;

		*end='\0';
		pl->team=begin;
		*retlen=end-start+strspn(end, " ");
		return(pl);
	}
	// ... Team "TERRORIST" ...
	else if(strnmatch(cp, "Team ", 5)) {
		// find team
		begin=strchr(cp+5, '"')+1;
		end=strchr(begin, '"');
		len=end-begin;
		if(len <= 0)
			return(pl);

		// store team
		*end='\0';
		pl->team=begin;
		*retlen=end-start+strspn(end, " ");
		return(pl);
	}
	// ... "Joe<15><785><TERRORIST>" ...
	else if(cp[0] == '"') {
		// find player name
		begin=cp+1;
		end=strchr(begin, '<');
		len=end-begin;
		if(len <= 0) {
			*retlen=begin-1-start;
			return(pl);
		}
		// store player name
		*end='\0';
		pl->name=begin;

		// find userid
		begin=end+1;
		end=strchr(begin, '>');
		len=end-begin;
		if(len <= 0) {
			*retlen=begin-1-start;
			return(pl);
		}
		// store userid
		*end='\0';
		pl->userid=atoi(begin);

		// find wonid
		begin=strchr(end+1, '<')+1;
		end=strchr(begin, '>');
		len=end-begin;
		if(len <= 0) {
			*retlen=begin-1-start;
			return(pl);
		}
		// store wonid
		*end='\0';
		pl->wonid=atoi(begin);

		// find team
		begin=strchr(end+1, '<')+1;
		end=strchr(begin, '>');
		len=end-begin;
		if(len <= 0) {
			*retlen=begin-1-start;
			return(pl);
		}
		// store team
		*end='\0';
		pl->team=begin;

		*retlen=end-start+strspn(end, " ");
		return(pl);
	}
	else {
		// No player strings found, so we go ahead and free the struct we
		// malloc'd at the beginning.
		free(pl);
		return(NULL);
	}
}

// Parse a quoted string.
char * DLLINTERNAL parse_quoted(char *start, int *retlen) {
	char *cp, *begin, *end;
	int len;

	// ... "something" ...
	cp=start;
	*retlen=0;
	cp+=strspn(cp, " ");

	begin=strchr(cp, '"')+1;
	end=strchr(begin, '"');
	len=end-begin;
	if(len <= 0)
		return(NULL);
	*end='\0';
	*retlen=end-start+strspn(end, " ");
	return(begin);
}

#endif /* UNFINISHED */
