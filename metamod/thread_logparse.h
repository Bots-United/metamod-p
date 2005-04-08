// vi: set ts=4 sw=4 :
// vim: set tw=75 :

#ifdef UNFINISHED

// thread_logparse.h - prototypes for thread_logparse functions

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

#ifndef THREAD_LOGPARSE_H
#define THREAD_LOGPARSE_H

#include "mqueue.h"			// Queue template
#include <osdep.h>			// THREAD_T, etc

extern MLogmsgQueue *LogQueue;
extern MFuncQueue *HookQueue;

extern THREAD_T logparse_thread_id;

void startup_logparse_thread(void);
void WINAPI logparse_handler(void);

event_args_t *parse_event_args(const char *logline);
event_player_t *parse_player(char *start, int *retlen);
char *parse_quoted(char *start, int *retlen);

#endif /* THREAD_LOGPARSE_H */

#endif /* UNFINISHED */
