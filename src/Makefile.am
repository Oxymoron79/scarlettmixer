bin_PROGRAMS=scarlettmixer

scarlettmixer_CFLAGS=@ALSA_CFLAGS@ @GTK_CFLAGS@
scarlettmixer_LDADD=@ALSA_LIBS@ @GTK_LIBS@ -lm
scarlettmixer_SOURCES=							\
	scarlettmixer.c								\
	sm-channel.c sm-channel.h					\
	sm-source.c sm-sourcel.h					\
	scarlettmixerapp.c scarlettmixerapp.h		\
	scarlettmixerappwin.c scarlettmixerappwin.h	\
	scarlettmixerstrip.c scarlettmixerstrip.h	\
	scarlettmixerresources.c

BUILT_SOURCES = scarlettmixerresources.c

scarlettmixerresources.c: scarlettmixerapp.gresource.xml scarlettmixerappwin.ui scarlettmixerstrip.ui
	$(AM_V_GEN) glib-compile-resources $(srcdir)/scarlettmixerapp.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source