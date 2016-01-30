bin_PROGRAMS=scarlettmixer

scarlettmixer_CFLAGS=@ALSA_CFLAGS@ @GTK_CFLAGS@
scarlettmixer_LDADD=@ALSA_LIBS@ @GTK_LIBS@ -lm
scarlettmixer_SOURCES=			\
	scarlettmixer.c				\
	sm-channel.c sm-channel.h	\
	sm-source.c sm-source.h		\
	sm-switch.c sm-sswitch.h	\
	sm-app.c sm-app.h			\
	sm-appwin.c sm-appwin.h		\
	sm-strip.c sm-strip.h		\
	sm-appresources.c

BUILT_SOURCES = sm-appresources.c

sm-appresources.c: sm-app.gresource.xml \
                   sm-appwin.ui \
                   sm-strip.ui \
                   sm-appmenu.ui \
                   sm-strip.css \
                   sm-appwin.css
	$(AM_V_GEN) glib-compile-resources $(srcdir)/sm-app.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source