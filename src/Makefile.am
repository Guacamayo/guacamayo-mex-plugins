plugin_datadir = $(pkgdatadir)/plugins
pluginsdir = $(mexpluginsdir)
plugins_LTLIBRARIES =

bin_PROGRAMS =

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =

AM_CPPFLAGS = -I$(top_srcdir)

#
# System settings plugin
#
plugins_LTLIBRARIES += guaca-system.la

guaca_system_la_SOURCES =	\
	system/guaca-system.c	\
	system/guaca-system.h	\
	$(NULL)

guaca_system_la_CFLAGS = $(PLUGINS_CFLAGS)		\
			 -DTHEMEDIR=\"$(pkgdatadir)/\"	\
			 $(NULL)

guaca_system_la_LDFLAGS = -no-undefined -module -avoid-version
guaca_system_la_LIBADD  = $(PLUGINS_LIBS)

bin_PROGRAMS += guacamayo-hostname
guacamayo_hostname_SOURCES = system/guaca-hostname.c

#
# Clock settings plugin
#
plugins_LTLIBRARIES += guaca-clock.la

guaca_clock_la_SOURCES =	\
	clock/guaca-clock.c	\
	clock/guaca-clock.h	\
	$(NULL)

guaca_clock_la_CFLAGS = $(PLUGINS_CFLAGS)		\
			 -DTHEMEDIR=\"$(pkgdatadir)/\"	\
			 $(NULL)

guaca_clock_la_LDFLAGS = -no-undefined -module -avoid-version
guaca_clock_la_LIBADD  = $(PLUGINS_LIBS)

bin_PROGRAMS += guacamayo-timezone
guacamayo_timezone_SOURCES = clock/guaca-timezone.c

# guacamayo-hostname needs to be installed suid
install-exec-hook:
	chmod 4755 $(DESTDIR)$(bindir)/guacamayo-hostname
	chmod 4755 $(DESTDIR)$(bindir)/guacamayo-timezone
