#ifndef __SCARLETTMIXERSTRIP_H
#define __SCARLETTMIXERSTRIP_H

#include <gtk/gtk.h>

#include "scarlettmixer.h"
#include "sm-channel.h"


#define SCARLETTMIXER_STRIP_TYPE (sm_strip_get_type ())
#define SCARLETTMIXER_STRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCARLETTMIXER_STRIP_TYPE, ScarlettMixerStrip))


typedef struct _ScarlettMixerStrip         ScarlettMixerStrip;
typedef struct _ScarlettMixerStripClass    ScarlettMixerStripClass;


GType               sm_strip_get_type     (void);
ScarlettMixerStrip *sm_strip_new          (SmChannel *channel);

#endif /* __SCARLETTMIXERSTRIP_H */
