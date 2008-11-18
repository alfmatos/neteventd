AC_PREREQ(2.59)

AC_INIT([neteventd], [0.3.2], [alfredo.matos@digitalself.org])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_INSTALL

AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h stdlib.h string.h sys/socket.h sys/time.h])

AC_CHECK_HEADER(iwlib.h,, AC_MSG_ERROR([iwlib is required but was not found. Installing wireless-tools-dev should fix this.]))

IWLIB=-liw
AC_SUBST(IWLIB)

LDFLAGS="$LDFLAGS $IWLIB"

AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME
AC_STRUCT_TM

AC_CHECK_FUNCS([gettimeofday memset socket strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
