/*
 * sm-switch.c - Input switch object.
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

#include "sm-switch.h"

struct _SmSwitch
{
    GObject parent_instance;

    /* Other members, including private data. */
    snd_mixer_elem_t *elem;
    sm_switch_type_t type;
    gchar *name;
    unsigned int id;
    gchar *type_name;
};

G_DEFINE_TYPE(SmSwitch, sm_switch, G_TYPE_OBJECT);

enum
{
    SM_SWITCH_SIGNAL_CHANGED,
    N_SIGNALS
};

static int sm_switch_signals[N_SIGNALS] = {};

static void
sm_switch_dispose(GObject *gobject)
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
    G_OBJECT_CLASS(sm_switch_parent_class)->dispose(gobject);
}

static void
sm_switch_finalize(GObject *gobject)
{
    SmSwitch *self = SM_SWITCH(gobject);

    g_debug("sm_switch_finalize: %s", self->name);
    g_free(self->name);
    /* Always chain up to the parent class; as with dispose(), finalize()
     * is guaranteed to exist on the parent's class virtual function table
     */
    G_OBJECT_CLASS(sm_switch_parent_class)->finalize(gobject);
}

static void
sm_switch_class_init(SmSwitchClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* init destruction methods */
    object_class->dispose = sm_switch_dispose;
    object_class->finalize = sm_switch_finalize;

    /* init signals */
    sm_switch_signals[SM_SWITCH_SIGNAL_CHANGED] =
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
sm_switch_init(SmSwitch *self)
{
    /* initialize all public and private members to reasonable default values.
     * They are all automatically initialized to 0 to begin with.
     */
}

SmSwitch*
sm_switch_new()
{
    return g_object_new(SM_TYPE_SWITCH, NULL);
}

const gchar*
sm_switch_get_name(SmSwitch *self)
{
    return self->name;
}

sm_switch_type_t
sm_switch_get_switch_type(SmSwitch *self)
{
    return self->type;
}

unsigned int
sm_switch_get_id(SmSwitch *self)
{
    return self->id;
}

gboolean
sm_switch_add_mixer_elem(SmSwitch *self, snd_mixer_elem_t *elem)
{
    const gchar *elem_name = snd_mixer_selem_get_name(elem);
    unsigned int id;
    char buf[16];

    if (snd_mixer_selem_is_enumerated(elem)
            && !snd_mixer_selem_is_enum_playback(elem)
            && !snd_mixer_selem_is_enum_capture(elem))
    {
        if (self->elem != NULL)
        {
            return FALSE;
        }
        if (g_str_has_prefix(elem_name, "Input"))
        {
            sscanf(elem_name, "Input %u %s", &id, buf);
            self->elem = elem;
            self->name = g_strdup(elem_name);
            self->type_name = g_strdup(buf);
            self->id = id;
            if (g_strcmp0(self->type_name, "Impedance") == 0)
            {
                self->type = SM_SWITCH_INPUT_IMPEDANCE;
            }
            if (g_strcmp0(self->type_name, "Pad") == 0)
            {
                self->type = SM_SWITCH_INPUT_PAD;
            }
            return TRUE;
        }
        if (g_str_has_prefix(elem_name, "Sample Clock Source"))
        {
            self->elem = elem;
            self->name = g_strdup(elem_name);
            self->type_name = g_strdup(elem_name);
            self->id = 0;
            self->type = SM_SWITCH_CLOCK_SOURCE;
            return TRUE;
        }
        if (g_str_has_prefix(elem_name, "Sample Clock Sync"))
        {
            self->elem = elem;
            self->name = g_strdup(elem_name);
            self->type_name = g_strdup(elem_name);
            self->id = 0;
            self->type = SM_SWITCH_SYNC_STATUS;
            return TRUE;
        }
        if (g_str_has_suffix(elem_name, "Sync"))
        {
            self->elem = elem;
            self->name = g_strdup(elem_name);
            self->type_name = g_strdup(elem_name);
            self->id = 0;
            self->type = SM_SWITCH_USB_SYNC;
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
sm_switch_has_mixer_elem(SmSwitch *self, snd_mixer_elem_t *elem)
{
    if (self->elem == elem)
    {
        return TRUE;
    }
    return FALSE;
}

void
sm_switch_mixer_elem_changed(SmSwitch *self, snd_mixer_elem_t *elem)
{
    if (sm_switch_has_mixer_elem(self, elem))
    {
        g_signal_emit(self, sm_switch_signals[SM_SWITCH_SIGNAL_CHANGED], 0);
    }
}

GList*
sm_switch_get_item_names(SmSwitch *self)
{
    int idx;
    gchar buf[16];
    GList *ret = NULL;

    if (!self->elem)
    {
        return NULL;
    }
    for(idx = 0; idx < snd_mixer_selem_get_enum_items(self->elem); idx++)
    {
        if(snd_mixer_selem_get_enum_item_name(self->elem, idx, 16, buf) == 0)
        {
            ret = g_list_prepend(ret, g_strdup(buf));
        }
    }
    return g_list_reverse(ret);
}

int
sm_switch_get_selected_item_index(SmSwitch *self)
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
        g_warning("sm_switch_get_selected_item_index: Cannot get selected item index!");
        return -1;
    }
    return idx;
}

gboolean
sm_switch_set_selected_item_index(SmSwitch *self, unsigned int idx)
{
    int err;

    if (!self->elem)
    {
        return FALSE;
    }
    err = snd_mixer_selem_set_enum_item(self->elem, SND_MIXER_SCHN_FRONT_LEFT, idx);
    if (err < 0)
    {
        g_warning("sm_switch_get_selected_item_index: Cannot set selected item to index %d!", idx);
        return FALSE;
    }
    return TRUE;
}

gchar*
sm_switch_get_selected_item_name(SmSwitch *self)
{
    int err;
    unsigned int idx;
    gchar buf[16];

    if (!self->elem)
    {
        return NULL;
    }
    err = snd_mixer_selem_get_enum_item(self->elem, SND_MIXER_SCHN_FRONT_LEFT, &idx);
    if (err < 0)
    {
        g_warning("sm_switch_get_selected_item_name: Cannot get selected item index!");
        return NULL;
    }
    if(snd_mixer_selem_get_enum_item_name(self->elem, idx, 16, buf) == 0)
    {
        return  g_strdup(buf);
    }
    return NULL;
}
