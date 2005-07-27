// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// log_mega.cpp - logging routines

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

#include <stdio.h>		// vsnprintf, etc
#include <stdarg.h>		// va_start, etc

#include <extdll.h>				// always
#include <enginecallback.h>		// ALERT, etc

#include "sdk_util.h"			// SERVER_PRINT, etc
#include "log_meta.h"			// me
#include "osdep.h"				// win32 vsnprintf, etc
#include "support_meta.h"		// MAX

cvar_t meta_debug = {"meta_debug", "0", FCVAR_EXTDLL, 0, NULL};

#ifdef __META_DEBUG_VALUE__CACHE_AS_INT__
int meta_debug_value = 0; //meta_debug_value is converted from float(meta_debug.value) to int on every frame
#endif

// Print to console.
void DLLINTERNAL META_CONS(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(likely(len < sizeof(buf)-2)) {	// -1 null, -1 for newline
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
	int dev;

	dev=(int) CVAR_GET_FLOAT("developer");
	if(likely(dev==0)) return;

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[META] dev: %s\n", buf);
}

// Log infos.
void DLLINTERNAL META_INFO(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[META] INFO: %s\n", buf);
}

// Log warnings.
void DLLINTERNAL META_WARNING(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[META] WARNING: %s\n", buf);
}

// Log errors.
void DLLINTERNAL META_ERROR(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[META] ERROR: %s\n", buf);
}

// Normal log messages.
void DLLINTERNAL META_LOG(const char *fmt, ...) {
	va_list ap;
	char buf[MAX_LOGMSG_LEN];

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ALERT(at_logged, "[META] %s\n", buf);
}

// Print to client.
void DLLINTERNAL META_CLIENT(edict_t *pEntity, const char *fmt, ...) {
	va_list ap;
	char buf[MAX_CLIENTMSG_LEN];
	unsigned int len;

	va_start(ap, fmt);
	safe_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(likely(len < sizeof(buf)-2))	{	// -1 null, -1 for newline
		buf[len+0] = '\n';
		buf[len+1] = 0;
	}
	else
		buf[len-1] = '\n';

	CLIENT_PRINTF(pEntity, print_console, buf);
}

#ifndef __BUILD_FAST_METAMOD__

void DLLINTERNAL META_DO_DEBUG(int level, const char *fmt, ...) {
	char meta_debug_str[1024];
	va_list ap;
	
	va_start(ap, fmt);
	safe_vsnprintf(meta_debug_str, sizeof(meta_debug_str), fmt, ap);
	va_end(ap);
	
	ALERT(at_logged, "[META] (debug:%d) %s\n", level, meta_debug_str);
}

#endif /*!__BUILD_FAST_METAMOD__*/
