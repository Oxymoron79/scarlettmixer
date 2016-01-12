dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(scarlettmixer, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

PKG_CHECK_MODULES(ALSA, alsa)
PKG_CHECK_MODULES(GTK, gtk+-3.0 gobject-2.0)

AC_PROG_CC

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

