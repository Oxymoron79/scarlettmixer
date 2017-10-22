#ifndef __SM_APPWIN_H
#define __SM_APPWIN_H
/**
 * @file
 * @brief Header file for the main application window object.
 */
#include <gtk/gtk.h>
#include "sm-app.h"

/**
 * @brief Macro to get the type information of the application window.
 */
#define SM_APPWIN_TYPE (sm_appwin_get_type ())
/**
 * @brief Macro to cast obj to application window object.
 */
#define SM_APPWIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SM_APPWIN_TYPE, SmAppWin))

/**
 * @brief Type definition for application window object.
 */
typedef struct _SmAppWin         SmAppWin;
/**
 * @brief Type definition for application window class.
 */
typedef struct _SmAppWinClass    SmAppWinClass;

/**
 * @brief Get the type of the application window object.
 * @return The type of the application window object.
 */
GType          sm_appwin_get_type(void);

/**
 * @brief Create a new application window instance.
 * @param app The application object.
 * @param prefix ALSA sound card name prefix to find (e.g. "Scarlett").
 * @return Pointer to new application window instance.
 */
SmAppWin      *sm_appwin_new(SmApp *app, const gchar* prefix);

/**
 * @brief Get the GtkFileFilter for the configuration file dialogs.
 * @param win The application window object.
 * @return The GtkFileFilter. Unref when no longer needed with g_object_unref.
 */
GtkFileFilter *sm_appwin_get_file_filter(SmAppWin *win);

/**
 * @brief Run the open configuration file dialog.
 * Loads the configuration from the selected configuration file.
 * @param win The application window object.
 */
void           sm_appwin_open_configfile(SmAppWin *win);

/**
 * @brief Save the current configuration to the currently loaded configuration file.
 * If no configuration was is loaded, the save configuration file dialog is run.
 * @param win The application window object.
 */
void           sm_appwin_save_configfile(SmAppWin *win);

/**
 * @brief Run the save as configuration file dialog.
 * Writes the configuration to the selected configuration file.
 * @param win The application window object.
 */
void           sm_appwin_saveas_configfile(SmAppWin *win);


#endif /* __SM_APPWIN_H */
