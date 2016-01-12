#ifndef __SM_SOURCE_H__
#define __SM_SOURCE_H__

#include <glib.h>
#include <glib-object.h>
#include <alsa/asoundlib.h>

G_BEGIN_DECLS

/*
 * Final type declaration cannot be subclassed.
 */
#define SM_TYPE_SOURCE sm_source_get_type()
G_DECLARE_FINAL_TYPE(SmSource, sm_source, SM, SOURCE, GObject)

/*
 * Method definitions.
 */
SmSource*    sm_source_new();
const gchar* sm_source_get_name(SmSource *self);
gboolean     sm_source_add_mixer_elem(SmSource *self, snd_mixer_elem_t *elem);
gboolean     sm_source_has_mixer_elem(SmSource *self, snd_mixer_elem_t *elem);
void         sm_source_mixer_elem_changed(SmSource *self, snd_mixer_elem_t *elem);

GList*       sm_source_get_item_names(SmSource *self);
int          sm_source_get_selected_item_index(SmSource *self);
gboolean     sm_source_set_selected_item_index(SmSource *self, unsigned int idx);

G_END_DECLS

#endif /* __SM_SOURCE_H__ */
