m4_define([plugins_major], [0])
m4_define([plugins_minor], [1])
m4_define([plugins_micro], [0])

m4_define([plugins_version], [plugins_major.plugins_minor.plugins_micro])

AC_PREREQ(2.53)
AC_INIT([guacamayo-mex-plugins], [plugins_version], [http://guacamayo-project.org])
AC_CONFIG_SRCDIR([src/system/guaca-system.c])
AC_CONFIG_MACRO_DIR([build])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-xz tar-ustar])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

MAJOR_VERSION=plugins_major
MINOR_VERSION=plugins_minor
MICRO_VERSION=plugins_micro
VERSION=plugins_version
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(VERSION)

# i18n
AS_ALL_LINGUAS
IT_PROG_INTLTOOL([0.40.0], [no-xml])

GETTEXT_PACKAGE=media-explorer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Gettext domain name])
AM_GLIB_GNU_GETTEXT
GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

# check for programs
AC_PROG_CC
AM_PROG_CC_C_O

PKG_PROG_PKG_CONFIG

# check for headers
AC_HEADER_STDC

modules="mex-0.2"

PKG_CHECK_MODULES(PLUGINS, "$modules")

mexpluginsdir=`$PKG_CONFIG --variable=pluginsdir mex-0.2`
AC_SUBST(mexpluginsdir)

mex_api_major=`$PKG_CONFIG --variable=apiversion_major mex-0.2`
mex_api_minor=`$PKG_CONFIG --variable=apiversion_minor mex-0.2`
AC_DEFINE_UNQUOTED([MEX_API_MAJOR], [$mex_api_major], ["major number of the API version"])
AC_DEFINE_UNQUOTED([MEX_API_MINOR], [$mex_api_minor], ["minor number of the API version"])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  data/Makefile
  po/Makefile.in
])

AC_OUTPUT
