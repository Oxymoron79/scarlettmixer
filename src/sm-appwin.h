#ifndef __SM_APPWIN_H
#define __SM_APPWIN_H

#include <gtk/gtk.h>
#include "sm-app.h"


#define SM_APPWIN_TYPE (sm_appwin_get_type ())
#define SM_APPWIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SM_APPWIN_TYPE, SmAppWin))


typedef struct _SmAppWin         SmAppWin;
typedef struct _SmAppWinClass    SmAppWinClass;


GType     sm_appwin_get_type(void);
SmAppWin *sm_appwin_new(SmApp *app, const gchar* prefix);
void      sm_appwin_open(SmAppWin *win, GFile *file);


#endif /* __SM_APPWIN_H */
