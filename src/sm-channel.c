/*
 * sm-channel.c - Output channel object.
 * Copyright (c) 2016 Martin Roesch <martin.roesch79@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>

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
    gboolean joint_volume;
    gchar *name;
    gchar *display_name;
    unsigned int id;
    gchar mix_id;
};

G_DEFINE_TYPE(SmChannel, sm_channel, G_TYPE_OBJECT);

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

    g_debug("sm_channel_finalize: %s", self->name);
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
    self->joint_volume = TRUE;
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
    return self->name;
}

const gchar *
sm_channel_get_display_name(SmChannel *self)
{
    return self->display_name;
}

void
sm_channel_set_display_name(SmChannel *self, const gchar *name)
{
    g_free(self->display_name);
    self->display_name = g_strdup(name);
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
            ret = g_list_prepend(ret, g_strdup(buf));
        }
    }
    return g_list_reverse(ret);
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
sm_channel_volume_get_range_db(SmChannel *self, gdouble *min_db, gdouble *max_db)
{
    int err;
    long min_value;
    long max_value;

    if (self->volume == NULL)
        {
        g_warning("sm_channel_volume_get_range_db: Cannot get volume range in dB!");
        return FALSE;
    }
    err = snd_mixer_selem_get_playback_dB_range(self->volume, &min_value, &max_value);
    if (err < 0)
    {
        g_warning("sm_channel_volume_get_range_db: Cannot get volume range in dB!");
        return FALSE;
    }
    *min_db = (gdouble)min_value / 100.0;
    *max_db = (gdouble)max_value / 100.0;
    return TRUE;
}

gboolean
sm_channel_volume_get_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, gdouble *vol_db)
{
    int err;
    long value;

    if (!sm_channel_has_volume(self, ch))
    {
        g_warning("sm_channel_volume_get_db: Cannot get volume in dB!");
        return FALSE;
    }
    err = snd_mixer_selem_get_playback_dB(self->volume, ch, &value);
    if (err < 0)
    {
        g_warning("sm_channel_volume_get_range_db: Cannot get volume in dB!");
        return FALSE;
    }
    *vol_db = (gdouble)value / 100.0;
    return TRUE;
}

gboolean
sm_channel_volume_set_db(SmChannel *self, snd_mixer_selem_channel_id_t ch, gdouble vol_db)
{
    int err;
    long value;

    if (!sm_channel_has_volume(self, ch))
    {
        g_warning("sm_channel_volume_set_db: Cannot set volume in dB!");
        return FALSE;
    }
    value = (long)round(vol_db * 100.0);
    err = snd_mixer_selem_set_playback_dB(self->volume, ch, value, -1);
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

gboolean
sm_channel_get_joint_volume(SmChannel *self)
{
    return self->joint_volume;
}

void
sm_channel_set_joint_volume(SmChannel *self, gboolean joint)
{
    self->joint_volume = joint;
}

JsonNode*
sm_channel_to_json_node(SmChannel *self)
{
    JsonBuilder *jb;
    JsonNode *jn;
    gdouble vol_db;
    int mute;
    int source_index;

    jb = json_builder_new();
    jb = json_builder_begin_object(jb);

    jb = json_builder_set_member_name(jb, "channel_type");
    jb = json_builder_add_int_value(jb, self->channel_type);

    jb = json_builder_set_member_name(jb, "name");
    jb = json_builder_add_string_value(jb, self->name);

    jb = json_builder_set_member_name(jb, "display_name");
    jb = json_builder_add_string_value(jb, self->display_name);

    switch(self->channel_type)
    {
        case SM_CHANNEL_MASTER:
            jb = json_builder_set_member_name(jb, "vol_db");
            sm_channel_volume_get_db(self, SND_MIXER_SCHN_MONO, &vol_db);
            jb = json_builder_add_double_value(jb, vol_db);

            jb = json_builder_set_member_name(jb, "mute");
            sm_channel_volume_get_mute(self, SND_MIXER_SCHN_MONO, &mute);
            jb = json_builder_add_boolean_value(jb, mute == 0);
            break;
        case SM_CHANNEL_OUTPUT:
            jb = json_builder_set_member_name(jb, "vol_db");
            jb = json_builder_begin_array(jb);
            sm_channel_volume_get_db(self, SND_MIXER_SCHN_FRONT_LEFT, &vol_db);
            jb = json_builder_add_double_value(jb, vol_db);
            sm_channel_volume_get_db(self, SND_MIXER_SCHN_FRONT_RIGHT, &vol_db);
            jb = json_builder_add_double_value(jb, vol_db);
            jb = json_builder_end_array(jb);

            jb = json_builder_set_member_name(jb, "mute");
            jb = json_builder_begin_array(jb);
            sm_channel_volume_get_mute(self, SND_MIXER_SCHN_FRONT_LEFT, &mute);
            jb = json_builder_add_boolean_value(jb, mute == 0);
            sm_channel_volume_get_mute(self, SND_MIXER_SCHN_FRONT_RIGHT, &mute);
            jb = json_builder_add_boolean_value(jb, mute == 0);
            jb = json_builder_end_array(jb);

            jb = json_builder_set_member_name(jb, "joint_vol");
            jb = json_builder_add_boolean_value(jb, self->joint_volume);

            jb = json_builder_set_member_name(jb, "source_index");
            jb = json_builder_begin_array(jb);
            source_index = sm_channel_source_get_selected_item_index(self, SND_MIXER_SCHN_FRONT_LEFT);
            jb = json_builder_add_int_value(jb, source_index);
            source_index = sm_channel_source_get_selected_item_index(self, SND_MIXER_SCHN_FRONT_RIGHT);
            jb = json_builder_add_int_value(jb, source_index);
            jb = json_builder_end_array(jb);
            break;
        case SM_CHANNEL_MIX:
            jb = json_builder_set_member_name(jb, "vol_db");
            sm_channel_volume_get_db(self, SND_MIXER_SCHN_MONO, &vol_db);
            jb = json_builder_add_double_value(jb, vol_db);

            jb = json_builder_set_member_name(jb, "source_index");
            source_index = sm_channel_source_get_selected_item_index(self, SND_MIXER_SCHN_MONO);
            jb = json_builder_add_int_value(jb, source_index);
            break;
        default:
            break;
    }
    jb = json_builder_end_object(jb);
    jn = json_builder_get_root(jb);
    return jn;
}

gboolean
sm_channel_load_from_json_node(SmChannel *self, JsonNode *node)
{
    JsonObject *jo;
    JsonArray *ja;
    const gchar *name;
    gint64 source_index;
    gint64 type;
    gdouble vol_db;
    gboolean mute;
    gboolean joint_vol;

    jo = json_node_get_object(node);
    if (!json_object_has_member(jo, "name"))
    {
        g_warning("Invalid file format: No name member found in channel!");
        return FALSE;
    }
    if (!json_object_has_member(jo, "channel_type"))
    {
        g_warning("Invalid file format: No channel_type member found in channel!");
        return FALSE;
    }

    name = json_object_get_string_member(jo, "name");
    if (g_strcmp0(name, self->name) != 0)
    {
        return FALSE;
    }
    type = json_object_get_int_member(jo, "channel_type");
    if (type != self->channel_type)
    {
        return FALSE;
    }
    g_debug("sm_channel %s: read channel type: %d", self->name, type);
    switch(self->channel_type)
    {
        case SM_CHANNEL_MASTER:
            vol_db = json_object_get_double_member(jo, "vol_db");
            sm_channel_volume_set_db(self, SND_MIXER_SCHN_MONO, vol_db);
            mute = json_object_get_boolean_member(jo, "mute");
            sm_channel_volume_set_mute(self, SND_MIXER_SCHN_MONO, mute == 0);
            break;
        case SM_CHANNEL_OUTPUT:
            ja = json_object_get_array_member(jo, "vol_db");
            if (json_array_get_length(ja) >= 2)
            {
                vol_db = json_array_get_double_element(ja, 0);
                sm_channel_volume_set_db(self, SND_MIXER_SCHN_FRONT_LEFT, vol_db);
                vol_db = json_array_get_double_element(ja, 1);
                sm_channel_volume_set_db(self, SND_MIXER_SCHN_FRONT_RIGHT, vol_db);
            }
            ja = json_object_get_array_member(jo, "mute");
            if (json_array_get_length(ja) >= 2)
            {
                mute = json_array_get_boolean_element(ja, 0);
                sm_channel_volume_set_mute(self, SND_MIXER_SCHN_FRONT_LEFT, mute == 0);
                mute = json_array_get_boolean_element(ja, 1);
                sm_channel_volume_set_mute(self, SND_MIXER_SCHN_FRONT_RIGHT, mute == 0);
            }

            self->joint_volume = json_object_get_boolean_member(jo, "joint_vol");

            ja = json_object_get_array_member(jo, "source_index");
            if (json_array_get_length(ja) >= 2)
            {
                source_index = json_array_get_int_element(ja, 0);
                sm_channel_source_set_selected_item_index(self, SND_MIXER_SCHN_FRONT_LEFT, source_index);
                source_index = json_array_get_int_element(ja, 1);
                sm_channel_source_set_selected_item_index(self, SND_MIXER_SCHN_FRONT_RIGHT, source_index);
            }
            break;
        case SM_CHANNEL_MIX:
            vol_db = json_object_get_double_member(jo, "vol_db");
            sm_channel_volume_set_db(self, SND_MIXER_SCHN_MONO, vol_db);
            source_index = json_object_get_double_member(jo, "source_index");
            sm_channel_source_set_selected_item_index(self, SND_MIXER_SCHN_MONO, source_index);
            name = json_object_get_string_member(jo, "display_name");
            if (g_strcmp0(self->display_name, name) != 0)
            {
                sm_channel_set_display_name(self, name);
            }
            break;
        default:
            break;
    }
    g_signal_emit(self, sm_channel_signals[SM_CHANNEL_SIGNAL_CHANGED], 0);
    return TRUE;
}
