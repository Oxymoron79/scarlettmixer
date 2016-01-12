#include "sm-source.h"

struct _SmSource
{
    GObject parent_instance;

    /* Other members, including private data. */
    snd_mixer_elem_t *elem;
    gchar *name;
};

G_DEFINE_TYPE(SmSource, sm_source, G_TYPE_OBJECT)

enum
{
    N_PROPERTIES = 1
};

static GParamSpec *sm_source_properties[N_PROPERTIES] = {NULL};

static void
sm_source_set_property(GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    SmSource *self = SM_SOURCE(object);

    switch(property_id)
    {
        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sm_source_get_property(GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  SmSource *self = SM_SOURCE(object);

    switch(property_id)
    {
        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

enum
{
    SM_SOURCE_SIGNAL_CHANGED,
    N_SIGNALS
};

static int sm_source_signals[N_SIGNALS] = {};

static void
sm_source_dispose(GObject *gobject)
{
    /* In dispose(), you are supposed to free all types referenced from this
     * object which might themselves hold a reference to self. Generally,
     * the most simple solution is to unref all members on which you own a 
     * reference.
     */

    /* dispose() might be called multiple times, so we must guard against
     * calling g_object_unref() on an invalid GObject by setting the member
     * NULL; g_clear_object() does this for us.
     */

    /* Always chain up to the parent class; there is no need to check if
     * the parent class implements the dispose() virtual function: it is
     * always guaranteed to do so
     */
    G_OBJECT_CLASS(sm_source_parent_class)->dispose(gobject);
}

static void
sm_source_finalize(GObject *gobject)
{
    SmSource *self = SM_SOURCE(gobject);
    g_free(self->name);
    /* Always chain up to the parent class; as with dispose(), finalize()
     * is guaranteed to exist on the parent's class virtual function table
     */
    G_OBJECT_CLASS(sm_source_parent_class)->finalize(gobject);
}

static void
sm_source_class_init(SmSourceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* init destruction methods */
    object_class->dispose = sm_source_dispose;
    object_class->finalize = sm_source_finalize;

    /* init properties */
    object_class->set_property = sm_source_set_property;
    object_class->get_property = sm_source_get_property;

    g_object_class_install_properties(object_class,
                                      N_PROPERTIES,
                                      sm_source_properties);

    /* init signals */
    sm_source_signals[SM_SOURCE_SIGNAL_CHANGED] =
        g_signal_newv("changed",
                      G_TYPE_FROM_CLASS(object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                      NULL /* closure */,
                      NULL /* accumulator */,
                      NULL /* accumulator data */,
                      NULL /* C marshaller */,
                      G_TYPE_NONE /* return_type */,
                      0     /* n_params */,
                      NULL  /* param_types */);
}

static void
sm_source_init(SmSource *self)
{
    /* initialize all public and private members to reasonable default values.
     * They are all automatically initialized to 0 to begin with.
     */
}

SmSource*
sm_source_new()
{
    return g_object_new(SM_TYPE_SOURCE, NULL);
}

const gchar*
sm_source_get_name(SmSource *self)
{
    return g_strdup(self->name);
}

gboolean
sm_source_add_mixer_elem(SmSource *self, snd_mixer_elem_t *elem)
{
    const gchar *elem_name = snd_mixer_selem_get_name(elem);

    if (snd_mixer_selem_is_enumerated(elem)
            && !snd_mixer_selem_is_enum_playback(elem)
            && snd_mixer_selem_is_enum_capture(elem))
    {
        if (self->elem != NULL)
        {
            return FALSE;
        }
        self->elem = elem;
        self->name = g_strdup(elem_name);
        return TRUE;
    }
    return FALSE;
}

gboolean
sm_source_has_mixer_elem(SmSource *self, snd_mixer_elem_t *elem)
{
    if (self->elem == elem)
    {
        return TRUE;
    }
    return FALSE;
}

void
sm_source_mixer_elem_changed(SmSource *self, snd_mixer_elem_t *elem)
{
    if (sm_source_has_mixer_elem(self, elem))
    {
        g_signal_emit(self, sm_source_signals[SM_SOURCE_SIGNAL_CHANGED], 0);
    }
}

GList*
sm_source_get_item_names(SmSource *self)
{
    int idx;
    gchar buf[16];
    GList *ret = NULL;

    if (!self->elem)
    {
        return NULL;
    }
    for(idx = 0; idx < snd_mixer_selem_get_enum_items(self->elem); idx++) {
        if(snd_mixer_selem_get_enum_item_name(self->elem, idx, 16, buf) == 0) {
            ret = g_list_append(ret, g_strdup(buf));
        }
    }
    return ret;
}

int
sm_source_get_selected_item_index(SmSource *self)
{
    int err;
    unsigned int idx;

    if (!self->elem)
    {
        return -1;
    }
    err = snd_mixer_selem_get_enum_item(self->elem, SND_MIXER_SCHN_FRONT_LEFT, &idx);
    if (err < 0)
    {
        g_warning("sm_source_get_selected_item_index: Cannot get selected item index!");
        return -1;
    }
    return idx;
}

gboolean
sm_source_set_selected_item_index(SmSource *self, unsigned int idx)
{
    int err;

    if (!self->elem)
    {
        return FALSE;
    }
    err = snd_mixer_selem_set_enum_item(self->elem, SND_MIXER_SCHN_FRONT_LEFT, idx);
    if (err < 0)
    {
        g_warning("sm_source_get_selected_item_index: Cannot set selected item to index %d!", idx);
        return FALSE;
    }
    return TRUE;
}
