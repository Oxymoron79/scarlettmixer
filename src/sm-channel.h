#ifndef __SM_CHANNEL_H__
#define __SM_CHANNEL_H__
/**
 * @file
 * @brief Header file for the scarlett mixer channel object.
 */
#include <glib.h>
#include <glib-object.h>
#include <alsa/asoundlib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * @brief Macro to get the type information of the channel object.
 */
#define SM_TYPE_CHANNEL sm_channel_get_type()
/**
 * @brief Macro declaring the final channel object type.
 */
G_DECLARE_FINAL_TYPE(SmChannel, sm_channel, SM, CHANNEL, GObject);

/**
 * Type of the mixer channel.
 */
typedef enum {
    SM_CHANNEL_NONE,  ///< No type assigned.
    SM_CHANNEL_MASTER,///< Master channel.
    SM_CHANNEL_OUTPUT,///< Output channel.
    SM_CHANNEL_MIX    ///< Matrix mix channel.
} sm_channel_type_t;

/**
 * @brief Create a new channel instance.
 * @return Pointer to new channel instance.
 */
SmChannel*        sm_channel_new();

/**
 * @brief Get the channel type.
 * @param self The channel object.
 * @return The channel type.
 */
sm_channel_type_t sm_channel_get_channel_type(SmChannel *self);

/**
 * @brief Get the ALSA mixer element name of the channel.
 * @param self The channel object.
 * @return The ALSSA mixer element name of the channel.
 */
const gchar*      sm_channel_get_name(SmChannel *self);

/**
 * @brief Get the name of the channel to display in the user interface.
 * @param self The channel object.
 * @return The name of the channel.
 */
const gchar*      sm_channel_get_display_name(SmChannel *self);

/**
 * @brief Set the name of the channel to display in the user interface.
 * @param self The channel object.
 * @param name The channel name to set.
 */
void              sm_channel_set_display_name(SmChannel *self, const gchar *name);

/**
 * @brief Get the channel id.
 * The id is parsed from the name of the mixer element passed to
 * @ref sm_channel_add_mixer_elem.
 * @param self The channel object.
 * @return The channel id.
 */
unsigned int      sm_channel_get_id(SmChannel *self);

/**
 * @brief Get the mix id of a @ref SM_CHANNEL_MIX channel.
 * The id is parsed from the name of the mixer element passed to
 * @ref sm_channel_add_mixer_elem.
 * @param self The channel object.
 * @return The mix id.
 */
char              sm_channel_get_mix_id(SmChannel *self);

/**
 * @brief Add and parse ALSA mixer element to the channel.
 * @param self The channel object.
 * @param elem The ALSA mixer element.
 * @return TRUE if the mixer element was added to the channel, FALSE otherwise.
 */
gboolean          sm_channel_add_mixer_elem(SmChannel *self, snd_mixer_elem_t *elem);

/**
 * @brief Check whether an ALSA mixer element is contained in the channel.
 * @param self The channel object.
 * @param elem The ALSA mixer element.
 * @return True if the mixer element is contained in the channel, false otherwise.
 */
gboolean          sm_channel_has_mixer_elem(SmChannel *self, snd_mixer_elem_t *elem);

/**
 * @brief Inform the channel that a ALSA mixer element has changed.
 * If the mixer element is contained in the channel, the channel will emit the
 * SM_CHANNEL_SIGNAL_CHANGED signal.
 * @param self The channel object.
 * @param elem The changed ALSA mixer element.
 */
void              sm_channel_mixer_elem_changed(SmChannel *self, snd_mixer_elem_t *elem);

/**
 * @brief Check whether the channel has a source for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @return True if the channel has a source for the channel ID, false otherwise.
 */
gboolean          sm_channel_has_source(SmChannel *self, snd_mixer_selem_channel_id_t ch);

/**
 * @brief Get the list of source names for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @return The list of source names or NULL.
 */
GList*            sm_channel_source_get_item_names(SmChannel *self, snd_mixer_selem_channel_id_t ch);

/**
 * @brief Get the index of the currently selected source for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @return The index or -1 in case of an error.
 */
int               sm_channel_source_get_selected_item_index(SmChannel *self, snd_mixer_selem_channel_id_t ch);

/**
 * @brief Set the currently selected source for a given ALSA channel ID by index.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @param idx The index.
 * @return TRUE for success or FALSE otherwise..
 */
gboolean          sm_channel_source_set_selected_item_index(SmChannel *self, snd_mixer_selem_channel_id_t ch, unsigned int idx);

/**
 * @brief Check whether the channel has a volume for a given ALSA channel ID.
 *  Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel id.
 * @return TRUE if the channel has a volume, FALSE otherwise.
 */
gboolean          sm_channel_has_volume(SmChannel *self, snd_mixer_selem_channel_id_t ch);

/**
 * @brief Check whether the channel has a mute switch for a given ALSA channel ID.
 *  Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @return TRUE if the channel has a mute switch, FALSE otherwise.
 */
gboolean          sm_channel_has_volume_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch);

/**
 * @brief Get the volume range in dB.
 * @param self The channel object.
 * @param[out] min_db Pointer to write the minimal volume in dB to.
 * @param[out] max_db Pointer to write the maximal volume in dB to
 * @return TRUE on success, FALSE otherwise.
 */
gboolean          sm_channel_volume_get_range_db(SmChannel *self, gdouble *min_db, gdouble *max_db);

/**
 * @brief Get the current volume in dB for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @param[out] vol_db Pointer to write the volume in dB to.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean          sm_channel_volume_get_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, gdouble *vol_db);

/**
 * @brief Set the volume in dB for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @param vol_db The volume in dB.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean          sm_channel_volume_set_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, gdouble vol_db);

/**
 * @brief Get the current mute state for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @param[out] mute Pointer to write the mute state to.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean          sm_channel_volume_get_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch, int *mute);

/**
 * @brief Set the mute state for a given ALSA channel ID.
 * Accepted ALSA channel IDs:
 * - SND_MIXER_SCHN_FRONT_LEFT
 * - SND_MIXER_SCHN_FRONT_RIGHT
 * @param self The channel object.
 * @param ch The ALSA channel ID.
 * @param mute The mute state.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean          sm_channel_volume_set_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch, int mute);

/**
 * @brief Check whether the volume channels are joint.
 * @param self The channel object.
 * @return TRUE if the volume channels are joint, FALSE otherwise.
 */
gboolean          sm_channel_get_joint_volume(SmChannel *self);

/**
 * @brief Set the joint status of the volume channels.
 * @param self The channel object.
 * @param joint The joint status.
 */
void              sm_channel_set_joint_volume(SmChannel *self, gboolean joint);

/**
 * @brief Get a JSON object representation of the channel.
 * @param self The channel object.
 * @return The JSON object.
 */
JsonNode*         sm_channel_to_json_node(SmChannel *self);

/**
 * @brief Load channel settings from JSON object.
 * @param self The channel object.
 * @param node The JSON object.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean          sm_channel_load_from_json_node(SmChannel *self, JsonNode *node);
G_END_DECLS

#endif /* __SM_CHANNEL_H__ */
