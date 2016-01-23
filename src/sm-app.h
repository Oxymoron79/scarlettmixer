#ifndef __SCARLETTMIXERAPP_H
#define __SCARLETTMIXERAPP_H

#include <gtk/gtk.h>

#define SCARLETTMIXER_APP_TYPE (sm_app_get_type())
#define SCARLETTMIXER_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SCARLETTMIXER_APP_TYPE, ScarlettMixerApp))


typedef struct _ScarlettMixerApp       ScarlettMixerApp;
typedef struct _ScarlettMixerAppClass  ScarlettMixerAppClass;


GType             sm_app_get_type();
ScarlettMixerApp* sm_app_new();
gint              sm_app_find_card(const gchar* prefix);
const gchar*      sm_app_open_mixer(ScarlettMixerApp *app, int card_number);
GList*            sm_app_get_channels(ScarlettMixerApp *app);
GList*            sm_app_get_input_sources(ScarlettMixerApp *app);

#endif /* __SCARLETTMIXERAPP_H */
