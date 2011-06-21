INCLUDES = $(netevent_include_paths)

lib_LTLIBRARIES = libnetevent.la
libnetevent_la_SOURCES = rtnl.c console.c events.c iw.c nl80211.c utils.c

bin_PROGRAMS = neteventd
neteventd_SOURCES = neteventd.c
neteventd_LDADD = libnetevent.la
