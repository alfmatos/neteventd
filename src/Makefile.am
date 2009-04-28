bin_PROGRAMS = neteventd
noinst_LIBRARIES = libnetevent.a

libnetevent_a_SOURCES = netlink.c events.c

neteventd_SOURCES = neteventd.c
neteventd_LDADD = libnetevent.a