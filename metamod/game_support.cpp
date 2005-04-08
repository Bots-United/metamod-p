// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// game_support.cpp - info to recognize different HL mod "games"

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
#include <fcntl.h>          // open, write

#include <extdll.h>			// always

#include "game_support.h"	// me
#include "log_meta.h"		// META_LOG, etc
#include "types_meta.h"		// mBOOL
#include "osdep.h"			// win32 snprintf, etc
#include "game_autodetect.h"	// autodetect_gamedll
#include "support_meta.h"	// MIN

// Adapted from adminmod h_export.cpp:
//! this structure contains a list of supported mods and their dlls names
//! To add support for another mod add an entry here, and add all the 
//! exported entities to link_func.cpp
const game_modlist_t known_games = {
	// name/gamedir	 linux_so			win_dll			desc
	//
	// Previously enumerated in this sourcefile, the list is now kept in a
	// separate file, generated based on game information stored in a 
	// convenient db.
	//
#include "games.h"
	// End of list terminator:
	{NULL, NULL, NULL, NULL, NULL}
};

// Find a modinfo corresponding to the given game name.
const game_modinfo_t *lookup_game(const char *name) {
	const game_modinfo_t *imod;
	int i;
	for(i=0; likely(known_games[i].name); i++) {
		imod=&known_games[i];
		if(unlikely(strcasematch(imod->name, name)))
			return(imod);
	}
	// no match found
	return(NULL);
}

// Installs gamedll from Steam cache
mBOOL install_gamedll(char *from, const char *to) {
	int length_in;
	int length_out;
	
	if(unlikely(!from)) 
		return mFALSE;
	if(unlikely(!to))
		to = from;
	
	byte* cachefile = LOAD_FILE_FOR_ME(from, &length_in);
	
	// If the file seems to exist in the cache.
	if(likely(cachefile)) {
		int fd=open(to, O_WRONLY|O_CREAT|O_EXCL|O_BINARY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
		
		if(unlikely(fd < 0)) {
			META_DEBUG(3, ("Installing gamedll from cache: Failed to create file %s: %s", to, strerror(errno)) );
			FREE_FILE(cachefile);
			return(mFALSE);
		}
		
		length_out=write(fd, cachefile, length_in);
		FREE_FILE(cachefile);
		close(fd);
		
		// Writing the file was not successfull
		if(unlikely(length_out != length_in)) {
			META_DEBUG(3,("Installing gamedll from chache: Failed to write all %d bytes to file, only %d written: %s", length_in, length_out, strerror(errno)));
			// Let's not leave a mess but clean up nicely.
			if(likely(length_out >= 0))
				unlink(to);
			
			return(mFALSE);
		}
		
		META_LOG("Installed gamedll %s from cache.", to);
	} else {
		META_DEBUG(3, ("Failed to install gamedll from cache: file %s not found in cache.", from) );
		return(mFALSE);
	}

	return(mTRUE);
}

// Set all the fields in the gamedll struct, - based either on an entry in
// known_games matching the current gamedir, or on one specified manually 
// by the server admin. This is called by setup_gamedll with dlls directory 
// name.
//
// meta_errno values:
//  - ME_NOTFOUND	couldn't recognize game
mBOOL setup_gamedll_in_dlls_dir(const game_modinfo_t *known, const char * dlls_dir, gamedll_t *gamedll) {
#ifdef __x86_64__
	static char fixname_amd64[256]; // pointer is given outside function
#endif
	static char override_desc_buf[256]; // pointer is given outside function
	static char autodetect_desc_buf[256]; // pointer is given outside function
	char install_path[256];
	char *cp;
	const char *autofn = 0, *knownfn=0;
	int override=0;
	
	// Check for old-style "metagame.ini" file and complain.
	if(unlikely(valid_gamedir_file(OLD_GAMEDLL_TXT)))
		META_WARNING("File '%s' is no longer supported; instead, specify override gamedll in %s or with '+localinfo mm_gamedll <dllfile>'", OLD_GAMEDLL_TXT, CONFIG_INI);
	// First, look for a known game, based on gamedir.
	if(likely(known)) {
#ifdef _WIN32
		knownfn=known->win_dll;
#elif defined(linux)
		knownfn=known->linux_so;
	#ifdef __x86_64__
		//AMD64: convert _i386.so to _amd64.so
		if(likely((cp = strstr(knownfn, "_i386.so"))) ||
		   unlikely((cp = strstr(knownfn, "_i486.so"))) ||
		   unlikely((cp = strstr(knownfn, "_i586.so"))) ||
		   unlikely((cp = strstr(knownfn, "_i686.so")))) {
		   	//make sure that it's the ending that has "_iX86.so"
		   	if(likely(cp[strlen("_i386.so")] == 0)) {
				STRNCPY(fixname_amd64, known->linux_so, 
					MIN(((size_t)cp - (size_t)knownfn) + 1, 
					sizeof(fixname_amd64)));
				strncat(fixname_amd64, "_amd64.so", sizeof(fixname_amd64));
				
				knownfn=fixname_amd64;
			}
		}
	#endif /*__x86_64__*/
#else
#error "OS unrecognized"
#endif /* _WIN32 */
		// Do this before autodetecting gamedll from "dlls/*.dll"
		if(likely(!Config->gamedll)) {
			safe_snprintf(gamedll->pathname, sizeof(gamedll->pathname), "%s/%s", 
				dlls_dir, knownfn);
			// Check if the gamedll file exists. If not, try to install it from
			// the cache.
			if(unlikely(!valid_gamedir_file(gamedll->pathname))) {
				safe_snprintf(install_path, sizeof(install_path), "%s/%s/%s", 
						gamedll->gamedir, dlls_dir, knownfn);
				install_gamedll(gamedll->pathname, install_path);
				
				// Check if the gamedll file exists after trying install it.
				// Only check for special dlls directory if current dlls_dir is 'dlls'.
				if(unlikely(!valid_gamedir_file(gamedll->pathname)) && likely(!strcmp(dlls_dir, "dlls"))) {
					// Check if Mod has special non-standard directory for gamedll
#ifdef linux
					if(likely(known->linux_dir) && unlikely(known->linux_dir[0])) {
						return(setup_gamedll_in_dlls_dir(known, known->linux_dir, gamedll));
					}
#elif defined(_WIN32)
					if(likely(known->win_dir) && unlikely(known->win_dir[0])) {
						return(setup_gamedll_in_dlls_dir(known, known->win_dir, gamedll));
					}
#else
#error "OS unrecognized"
#endif /*linux*/
				}
			}
		}
	}
	
	// Then, autodetect gamedlls in "gamedir/dlls/"
	// autodetect_gamedll returns 0 if knownfn exists and is valid gamedll.
	if(likely(Config->autodetect) && likely((autofn=autodetect_gamedll(gamedll, knownfn)))) {
		// If knownfn is set and autodetect_gamedll returns non-null
		// then knownfn doesn't exists and we should use autodetected
		// dll instead.
		if(unlikely(knownfn)) {
			// Whine loud about fact that known-list dll doesn't exists!
			//META_LOG(plapla);
			knownfn = autofn;
		}
	}
	
	// Neither override nor known-list nor auto-detect found a gamedll.
	if(unlikely(!known) && likely(!Config->gamedll) && unlikely(!autofn))
		RETURN_ERRNO(mFALSE, ME_NOTFOUND);
	
	// Use override-dll if specified.
	if(unlikely(Config->gamedll)) {
		STRNCPY(gamedll->pathname, Config->gamedll, 
				sizeof(gamedll->pathname));
		override=1;

		// If the path is relative, the gamedll file will be missing and
		// it might be found in the cache file.
		if(likely(!is_absolute_path(gamedll->pathname))) {
			safe_snprintf(install_path, sizeof(install_path),
					"%s/%s", gamedll->gamedir, gamedll->pathname);
			// If we could successfully install the gamedll from the cache we
			// rectify the pathname to be a full pathname.
			if(likely(install_gamedll(gamedll->pathname, install_path)))
				STRNCPY(gamedll->pathname, install_path, sizeof(gamedll->pathname));
		}
	}
	// Else use Known-list dll.
	else if(likely(known)) {
		safe_snprintf(gamedll->pathname, sizeof(gamedll->pathname), "%s/%s/%s", 
				gamedll->gamedir, dlls_dir, knownfn);
	}
	// Else use Autodetect dll.
	else {
		safe_snprintf(gamedll->pathname, sizeof(gamedll->pathname), "%s/%s/%s", 
				gamedll->gamedir, dlls_dir, autofn);
	}

	// get filename from pathname
	cp=strrchr(gamedll->pathname, '/');
	if(likely(cp))
		cp++;
	else
		cp=gamedll->pathname;
	gamedll->file=cp;

	// If found, store also the supposed "real" dll path based on the
	// gamedir, in case it differs from the "override" dll path.
	if(likely(known) && unlikely(override))
		safe_snprintf(gamedll->real_pathname, sizeof(gamedll->real_pathname),
				"%s/%s/%s", gamedll->gamedir, dlls_dir, knownfn);
	else if(likely(known) && unlikely(autofn))
		safe_snprintf(gamedll->real_pathname, sizeof(gamedll->real_pathname),
				"%s/%s/%s", gamedll->gamedir, dlls_dir, knownfn);
	else // !known or (!override and !autofn)
		STRNCPY(gamedll->real_pathname, gamedll->pathname, 
				sizeof(gamedll->real_pathname));
	
	if(unlikely(override)) {
		// generate a desc
		safe_snprintf(override_desc_buf, sizeof(override_desc_buf), "%s (override)", gamedll->file);
		gamedll->desc=override_desc_buf;
		// log result
		META_LOG("Overriding game '%s' with dllfile '%s'", gamedll->name, gamedll->file);
	}
	else if(likely(known) && unlikely(autofn)) {
		// dll in known-list doesn't exists but we found new one with autodetect.
		
		// generate a desc
		safe_snprintf(autodetect_desc_buf, sizeof(autodetect_desc_buf), "%s (autodetect-override)", gamedll->file);
		gamedll->desc=autodetect_desc_buf;
		META_LOG("Recognized game '%s'; Autodetection override; using dllfile '%s'", gamedll->name, gamedll->file);
	}
	else if(unlikely(autofn)) {
		// generate a desc
		safe_snprintf(autodetect_desc_buf, sizeof(autodetect_desc_buf), "%s (autodetect)", gamedll->file);
		gamedll->desc=autodetect_desc_buf;
		META_LOG("Autodetected game '%s'; using dllfile '%s'", gamedll->name, gamedll->file);
	}
	else if(likely(known)) {
		gamedll->desc=known->desc;
		META_LOG("Recognized game '%s'; using dllfile '%s'", gamedll->name, gamedll->file);
	}
	return(mTRUE);
}

// Set all the fields in the gamedll struct, - based either on an entry in
// known_games matching the current gamedir, or on one specified manually 
// by the server admin. 
mBOOL setup_gamedll(gamedll_t *gamedll) {
	return(setup_gamedll_in_dlls_dir(lookup_game(gamedll->name), "dlls", gamedll));
}
