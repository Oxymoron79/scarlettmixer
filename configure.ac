dnl Process this file with autoconf to produce a configure script.

dnl Initialization
AC_PREREQ(2.59)
AC_INIT([Scarlett Mixer],
        [0.1.0],
        [http://www.github.com/Oxymoron79/scarlettmixer/issues],
        [scarlettmixer],
        [http://www.github.com/Oxymoron79/scarlettmixer])
AC_CONFIG_SRCDIR([src/scarlettmixer.c])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.11 -Wall foreign])

AX_IS_RELEASE([git-directory])
AX_CHECK_ENABLE_DEBUG()
AM_CONDITIONAL(COMPILE_GSCHEMAS, test $ax_enable_debug = yes)

dnl Required tools
AC_PROG_CC
PKG_PROG_PKG_CONFIG
GLIB_GSETTINGS

dnl Libraries
AC_CHECK_LIBM
AC_SUBST(LIBM)
PKG_CHECK_MODULES(ALSA, alsa)
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.12 gobject-2.0 json-glib-1.0 >= 1.0)

dnl Output
AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
