bin_PROGRAMS=scarlettmixer

scarlettmixer_CFLAGS=@ALSA_CFLAGS@ @GTK_CFLAGS@
scarlettmixer_LDADD=@ALSA_LIBS@ @GTK_LIBS@ @LIBM@
scarlettmixer_SOURCES=				\
	scarlettmixer.c					\
	sm-channel.c sm-channel.h		\
	sm-source.c sm-source.h			\
	sm-switch.c sm-switch.h 		\
	sm-app.c sm-app.h				\
	sm-appwin.c sm-appwin.h			\
	sm-prefs.c sm-prefs.h			\
	sm-strip.c sm-strip.h			\
	sm-mix-strip.c sm-mix-strip.h
nodist_scarlettmixer_SOURCES=	\
	sm-appresources.c

BUILT_SOURCES = sm-appresources.c
GRESOURCES_DEPS =			\
	sm-app.gresource.xml	\
	sm-appwin.ui			\
	sm-strip.ui				\
	sm-mix-strip.ui			\
	sm-appmenu.ui			\
	sm-prefsdialog.ui		\
	sm-strip.css			\
	sm-appwin.css			\
	sm-icon-stereo.svg
sm-appresources.c: $(GRESOURCES_DEPS)
	$(AM_V_GEN) glib-compile-resources $(srcdir)/sm-app.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source

gsettings_SCHEMAS = org.alsa.scarlettmixer.gschema.xml
EXTRA_DIST =			\
	$(GRESOURCES_DEPS)	\
	$(gsettings_SCHEMAS)

@GSETTINGS_RULES@

if COMPILE_GSCHEMAS
BUILT_SOURCES += gschemas.compiled

gschemas.compiled: org.alsa.scarlettmixer.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) --targetdir=$(builddir) $(srcdir)
endif

CLEANFILES = $(BUILT_SOURCES)
