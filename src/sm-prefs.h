#ifndef __SM_PREFS_H
#define __SM_PREFS_H

#include <gtk/gtk.h>

#include "sm-appwin.h"

#define SM_PREFS_TYPE (sm_prefs_get_type())
#define SM_PREFS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SM_PREFS_TYPE, SmPrefs))


typedef struct _SmPrefs      SmPrefs;
typedef struct _SmPrefsClass SmPrefsClass;


GType    sm_prefs_get_type(void);
SmPrefs *sm_prefs_new(SmAppWin *win);
#endif /* __SM_PREFS_H */
