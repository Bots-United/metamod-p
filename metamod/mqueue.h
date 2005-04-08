// vi: set ts=4 sw=4 :
// vim: set tw=75 :

#ifdef UNFINISHED

// mqueue.h - class and types for queues

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

#ifndef MQUEUE_H
#define MQUEUE_H

#include "plinfo.h"			// plid_t, etc
#include "tqueue.h"			// templates Queue<>, QItem<>, etc
#include "mhook.h"			// event_args_t, etc
#include "types_meta.h"		// mBOOL
#include "osdep.h"			// MUTEX_T, etc
#include "new_baseclass.h"

// Our queue types.
typedef Queue<const char>	MLogmsgQueue;

typedef struct {
	MHook *hook;
	event_args_t *evargs;
	const char *logline;
} func_item_t;

class MFuncQueue : public class_metamod_new, Queue<func_item_t> {
	public:
	// renamed constructors:
		MFuncQueue(void) :Queue<func_item_t>() { };
		MFuncQueue(int qmaxsize) :Queue<func_item_t>(qmaxsize) { };
	// added functions:
		void remove(plid_t plid);
};

#endif /* MQUEUE_H */

#endif /* UNFINISHED */
