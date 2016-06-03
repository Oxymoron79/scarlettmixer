#ifndef __SM_SWITCH_H__
#define __SM_SWITCH_H__

#include <glib.h>
#include <glib-object.h>
#include <alsa/asoundlib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/*
 * Final type declaration cannot be subclassed.
 */
#define SM_TYPE_SWITCH sm_switch_get_type()
G_DECLARE_FINAL_TYPE(SmSwitch, sm_switch, SM, SWITCH, GObject);

typedef enum {
    SM_SWITCH_NONE,
    SM_SWITCH_INPUT_PAD,
    SM_SWITCH_INPUT_IMPEDANCE,
    SM_SWITCH_CLOCK_SOURCE,
    SM_SWITCH_SYNC_STATUS,
    SM_SWITCH_USB_SYNC
} sm_switch_type_t;

/*
 * Method definitions.
 */
SmSwitch*        sm_switch_new();
const gchar*     sm_switch_get_name(SmSwitch *self);
sm_switch_type_t sm_switch_get_switch_type(SmSwitch *self);
unsigned int     sm_switch_get_id(SmSwitch *self);
gboolean         sm_switch_add_mixer_elem(SmSwitch *self, snd_mixer_elem_t *elem);
gboolean         sm_switch_has_mixer_elem(SmSwitch *self, snd_mixer_elem_t *elem);
void             sm_switch_mixer_elem_changed(SmSwitch *self, snd_mixer_elem_t *elem);

GList*           sm_switch_get_item_names(SmSwitch *self);
int              sm_switch_get_selected_item_index(SmSwitch *self);
gboolean         sm_switch_set_selected_item_index(SmSwitch *self, unsigned int idx);
gchar*           sm_switch_get_selected_item_name(SmSwitch *self);

JsonNode*        sm_switch_to_json_node(SmSwitch *self);
gboolean         sm_switch_load_from_json_node(SmSwitch *self, JsonNode *node);
G_END_DECLS

#endif /* __SM_SWITCH_H__ */
