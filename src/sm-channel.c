#include "sm-channel.h"

struct _SmChannel
{
    GObject parent_instance;

    /* Other members, including private data. */
    snd_mixer_elem_t *volume;
    snd_mixer_elem_t *source_left;
    snd_mixer_elem_t *source_right;
    snd_mixer_elem_t *source_mix;
    sm_channel_type_t channel_type;
    gchar *name;
    gchar *display_name;
    unsigned int id;
    gchar mix_id;
};

G_DEFINE_TYPE(SmChannel, sm_channel, G_TYPE_OBJECT)

enum
{
    N_PROPERTIES = 1
};

static GParamSpec *sm_channel_properties[N_PROPERTIES] = {NULL};

static void
sm_channel_set_property(GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    SmChannel *self = SM_CHANNEL(object);

    switch (property_id)
    {
        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sm_channel_get_property(GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  SmChannel *self = SM_CHANNEL(object);

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
    SM_CHANNEL_SIGNAL_CHANGED,
    N_SIGNALS
};

static int sm_channel_signals[N_SIGNALS] = {0};

static void
sm_channel_dispose(GObject *gobject)
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
    G_OBJECT_CLASS(sm_channel_parent_class)->dispose(gobject);
}

static void
sm_channel_finalize(GObject *gobject)
{
    SmChannel *self = SM_CHANNEL(gobject);
    g_free(self->name);
    g_free(self->display_name);
    /* Always chain up to the parent class; as with dispose(), finalize()
     * is guaranteed to exist on the parent's class virtual function table
     */
    G_OBJECT_CLASS(sm_channel_parent_class)->finalize(gobject);
}

static void
sm_channel_class_init(SmChannelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* init destruction methods */
    object_class->dispose = sm_channel_dispose;
    object_class->finalize = sm_channel_finalize;

    /* init properties */
    object_class->set_property = sm_channel_set_property;
    object_class->get_property = sm_channel_get_property;

    g_object_class_install_properties(object_class,
                                      N_PROPERTIES,
                                      sm_channel_properties);

    /* init signals */
    sm_channel_signals[SM_CHANNEL_SIGNAL_CHANGED] =
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
sm_channel_init(SmChannel *self)
{
    /* initialize all public and private members to reasonable default values.
     * They are all automatically initialized to 0 to begin with.
     */
}

SmChannel*
sm_channel_new()
{
    return g_object_new (SM_TYPE_CHANNEL, NULL);
}

sm_channel_type_t
sm_channel_get_channel_type(SmChannel *self)
{
    return self->channel_type;
}

const gchar *
sm_channel_get_name(SmChannel *self)
{
    return g_strdup(self->name);
}

const gchar *
sm_channel_get_display_name(SmChannel *self)
{
    return g_strdup(self->display_name);
}

unsigned int
sm_channel_get_id(SmChannel *self)
{
    return self->id;
}

char
sm_channel_get_mix_id(SmChannel *self)
{
    return self->mix_id;
}

gboolean
sm_channel_add_mixer_elem(SmChannel *self, snd_mixer_elem_t *elem)
{
    unsigned int id;
    char buf[16];
    const gchar *elem_name = snd_mixer_selem_get_name(elem);

    if (snd_mixer_selem_has_playback_switch(elem)
            && snd_mixer_selem_has_playback_switch_joined(elem)
            && snd_mixer_selem_has_playback_volume(elem)
            && snd_mixer_selem_has_playback_volume_joined(elem))
    {
        /* Master channel */
        if (self->channel_type != SM_CHANNEL_NONE
                && self->channel_type != SM_CHANNEL_MASTER)
        {
            return FALSE;
        }
        if (self->volume != NULL)
        {
            return FALSE;
        }
        self->channel_type = SM_CHANNEL_MASTER;
        self->volume = elem;
        self->name = g_strdup(elem_name);
        self->display_name = g_strdup(elem_name);
        return TRUE;
    }

    if (snd_mixer_selem_has_playback_switch(elem)
            && !snd_mixer_selem_has_playback_switch_joined(elem)
            && snd_mixer_selem_has_playback_volume(elem)
            && !snd_mixer_selem_has_playback_volume_joined(elem))
    {
        /* Output channel volume */
        if (self->channel_type != SM_CHANNEL_NONE
                && self->channel_type != SM_CHANNEL_OUTPUT)
        {
            return FALSE;
        }
        if (self->volume != NULL)
        {
            return FALSE;
        }
        sscanf(elem_name, "Master %u (%[^)])", &id, buf);
        self->channel_type = SM_CHANNEL_OUTPUT;
        self->volume = elem;
        self->name = g_strdup(elem_name);
        self->display_name = g_strdup(buf);
        self->id = id;
        return TRUE;
    }
    if (!snd_mixer_selem_has_playback_switch(elem)
            && !snd_mixer_selem_has_playback_switch_joined(elem)
            && snd_mixer_selem_has_playback_volume(elem)
            && snd_mixer_selem_has_playback_volume_joined(elem))
    {
        /* Mix channel */
        gchar mix_id;

        if (self->channel_type != SM_CHANNEL_NONE
                && self->channel_type != SM_CHANNEL_MIX)
        {
            return FALSE;
        }
        if (self->volume != NULL)
        {
            return FALSE;
        }
        sscanf(elem_name, "Matrix %u Mix %c", &id, &mix_id);
        if(self->source_left != NULL && self->id != id)
        {
            return FALSE;
        }
        self->channel_type = SM_CHANNEL_MIX;
        self->volume = elem;
        self->name = g_strdup(elem_name);
        self->display_name = g_strdup_printf("%u", id);
        self->id = id;
        self->mix_id = mix_id;
        return TRUE;
    }
    if (snd_mixer_selem_is_enumerated(elem)
            && snd_mixer_selem_is_enum_playback(elem)
            && !snd_mixer_selem_is_enum_capture(elem))
    {
        if (g_str_has_prefix(elem_name, "Master"))
        {
            /* Output source */
            gchar ch;

            if (self->channel_type != SM_CHANNEL_NONE
                    && self->channel_type != SM_CHANNEL_OUTPUT)
            {
                return FALSE;
            }
            sscanf(elem_name, "Master %u%c (%[^)])", &id, &ch, buf);
            switch(ch) {
                case 'L':
                    if (self->source_left != NULL)
                    {
                        return FALSE;
                    }
                    self->source_left = elem;
                    break;
                case 'R':
                    if (self->source_right != NULL)
                    {
                        return FALSE;
                    }
                    self->source_right = elem;
                    break;
            }
            self->channel_type = SM_CHANNEL_OUTPUT;
            self->id = id;
            return TRUE;
        }
        if (g_str_has_prefix(elem_name, "Matrix"))
        {
            /* Mix source */
            if (self->channel_type != SM_CHANNEL_NONE
                    && self->channel_type != SM_CHANNEL_MIX)
            {
                return FALSE;
            }
            if (self->source_left != NULL)
            {
                return FALSE;
            }
            sscanf(elem_name, "Matrix %u Input", &id);
            if (self->volume != NULL && self->id != id)
            {
                return FALSE;
            }
            self->channel_type = SM_CHANNEL_MIX;
            self->source_left = elem;
            self->id = id;
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
sm_channel_has_mixer_elem(SmChannel *self, snd_mixer_elem_t *elem)
{
    if (self->volume == elem
            || self->source_left == elem
            ||self->source_right == elem
            ||self->source_mix == elem)
    {
        return TRUE;
    }
    return FALSE;
}

void
sm_channel_mixer_elem_changed(SmChannel *self, snd_mixer_elem_t *elem)
{
    if (sm_channel_has_mixer_elem(self, elem))
    {
        g_signal_emit(self, sm_channel_signals[SM_CHANNEL_SIGNAL_CHANGED], 0);
    }
}

gboolean
sm_channel_has_source(SmChannel *self, snd_mixer_selem_channel_id_t ch)
{
    switch (ch)
    {
        case SND_MIXER_SCHN_FRONT_LEFT:
            return self->source_left != NULL;
            break;
        case SND_MIXER_SCHN_FRONT_RIGHT:
            return self->source_right != NULL;
            break;
        default:
            return FALSE;
    }
}

GList*
sm_channel_source_get_item_names(SmChannel *self, snd_mixer_selem_channel_id_t ch)
{
    snd_mixer_elem_t *elem;
    int idx;
    gchar buf[16];
    GList *ret = NULL;

    if (!sm_channel_has_source(self, ch))
    {
        return NULL;
    }
    switch (ch)
    {
        case SND_MIXER_SCHN_FRONT_LEFT:
            elem = self->source_left;
            break;
        case SND_MIXER_SCHN_FRONT_RIGHT:
            elem = self->source_right;
            break;
        default:
            return NULL;
    }
    for(idx = 0; idx < snd_mixer_selem_get_enum_items(elem); idx++) {
        if(snd_mixer_selem_get_enum_item_name(elem, idx, 16, buf) == 0) {
            ret = g_list_append(ret, g_strdup(buf));
        }
    }
    return ret;
}

int
sm_channel_source_get_selected_item_index(SmChannel *self, snd_mixer_selem_channel_id_t ch)
{
    snd_mixer_elem_t *elem;
    int err;
    unsigned int idx;

    if (!sm_channel_has_source(self, ch))
    {
        return -1;
    }
    switch (ch)
    {
        case SND_MIXER_SCHN_FRONT_LEFT:
            elem = self->source_left;
            break;
        case SND_MIXER_SCHN_FRONT_RIGHT:
            elem = self->source_right;
            break;
        default:
            return -1;
    }
    err = snd_mixer_selem_get_enum_item(elem, SND_MIXER_SCHN_FRONT_LEFT, &idx);
    if (err < 0)
    {
        g_warning("sm_channel_source_get_selected_item_index: Cannot get selected item index!");
        return -1;
    }
    return idx;
}

gboolean
sm_channel_source_set_selected_item_index(SmChannel *self, snd_mixer_selem_channel_id_t ch, unsigned int idx)
{
    snd_mixer_elem_t *elem;
    int err;

    if (!sm_channel_has_source(self, ch))
    {
        return FALSE;
    }
    switch (ch)
    {
        case SND_MIXER_SCHN_FRONT_LEFT:
            elem = self->source_left;
            break;
        case SND_MIXER_SCHN_FRONT_RIGHT:
            elem = self->source_right;
            break;
        default:
            return FALSE;
    }
    err = snd_mixer_selem_set_enum_item(elem, SND_MIXER_SCHN_FRONT_LEFT, idx);
    if (err < 0)
    {
        g_warning("sm_channel_source_get_selected_item_index: Cannot get selected item index!");
        return FALSE;
    }
    return TRUE;
}

gboolean
sm_channel_has_volume(SmChannel *self, snd_mixer_selem_channel_id_t ch)
{
    if (self->volume == NULL)
    {
        return FALSE;
    }
    return snd_mixer_selem_has_playback_channel(self->volume, ch) == 1;
}

gboolean
sm_channel_has_volume_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch)
{
    if (!sm_channel_has_volume(self, ch))
    {
        return FALSE;
    }
    return snd_mixer_selem_has_playback_switch(self->volume) == 1;
}

gboolean
sm_channel_volume_get_range_db(SmChannel *self, long *min_db, long *max_db)
{
    int err;

    if (self->volume == NULL)
        {
        g_warning("sm_channel_volume_get_range_db: Cannot get volume range in dB!");
        return FALSE;
    }
    err = snd_mixer_selem_get_playback_dB_range(self->volume, min_db, max_db);
    if (err < 0)
    {
        g_warning("sm_channel_volume_get_range_db: Cannot get volume range in dB!");
        return FALSE;
    }
    return TRUE;
}

gboolean
sm_channel_volume_get_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, long *vol_db)
{
    int err;

    if (!sm_channel_has_volume(self, ch))
    {
        g_warning("sm_channel_volume_get_db: Cannot get volume in dB!");
        return FALSE;
    }
    err = snd_mixer_selem_get_playback_dB(self->volume, ch, vol_db);
    if (err < 0)
    {
        g_warning("sm_channel_volume_get_range_db: Cannot get volume in dB!");
        return FALSE;
    }
    return TRUE;
}

gboolean
sm_channel_volume_set_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, long vol_db)
{
    int err;

    if (!sm_channel_has_volume(self, ch))
    {
        g_warning("sm_channel_volume_set_db: Cannot set volume in dB!");
        return FALSE;
    }
    err = snd_mixer_selem_set_playback_dB(self->volume, ch, (long)vol_db * 100, -1);
    if (err < 0)
    {
        g_warning("sm_channel_volume_set_range_db: Cannot set volume in dB!");
        return FALSE;
    }
    return TRUE;
}

gboolean
sm_channel_volume_get_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch, int *mute)
{
    int err;

    if (!sm_channel_has_volume(self, ch))
    {
        g_warning("sm_channel_volume_get_mute: Cannot get volume mute!");
        return FALSE;
    }
    err = snd_mixer_selem_get_playback_switch(self->volume, ch, mute);
    if (err < 0)
    {
        g_warning("sm_channel_volume_get_mute: Cannot get volume mute!");
        return FALSE;
    }
    return TRUE;
}

gboolean
sm_channel_volume_set_mute(SmChannel *self, snd_mixer_selem_channel_id_t ch, int mute)
{
    int err;

    if (!sm_channel_has_volume(self, ch))
    {
        g_warning("sm_channel_volume_set_mute: Cannot set volume mute!");
        return FALSE;
    }
    err = snd_mixer_selem_set_playback_switch(self->volume, ch, mute);
    if (err < 0)
    {
        g_warning("sm_channel_volume_set_mute: Cannot set volume mute!");
        return FALSE;
    }
    return TRUE;
}
