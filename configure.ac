AC_PREREQ(2.61)

AC_INIT([neteventd], [0.2], [alfredo.matos@digitalself.org])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_INSTALL

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/socket.h sys/time.h])

AC_HEADER_TIME
AC_STRUCT_TM

AC_CHECK_FUNCS([gettimeofday memset socket strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
