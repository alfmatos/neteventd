lib_LTLIBRARIES = libnetevent.la
libnetevent_la_SOURCES = netlink.c events.c

bin_PROGRAMS = neteventd
neteventd_SOURCES = neteventd.c
neteventd_LDADD = libnetevent.la
