// vi: set ts=4 sw=4 :
// vim: set tw=75 :

#ifdef UNFINISHED

// tqueue.h - template classes for Queue and QItem

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

#ifndef TQUEUE_H
#define TQUEUE_H

#define MAX_QUEUE_SIZE	50

#include "osdep.h"			// MUTEX_T, etc
#include "new_baseclass.h"

// Forward declarations.
template<class qdata_t> class Queue;

// Template for Queue.
template<class qdata_t>
class Queue : public class_metamod_new {
	private:
	// private copy/assign constructors:
		Queue(const Queue &src);
		void operator=(const Queue &src);
	protected:
	// structs:
		class QItem : public class_metamod_new {
			private:
			// private copy/assign constructors:
				QItem(const QItem &src);
				void operator=(const QItem &src);
			public:
				qdata_t *data;
				QItem *next;
				QItem(void) :data(NULL), next(NULL) { };
				QItem(qdata_t *dnew) :data(dnew), next(NULL) { };
		};
	// data:
		int size;
		int max_size;
		QItem *front;
		QItem *end;
		MUTEX_T mx_queue;
		COND_T cv_push;
		COND_T cv_pop;
	//functions
		int DLLINTERNAL MXlock(void) { return(MUTEX_LOCK(&mx_queue)); };
		int DLLINTERNAL MXunlock(void) { return(MUTEX_UNLOCK(&mx_queue)); };
	public:
	// constructor:
		Queue(void) DLLINTERNAL;
		Queue(int qmaxsize) DLLINTERNAL;
	// functions:
		void DLLINTERNAL push(qdata_t *qadd);
		qdata_t * DLLINTERNAL pop(void);
};


///// Template Queue:

// Queue constructor (default).
template<class qdata_t> Queue<qdata_t>::Queue(void)
	: size(0), max_size(MAX_QUEUE_SIZE), front(NULL), end(NULL), mx_queue(), 
	  cv_push(), cv_pop()
{
	MUTEX_INIT(&mx_queue);
	COND_INIT(&cv_push);
	COND_INIT(&cv_pop);
}

// Queue constructor.
template<class qdata_t> Queue<qdata_t>::Queue(int qmaxsize)
	: size(0), max_size(qmaxsize), front(NULL), end(NULL), mx_queue(), 
	  cv_push(), cv_pop()
{
	MUTEX_INIT(&mx_queue);
	COND_INIT(&cv_push);
	COND_INIT(&cv_pop);
}

// Push onto the queue (at end).
template<class qdata_t> void Queue<qdata_t>::push(qdata_t *qadd) {
	QItem *qnew;
	MXlock();
	while(likely(size >= max_size))
		COND_WAIT(&cv_push, &mx_queue);
	qnew = new QItem(qadd);
	end->next = qnew;
	end=qnew;
	size++;
	MXunlock();
}

// Pop from queue (from front).  Wait for an item to actually be available
// on the queue (block until there's something there).
template<class qdata_t> qdata_t* Queue<qdata_t>::pop(void) {
	QItem *qtmp;
	qdata_t *ret;
	MXlock();
	while(likely(!size))
		COND_WAIT(&cv_pop, &mx_queue);
	qtmp=front;
	front=qtmp->next;
	size--;
	ret=qtmp->data;
	delete qtmp;
	MXunlock();
	return(ret);
}

#endif /* TQUEUE_H */

#endif /* UNFINISHED */
