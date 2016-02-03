#ifndef __SM_MIX_STRIP_H
#define __SM_MIX_STRIP_H

#include <gtk/gtk.h>

#include "sm-channel.h"


#define SM_MIX_STRIP_TYPE (sm_mix_strip_get_type())
#define SM_MIX_STRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SM_MIX_STRIP_TYPE, SmMixStrip))


typedef struct _SmMixStrip      SmMixStrip;
typedef struct _SmMixStripClass SmMixStripClass;


GType       sm_mix_strip_get_type(void);
SmMixStrip *sm_mix_strip_new(SmChannel *channel);
gboolean    sm_mix_strip_add_channel(SmMixStrip *strip, SmChannel *channel);
gchar      *sm_mix_strip_get_mix_ids(SmMixStrip *strip);

#endif /* __SM_MIX_STRIP_H */
