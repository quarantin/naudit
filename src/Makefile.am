AM_CFLAGS = -Wall -Wextra -std=c99 -pedantic

bin_PROGRAMS = naudit
naudit_SOURCES = naudit.h naudit.c config.h config.c editor.h editor.c util.h util.c find.h find.c map.h map.c vector.h vector.c
naudit_LDADD = $(CDK_LIBS) $(CURSES_LIBS)
naudit_LDFLAGS = $(all_libraties)
AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

