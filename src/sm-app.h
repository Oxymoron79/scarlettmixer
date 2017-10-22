#ifndef __SM_APP_H
#define __SM_APP_H
/**
 * @file
 * @brief Header file for the scarlett mixer application object.
 */
#include <gtk/gtk.h>
#include <gio/gio.h>
#include "sm-switch.h"

/**
 * @brief Macro to get the type information of the application.
 */
#define SM_APP_TYPE (sm_app_get_type())
/**
 * @brief Macro to cast obj to application object.
 */
#define SM_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SM_APP_TYPE, SmApp))

/**
 * @brief Type definition for application object.
 */
typedef struct _SmApp       SmApp;
/**
 * @brief Type definition for application class.
 */
typedef struct _SmAppClass  SmAppClass;


/**
 * @brief Get the type of the application object.
 * @return The type of the application object.
 */
GType        sm_app_get_type();

/**
 * @brief Create a new application instance.
 * @return Pointer to new application instance.
 */
SmApp*       sm_app_new();

/**
 * @brief Find sound card with name prefix.
 *
 * @param prefix ALSA sound card name prefix to find (e.g. "Scarlett").
 * @return The ALSA sound card number, if sound card was found; -1 else;
 */
gint         sm_app_find_card(const gchar* prefix);

/**
 * @brief Open ALSA mixer for given card number and group mixer elements.
 * The mixer elements are assigned to different objects:
 * * Mixer channels: Playback and Matrix mixer channels.
 * * Input sources: Input channels.
 * * Switches: Switches like Input Impedance switch and Sample clock source.
 *
 * @param app The application object.
 * @param card_number ALSA card number. @see sm_app_find_card
 * @return The card name if successful, NULL otherwise.
 */
const gchar* sm_app_open_mixer(SmApp *app, int card_number);

/**
 * @brief Get the GSettings object of the application.
 * @param app The application object.
 * @return  The GSettings object.
 */
GSettings*   sm_app_get_settings(SmApp *app);

/**
 * @brief Get the list of @ref _SmChannel objects.
 * The list is initialized by @ref sm_app_open_mixer().
 * @param app The application object.
 * @return List of @ref _SmChannel objects.
 */
GList*       sm_app_get_channels(SmApp *app);

/**
 * @brief Get the list of @ref _SmSource objects.
 * The list is initialized by @ref sm_app_open_mixer().
 * @param app The application object.
 * @return List of @ref _SmSource objects.
 */
GList*       sm_app_get_input_sources(SmApp *app);

/**
 * @brief Get the list of @ref _SmSwitch objects.
 * The list is initialized by @ref sm_app_open_mixer().
 * @param app The application object.
 * @return List of @ref _SmSwitch objects.
 */
GList*       sm_app_get_input_switches(SmApp *app);

/**
 * @brief Get the Clock Source @ref _SmSwitch object.
 * The switch is initialized by @ref sm_app_open_mixer().
 * @param app The application object.
 * @return The Clock Source @ref _SmSwitch object.
 */
SmSwitch*    sm_app_get_clock_source(SmApp *app);

/**
 * @brief Get the USB Sync Status @ref _SmSwitch object.
 * The switch is initialized by @ref sm_app_open_mixer().
 * @param app The application object.
 * @return The USB Sync Status @ref _SmSwitch object.
 */
SmSwitch*    sm_app_get_sync_status(SmApp *app);

/**
 * @brief Read the card name from a given config file.
 * @param filename The config file to parse.
 * @param err The GError that will be initialized in case of an error.
 * @return The card name, or NULL in case of an error.
 */
gchar*       sm_app_read_card_name_from_config_file(const gchar *filename, GError **err);

/**
 * @brief Write the current configuration to a given file.
 * @param app The application object.
 * @param filename Path of the configuration file to write to.
 * @param err The GError that will be initialized in case of an error.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean     sm_app_write_config_file(SmApp *app, const char *filename, GError **err);

/**
 * @brief Read the configuration from a given file.
 * @param app The application object.
 * @param filename Path of configuration file to read from.
 * @param err The GError that will be initialized in case of an error.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean     sm_app_read_config_file(SmApp *app, const char *filename, GError **err);
#endif /* __SM_APP_H */
