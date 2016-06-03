#ifndef __SM_CHANNEL_H__
#define __SM_CHANNEL_H__

#include <glib.h>
#include <glib-object.h>
#include <alsa/asoundlib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/*
 * Final type declaration cannot be subclassed.
 */
#define SM_TYPE_CHANNEL sm_channel_get_type()
G_DECLARE_FINAL_TYPE(SmChannel, sm_channel, SM, CHANNEL, GObject);

typedef enum {
    SM_CHANNEL_NONE,
    SM_CHANNEL_MASTER,
    SM_CHANNEL_OUTPUT,
    SM_CHANNEL_MIX
} sm_channel_type_t;

/*
 * Method definitions.
 */
SmChannel*        sm_channel_new();
sm_channel_type_t sm_channel_get_channel_type(SmChannel *self);
const gchar*      sm_channel_get_name(SmChannel *self);
const gchar*      sm_channel_get_display_name(SmChannel *self);
unsigned int      sm_channel_get_id(SmChannel *self);
char              sm_channel_get_mix_id(SmChannel *self);
gboolean          sm_channel_add_mixer_elem(SmChannel *self, snd_mixer_elem_t *elem);
gboolean          sm_channel_has_mixer_elem(SmChannel *self, snd_mixer_elem_t *elem);
void              sm_channel_mixer_elem_changed(SmChannel *self, snd_mixer_elem_t *elem);

gboolean          sm_channel_has_source(SmChannel *self, snd_mixer_selem_channel_id_t ch);
GList*            sm_channel_source_get_item_names(SmChannel *self, snd_mixer_selem_channel_id_t ch);
int               sm_channel_source_get_selected_item_index(SmChannel *self, snd_mixer_selem_channel_id_t ch);
gboolean          sm_channel_source_set_selected_item_index(SmChannel *self, snd_mixer_selem_channel_id_t ch, unsigned int idx);

gboolean          sm_channel_has_volume(SmChannel *self, snd_mixer_selem_channel_id_t ch);
gboolean          sm_channel_has_volume_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch);
gboolean          sm_channel_volume_get_range_db(SmChannel *self, gdouble *min_db, gdouble *max_db);
gboolean          sm_channel_volume_get_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, gdouble *vol_db);
gboolean          sm_channel_volume_set_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, gdouble vol_db);
gboolean          sm_channel_volume_get_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch, int *mute);
gboolean          sm_channel_volume_set_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch, int mute);

JsonNode*         sm_channel_to_json_node(SmChannel *self);
gboolean          sm_channel_load_from_json_node(SmChannel *self, JsonNode *node);
G_END_DECLS

#endif /* __SM_CHANNEL_H__ */
