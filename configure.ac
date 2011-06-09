AC_PREREQ(2.59)

AC_INIT([neteventd], [0.3.900], [alfredo.matos@caixamagica.pt])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_PROG_LIBTOOL
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

# Header files
AC_SUBST(NETEVENT_PATH, `pwd`)
AC_SUBST(NETEVENT_INCLUDE_PATH, "$NETEVENT_PATH/include")
AC_SUBST(netevent_include_paths, "-I $NETEVENT_INCLUDE_PATH" )

AC_CHECK_FUNCS([gettimeofday memset socket strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
