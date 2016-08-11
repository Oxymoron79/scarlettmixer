#ifndef __SM_APPWIN_H
#define __SM_APPWIN_H

#include <gtk/gtk.h>
#include "sm-app.h"


#define SM_APPWIN_TYPE (sm_appwin_get_type ())
#define SM_APPWIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SM_APPWIN_TYPE, SmAppWin))

typedef struct _SmAppWin         SmAppWin;
typedef struct _SmAppWinClass    SmAppWinClass;


GType          sm_appwin_get_type(void);
SmAppWin      *sm_appwin_new(SmApp *app, const gchar* prefix);
GtkFileFilter *sm_appwin_get_file_filter(SmAppWin *win);
void           sm_appwin_open_configfile(SmAppWin *win);
void           sm_appwin_save_configfile(SmAppWin *win);
void           sm_appwin_saveas_configfile(SmAppWin *win);


#endif /* __SM_APPWIN_H */
