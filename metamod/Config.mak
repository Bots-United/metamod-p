MODNAME = metamod

#__METAMOD_BUILD__ for our special eiface.h
EXTRA_CFLAGS = -D__METAMOD_BUILD__ -DNO_META_NEW_HANDLER
#-D__META_DEBUG_VALUE__CACHE_AS_INT__ // <- Use this for i586 and older.
#-DMETA_PERFMON

SRCFILES = api_hook.cpp api_info.cpp commands_meta.cpp conf_meta.cpp \
	dllapi.cpp engine_api.cpp game_support.cpp game_autodetect.cpp \
	h_export.cpp linkgame.cpp linkplug.cpp log_meta.cpp metamod.cpp \
	mhook.cpp mlist.cpp mplugin.cpp mqueue.cpp \
	mreg.cpp mutil.cpp osdep.cpp osdep_p.cpp reg_support.cpp \
	sdk_util.cpp studioapi.cpp support_meta.cpp thread_logparse.cpp \
	vdate.cpp

INFOFILES = info_name.h vers_meta.h
RESFILE = res_meta.rc

ifeq "$(OS)" "linux"
	SRCFILES+=osdep_linkent_linux.cpp
	EXTRA_LINK=
else
	SRCFILES+=osdep_linkent_win32.cpp
	EXTRA_LINK=-Xlinker --script -Xlinker i386pe.merge
endif

#STLFILES = mreg.cpp
