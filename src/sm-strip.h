#ifndef __SM_STRIP_H
#define __SM_STRIP_H

#include <gtk/gtk.h>

#include "sm-channel.h"


#define SM_STRIP_TYPE (sm_strip_get_type ())
#define SM_STRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SM_STRIP_TYPE, SmStrip))


typedef struct _SmStrip      SmStrip;
typedef struct _SmStripClass SmStripClass;


GType    sm_strip_get_type(void);
SmStrip *sm_strip_new(SmChannel *channel);

#endif /* __SM_STRIP_H */
