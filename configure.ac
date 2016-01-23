dnl Process this file with autoconf to produce a configure script.

dnl Initialization
AC_PREREQ(2.59)
AC_INIT([Scarlett Mixer], [0.1], [martin.roesch79@gmail.com])
AC_CONFIG_SRCDIR([src/scarlettmixer.c])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall foreign])

dnl Required tools
AC_PROG_CC
PKG_PROG_PKG_CONFIG

dnl Libraries
PKG_CHECK_MODULES(ALSA, alsa)
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.12 gobject-2.0)

dnl Output
AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
