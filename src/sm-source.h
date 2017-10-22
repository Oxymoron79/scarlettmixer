#ifndef __SM_SOURCE_H__
#define __SM_SOURCE_H__
/**
 * @file
 * @brief Header file for the scarlett mixer input source object.
 */
#include <glib.h>
#include <glib-object.h>
#include <alsa/asoundlib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * @brief Macro to get the type information of the input source object.
 */
#define SM_TYPE_SOURCE sm_source_get_type()
/**
 * @brief Macro declaring the final input source object type.
 */
G_DECLARE_FINAL_TYPE(SmSource, sm_source, SM, SOURCE, GObject);

/**
 * @brief Create a new input source instance.
 * @return Pointer to new input source instance.
 */
SmSource*    sm_source_new();

/**
 * @brief Get the name of the input source.
 * @param self The input source object.
 * @return The name of the input source.
 */
const gchar* sm_source_get_name(SmSource *self);

/**
 * @brief Add and parse ALSA mixer element to the input source.
 * @param self he input source object.
 * @param elem The ALSA mixer element.
 * @return TRUE if the mixer element was added to the channel, FALSE otherwise.
 */
gboolean     sm_source_add_mixer_elem(SmSource *self, snd_mixer_elem_t *elem);

/**
 * @brief Check whether an ALSA mixer element is contained in the input source.
 * @param self The input source object.
 * @param elem The ALSA mixer element.
 * @return TRUE if the mixer element is contained in the input source, FALSE otherwise.
 */
gboolean     sm_source_has_mixer_elem(SmSource *self, snd_mixer_elem_t *elem);

/**
 * @brief Inform the input source that a ALSA mixer element has changed.
 * If the mixer element is contained in the input source, the channel will emit
 * the SM_SOURCE_SIGNAL_CHANGED signal.
 * @param self The input source object.
 * @param elem The changed ALSA mixer element.
 */
void         sm_source_mixer_elem_changed(SmSource *self, snd_mixer_elem_t *elem);

/**
 * @brief Get the list of source names.
 * @param self The input source object.
 * @return The list of source names or NULL.
 */
GList*       sm_source_get_item_names(SmSource *self);

/**
 * @brief Get the index of the currently selected source.
 * @param self The input source object.
 * @return The index or -1 in case of an error.
 */
int          sm_source_get_selected_item_index(SmSource *self);

/**
 * @brief Set the currently selected source by index.
 * @param self The input source object.
 * @param idx The index.
 * @return TRUE for success or FALSE otherwise..
 */
gboolean     sm_source_set_selected_item_index(SmSource *self, unsigned int idx);

/**
 * @brief Get a JSON object representation of the input source.
 * @param self The input source object.
 * @return The JSON object.
 */
JsonNode*    sm_source_to_json_node(SmSource *self);

/**
 * @brief Load input source settings from JSON object.
 * @param self The input source object.
 * @param node The JSON object.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean     sm_source_load_from_json_node(SmSource *self, JsonNode *node);
G_END_DECLS

#endif /* __SM_SOURCE_H__ */
