// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// log_mega.cpp - logging routines

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

#include <stdio.h>		// vsnprintf, etc
#include <stdarg.h>		// va_start, etc

#include <extdll.h>				// always
#include "enginecallbacks.h"		// ALERT, etc

#include "sdk_util.h"			// SERVER_PRINT, etc
#include "log_meta.h"			// me
#include "osdep.h"				// win32 vsnprintf, etc
#include "support_meta.h"		// MAX

static char __meta_debug_name[16] = "meta_debug";
static char __meta_debug_value[4] = "0";
cvar_t meta_debug = {__meta_debug_name, __meta_debug_value, FCVAR_EXTDLL, 0, NULL};

int meta_debug_value = 0; //meta_debug_value is converted from float(meta_debug.value) to int on every frame

enum MLOG_SERVICE {
	mlsDEV = 1,
	mlsIWEL
};

static void buffered_ALERT(MLOG_SERVICE service, ALERT_TYPE atype, const char *fmt, ...);
static void vbuffered_ALERT(MLOG_SERVICE service, ALERT_TYPE atype, const char *fmt, va_list ap);

// Print to console.
void DLLINTERNAL META_CONS(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(len < sizeof(buf)-2) {	// -1 null, -1 for newline
		buf[len+0] = '\n';
		buf[len+1] = 0;
	}
	else
		buf[len-1] = '\n';

	SERVER_PRINT(buf);
}

// Log developer-level messages (obsoleted).
void DLLINTERNAL META_DEV(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	
	if (NULL != g_engfuncs.pfnAlertMessage && (int)CVAR_GET_FLOAT("developer") == 0)
			return;
	
	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	buffered_ALERT(mlsDEV, at_logged, "[META] dev: %s", buf);
}

// Log infos.
void DLLINTERNAL META_INFO(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	
	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	buffered_ALERT(mlsIWEL, at_logged, "[META] INFO: %s", buf);
}

// Log warnings.
void DLLINTERNAL META_WARNING(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	
	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	buffered_ALERT(mlsIWEL, at_logged, "[META] WARNING: %s", buf);
}

// Log errors.
void DLLINTERNAL META_ERROR(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	
	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	buffered_ALERT(mlsIWEL, at_logged, "[META] ERROR: %s", buf);
}

// Normal log messages.
void DLLINTERNAL META_LOG(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	
	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	buffered_ALERT(mlsIWEL, at_logged, "[META] %s", buf);
}

// Print to client.
void DLLINTERNAL META_CLIENT(edict_t *pEntity, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_CLIENTMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(len < sizeof(buf)-2)	{	// -1 null, -1 for newline
		buf[len+0] = '\n';
		buf[len+1] = 0;
	}
	else
		buf[len-1] = '\n';

	CLIENT_PRINTF(pEntity, print_console, buf);
}

#ifndef __BUILD_FAST_METAMOD__

int __internal_meta_do_debug_level = -1;

// meta debug log messages.
void DLLINTERNAL META_DO_DEBUG(const char *fmt, ...) {
	char buf[MAX_CLIENTMSG_LEN];
	va_list ap;
	
	va_start(ap, fmt);
	safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	
	buffered_ALERT(mlsIWEL, at_logged, "[META] (debug:%d) %s", __internal_meta_do_debug_level, buf);
}

#endif /*!__BUILD_FAST_METAMOD__*/

class BufferedMessage : public class_metamod_new {
public:
	MLOG_SERVICE service;
	ALERT_TYPE atype;
	char buf[MAX_LOGMSG_LEN];
	BufferedMessage *next;
};

static BufferedMessage *messageQueueStart = NULL;
static BufferedMessage *messageQueueEnd   = NULL;

static void buffered_ALERT(MLOG_SERVICE service, ALERT_TYPE atype, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vbuffered_ALERT(service, atype, fmt, ap);
	va_end(ap);
}

static void vbuffered_ALERT(MLOG_SERVICE service, ALERT_TYPE atype, const char *fmt, va_list ap) {
	char buf[MAX_LOGMSG_LEN];
	BufferedMessage *msg;

	if (NULL != g_engfuncs.pfnAlertMessage) {
		safevoid_vsnprintf(buf, sizeof(buf), fmt, ap);
		ALERT(atype, "%s\n", buf);
		return;
	}

	// Engine AlertMessage function not available. Buffer message.
	msg = new BufferedMessage;
	if (NULL == msg) {
		// though luck, gonna lose this message
		return;
	}
	
	msg->service = service;
	msg->atype = atype;
	safevoid_vsnprintf(msg->buf, sizeof(msg->buf), fmt, ap);
	msg->next = NULL;

	if (NULL == messageQueueEnd) {
        messageQueueEnd = msg;
		messageQueueStart = msg;
    }
	else {
		messageQueueEnd->next = msg;
        messageQueueEnd = msg;
    }
} 


// Flushes the message queue, printing messages to the respective
// service. This function doesn't check anymore if the g_engfuncs
// jumptable is set. Don't call it if it isn't set.
void DLLINTERNAL flush_ALERT_buffer(void) {
	BufferedMessage *msg = messageQueueStart;
	
	if(msg != NULL)
	{
		int dev = (int)CVAR_GET_FLOAT("developer");
		
		do {
			if(msg->service != mlsDEV || dev != 0) 
				ALERT(msg->atype, "b>%s\n", msg->buf);
			
			messageQueueStart = messageQueueStart->next;
			delete msg;
			msg = messageQueueStart;
		}
		while(NULL != msg);
	}
	
	messageQueueStart = messageQueueEnd = NULL;
}
