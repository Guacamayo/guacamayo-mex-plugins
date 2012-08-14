plugin_datadir = $(pkgdatadir)/plugins
pluginsdir = $(mexpluginsdir)
plugins_LTLIBRARIES =

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =

AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	$(PLUGINS_CFLAGS)	\
	$(NULL)

AM_LDFLAGS  = -no-undefined -module -avoid-version
_libadd     = $(PLUGINS_LIBS)

plugins_LTLIBRARIES += guaca-system.la

guaca_system_la_SOURCES =	\
	system/guaca-system.c	\
	system/guaca-system.h	\
	$(NULL)

guaca_system_la_CFLAGS  = -DTHEMEDIR=\"$(pkgdatadir)/\"
guaca_system_la_LIBADD  = $(_libadd)
