
AC_PREREQ([2.61])
AC_INIT([naudit], [0.1])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
CPPFLAGS=-I/usr/include/cdk

AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([ncurses], [endwin])
AC_CHECK_LIB([cdkw], [newCDKSelection], [:], [:], [-lncurses])

CDK_LIBS=-lcdkw
AC_SUBST(CDK_LIBS)

CURSES_LIBS="-lncurses -ltinfo"
AC_SUBST(CURSES_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h getopt.h libgen.h string.h strings.h sys/types.h sys/wait.h curses.h cdk.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset mkdir strrchr])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([shell/Makefile])
AC_OUTPUT
