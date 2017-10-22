#ifndef __SM_SWITCH_H__
#define __SM_SWITCH_H__
/**
 * @file
 * @brief Header file for the scarlett mixer switch object.
 */
#include <glib.h>
#include <glib-object.h>
#include <alsa/asoundlib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * @brief Macro to get the type information of the switch object.
 */
#define SM_TYPE_SWITCH sm_switch_get_type()
/**
 * @brief Macro declaring the final switch object type.
 */
G_DECLARE_FINAL_TYPE(SmSwitch, sm_switch, SM, SWITCH, GObject);

/**
 * Type of mixer switch.
 */
typedef enum {
    SM_SWITCH_NONE,           //!< No type assigned.
    SM_SWITCH_INPUT_PAD,      //!< Input pad.
    SM_SWITCH_INPUT_IMPEDANCE,//!< Input impedance.
    SM_SWITCH_CLOCK_SOURCE,   //!< Sample clock source.
    SM_SWITCH_SYNC_STATUS,    //!< Clock sync status.
    SM_SWITCH_USB_SYNC        //!< USB Sync.
} sm_switch_type_t;

/**
 * @brief Create a new switch instance.
 * @return Pointer to new switch instance.
 */
SmSwitch*        sm_switch_new();

/**
 * @brief Get the ALSA mixer element name of the switch.
 * @param self The channel object.
 * @return The ALSSA mixer element name of the swith.
 */
const gchar*     sm_switch_get_name(SmSwitch *self);

/**
 * @brief Get the switch type.
 * @param self The switch object.
 * @return The switch type.
 */
sm_switch_type_t sm_switch_get_switch_type(SmSwitch *self);

/**
 * @brief Get the switch id.
 * The id is parsed from the name of the mixer element passed to
 * @ref sm_switch_add_mixer_elem.
 * @param self The switch object.
 * @return The switch id.
 */
unsigned int     sm_switch_get_id(SmSwitch *self);

/**
 * @brief Add and parse ALSA mixer element to the switch.
 * @param self The switch object.
 * @param elem The ALSA mixer element.
 * @return TRUE if the mixer element was added to the switch, FALSE otherwise.
 */
gboolean         sm_switch_add_mixer_elem(SmSwitch *self, snd_mixer_elem_t *elem);

/**
 * @brief Check whether an ALSA mixer element is contained in the switch.
 * @param self The switch object.
 * @param elem The ALSA mixer element.
 * @return True if the mixer element is contained in the switch, false otherwise.
 */
gboolean         sm_switch_has_mixer_elem(SmSwitch *self, snd_mixer_elem_t *elem);

/**
 * @brief Inform the switch that a ALSA mixer element has changed.
 * If the mixer element is contained in the switch, the switch will emit the
 * SM_SWITCH_SIGNAL_CHANGED signal.
 * @param self The switch object.
 * @param elem The changed ALSA mixer element.
 */
void             sm_switch_mixer_elem_changed(SmSwitch *self, snd_mixer_elem_t *elem);

/**
 * @brief Get the list of item names of the switch..
 * @param self The switch object.
 * @return The list of item names or NULL.
 */
GList*           sm_switch_get_item_names(SmSwitch *self);

/**
 * @brief Get the index of the currently selected item of the switch.
 * @param self The switch object.
 * @return The index or -1 in case of an error.
 */
int              sm_switch_get_selected_item_index(SmSwitch *self);

/**
 * @brief Set the currently selected item of the switch by index.
 * @param self The switch object.
 * @param idx The index.
 * @return TRUE for success or FALSE otherwise..
 */
gboolean         sm_switch_set_selected_item_index(SmSwitch *self, unsigned int idx);

/**
 * @brief Get the name of the currently selected item of the switch.
 * @param self The switch object.
 * @return The item name or NULL in case of an error.
 */
gchar*           sm_switch_get_selected_item_name(SmSwitch *self);

/**
 * @brief Get a JSON object representation of the switch.
 * @param self The switch object.
 * @return The JSON object.
 */
JsonNode*        sm_switch_to_json_node(SmSwitch *self);

/**
 * @brief Load switch settings from JSON object.
 * @param self The switch object.
 * @param node The JSON object.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean         sm_switch_load_from_json_node(SmSwitch *self, JsonNode *node);
G_END_DECLS

#endif /* __SM_SWITCH_H__ */
