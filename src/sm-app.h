#ifndef __SM_APP_H
#define __SM_APP_H

#include <gtk/gtk.h>
#include "sm-switch.h"

#define SM_APP_TYPE (sm_app_get_type())
#define SM_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SM_APP_TYPE, SmApp))


typedef struct _SmApp       SmApp;
typedef struct _SmAppClass  SmAppClass;


GType        sm_app_get_type();
SmApp*       sm_app_new();
gint         sm_app_find_card(const gchar* prefix);
const gchar* sm_app_open_mixer(SmApp *app, int card_number);
GList*       sm_app_get_channels(SmApp *app);
GList*       sm_app_get_input_sources(SmApp *app);
GList*       sm_app_get_input_switches(SmApp *app);
SmSwitch*    sm_app_get_clock_source(SmApp *app);
SmSwitch*    sm_app_get_sync_status(SmApp *app);

#endif /* __SM_APP_H */
