#ifndef __SM_PREFS_H
#define __SM_PREFS_H
/**
 * @file
 * @brief Header file for the preferences dialog.
 */
#include <gtk/gtk.h>

#include "sm-appwin.h"

/**
 * @brief Macro to get the type information of the preferences dialog.
 */
#define SM_PREFS_TYPE (sm_prefs_get_type())
/**
 * @brief Macro to cast obj to preferences dialog object.
 */
#define SM_PREFS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SM_PREFS_TYPE, SmPrefs))

/**
 * @brief Type definition for preferences dialog object.
 */
typedef struct _SmPrefs      SmPrefs;
/**
 * @brief Type definition foe preferences dialog class.
 */
typedef struct _SmPrefsClass SmPrefsClass;

/**
 * @brief Get the type of the preferences dialog object.
 * @return The type of the preferences dialog object.
 */
GType    sm_prefs_get_type(void);

/**
 * @brief Create a new preferences dialog instance.
 * @param win The application window object.
 * @return Pointer to the preferences dialog.
 */
SmPrefs *sm_prefs_new(SmAppWin *win);
#endif /* __SM_PREFS_H */
