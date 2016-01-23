#ifndef __SCARLETTMIXERAPPWIN_H
#define __SCARLETTMIXERAPPWIN_H

#include <gtk/gtk.h>
#include "sm-app.h"


#define SCARLETTMIXER_APP_WINDOW_TYPE (sm_app_window_get_type ())
#define SCARLETTMIXER_APP_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCARLETTMIXER_APP_WINDOW_TYPE, ScarlettMixerAppWindow))


typedef struct _ScarlettMixerAppWindow         ScarlettMixerAppWindow;
typedef struct _ScarlettMixerAppWindowClass    ScarlettMixerAppWindowClass;


GType                   sm_app_window_get_type(void);
ScarlettMixerAppWindow *sm_app_window_new(ScarlettMixerApp *app,
                                          const gchar* prefix);
void                    sm_app_window_open(ScarlettMixerAppWindow *win,
                                           GFile *file);


#endif /* __SCARLETTMIXERAPPWIN_H */
