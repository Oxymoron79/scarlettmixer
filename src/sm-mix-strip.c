/*
 * sm-mix-strip.c - GTK+ widget for a mixer strip.
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

#include <gtk/gtk.h>
#include <math.h>

#include "sm-mix-strip.h"

struct _SmMixStripClass
{
    GtkBoxClass parent_class;
};

struct _SmMixStrip
{
    GtkBox parent;
};

typedef struct _SmMixStripPrivate SmMixStripPrivate;

struct _SmMixStripPrivate
{
    SmChannel *channel[2];
    unsigned int channel_id;
    gchar mix_ids[3];
    gulong changed_handler_id[2];
    GtkEntry *name_entry;
    GtkComboBoxText *source_comboboxtext;
    GtkScale *balance_scale;
    GtkScale *volume_scale;
    GtkAdjustment *volume_adjustment;
    GtkLevelBar *levelbar;
};

G_DEFINE_TYPE_WITH_PRIVATE(SmMixStrip, sm_mix_strip, GTK_TYPE_BOX);

static gdouble
vol_to_value(gdouble vol_db) {
    return pow(10, vol_db / 60.0);
}

static gdouble
value_to_vol(gdouble value) {
    return 60.0*log10(value);
}

static void
scale_source_comboboxtext_changed_cb(GtkComboBox *combo,
        gpointer     user_data)
{
    SmMixStripPrivate *priv;
    int active_idx;

    priv = sm_mix_strip_get_instance_private(user_data);
    active_idx = gtk_combo_box_get_active(combo);
    if(priv->channel[0]) {
        sm_channel_source_set_selected_item_index(priv->channel[0], SND_MIXER_SCHN_MONO, active_idx);
    }
    if(priv->channel[1]) {
        sm_channel_source_set_selected_item_index(priv->channel[1], SND_MIXER_SCHN_MONO, active_idx);
    }
}

static gchar*
volume_scale_format_value_cb(GtkScale *scale,
        gdouble value)
{
    return g_strdup_printf("%0.*f",
            gtk_scale_get_digits(scale), value_to_vol(value));
}

static void
sm_mix_strip_set_balance(SmMixStrip *strip)
{
    SmMixStripPrivate *priv;
    gdouble vol_db;
    gdouble value[2] = { 0.0, 0.0 };
    gdouble val = 0.0;
    gdouble balance = 0.0;

    priv = sm_mix_strip_get_instance_private(strip);
    if (priv->channel[0])
    {
        sm_channel_volume_get_db(priv->channel[0], SND_MIXER_SCHN_MONO, &vol_db);
        value[0] = vol_to_value(vol_db);
    }
    if (priv->channel[1])
    {
        sm_channel_volume_get_db(priv->channel[1], SND_MIXER_SCHN_MONO, &vol_db);
        value[1] = vol_to_value(vol_db);
    }
    if (value[0] > value[1])
    {
        if (value[0] > 0)
        {
            balance = (value[1] / value[0]) - 1.0;
        }
        val = value[0];
    }
    else
    {
        if (value[1] > 0)
        {
            balance = 1 - (value[0] / value[1]);
        }
        val = value[1];
    }
    gtk_range_set_value(GTK_RANGE(priv->balance_scale), balance);
    gtk_range_set_value(GTK_RANGE(priv->volume_scale), val);
}

static void
sm_mix_strip_set_volume(SmMixStrip *strip, gdouble vol, gdouble balance)
{
    SmMixStripPrivate *priv;
    gdouble vol_db;

    priv = sm_mix_strip_get_instance_private(strip);
    if (priv->channel[0])
    {
        vol_db = value_to_vol(vol);
        if (balance > 0)
        {
            vol_db = value_to_vol(vol * (1.0 - balance));
        }
        if (!sm_channel_volume_set_db(priv->channel[0], SND_MIXER_SCHN_MONO, vol_db))
        {
            g_warning("scale_value_changed_cb: Cannot set volume in dB.");
        }
    }
    if (priv->channel[1])
    {
        vol_db = value_to_vol(vol);
        if (balance < 0)
        {
            vol_db = value_to_vol(vol * (1.0 + balance));
        }
        if(!sm_channel_volume_set_db(priv->channel[1], SND_MIXER_SCHN_MONO, vol_db))
        {
            g_warning("scale_value_changed_cb: Cannot set volume in dB.");
        }
    }
}

static void
volume_scale_value_changed_cb(GtkRange *range, gpointer user_data)
{
    SmMixStripPrivate *priv;
    gdouble balance;
    gdouble vol;
    gdouble vol_db;

    priv = sm_mix_strip_get_instance_private(user_data);
    balance = gtk_range_get_value(GTK_RANGE(priv->balance_scale));
    vol = gtk_range_get_value(range);
    vol_db = value_to_vol(vol);
    g_debug("volume_scale_value_changed_cb: %f dB", vol_db);
    sm_mix_strip_set_volume(SM_MIX_STRIP(user_data), vol, balance);
}

static void
balance_scale_value_changed_cb(GtkRange *range, gpointer user_data)
{
    SmMixStripPrivate *priv;
    gdouble balance;
    gdouble vol;

    priv = sm_mix_strip_get_instance_private(user_data);
    vol = gtk_range_get_value(GTK_RANGE(priv->volume_scale));
    balance = gtk_range_get_value(range);
    g_debug("balance_scale_value_changed_cb: %f", balance);
    sm_mix_strip_set_volume(SM_MIX_STRIP(user_data), vol, balance);
}

static void
sm_mix_strip_channel_changed_cb(SmChannel *channel, gpointer user_data)
{
    SmMixStripPrivate *priv;
    gdouble vol_db;
    int mute;
    int idx;

    priv = sm_mix_strip_get_instance_private(user_data);
    g_debug("sm_mix_strip_channel_changed_cb: %s.", gtk_editable_get_chars(GTK_EDITABLE(priv->name_entry), 0, -1));
    idx = sm_channel_source_get_selected_item_index(channel, SND_MIXER_SCHN_MONO);
    if (idx < 0) {
        g_warning("Could not get selected item!");
    }
    else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(priv->source_comboboxtext), idx);
    }
    sm_mix_strip_set_balance(SM_MIX_STRIP(user_data));
}

static void
sm_mix_strip_dispose(GObject *object)
{
    SmMixStripPrivate *priv;

    priv = sm_mix_strip_get_instance_private(SM_MIX_STRIP(object));
    if (priv->channel[0])
    {
        g_debug("sm_mix_strip_dispose: %s", sm_channel_get_name(priv->channel[0]));
        g_signal_handler_disconnect(priv->channel[0], priv->changed_handler_id[0]);
        priv->channel[0] = NULL;
    }
    if (priv->channel[1])
    {
        g_debug("sm_mix_strip_dispose: %s", sm_channel_get_name(priv->channel[1]));
        g_signal_handler_disconnect(priv->channel[1], priv->changed_handler_id[1]);
        priv->channel[1] = NULL;
    }
    G_OBJECT_CLASS(sm_mix_strip_parent_class)->dispose(object);
}

static void
sm_mix_strip_class_init(SmMixStripClass *class)
{
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    G_OBJECT_CLASS(class)->dispose = sm_mix_strip_dispose;

    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_resource(GTK_CSS_PROVIDER(provider),
            "/org/alsa/scarlettmixer/sm-strip.css");
    g_object_unref(provider);
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/sm-mix-strip.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmMixStrip, name_entry);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmMixStrip, source_comboboxtext);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmMixStrip, volume_scale);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmMixStrip, balance_scale);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmMixStrip, volume_adjustment);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmMixStrip, levelbar);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_source_comboboxtext_changed_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            volume_scale_format_value_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            volume_scale_value_changed_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            balance_scale_value_changed_cb);
}

gboolean
sm_mix_strip_add_channel(SmMixStrip *strip, SmChannel *channel)
{
    SmMixStripPrivate *priv;
    unsigned int cid;
    int mix_idx;
    gdouble vol_db;

    priv = sm_mix_strip_get_instance_private(strip);
    if (sm_channel_get_channel_type(channel) != SM_CHANNEL_MIX)
    {
        return FALSE;
    }
    cid = sm_channel_get_id(channel);
    if (priv->channel_id != cid)
    {
        return FALSE;
    }
    mix_idx = (sm_channel_get_mix_id(channel) - 'A') % 2;
    if (priv->channel[mix_idx] != NULL)
    {
        return FALSE;
    }
    priv->channel[mix_idx] = channel;

    // Update volume and balance scale
    sm_mix_strip_set_balance(strip);

    priv->changed_handler_id[mix_idx] = g_signal_connect(
            SM_CHANNEL(priv->channel[mix_idx]),
            "changed",
            G_CALLBACK(sm_mix_strip_channel_changed_cb),
            strip);
    return TRUE;
}

SmMixStrip*
sm_mix_strip_new(SmChannel *channel)
{
    SmMixStrip *strip;
    SmMixStripPrivate *priv;
    GList *list;
    gdouble vol_db, min_db, max_db;
    int mix_idx, mix_idx2, err, idx;

    if (sm_channel_get_channel_type(channel) != SM_CHANNEL_MIX)
    {
        return NULL;
    }
    strip = g_object_new(SM_MIX_STRIP_TYPE, NULL);
    priv = sm_mix_strip_get_instance_private(strip);
    priv->channel_id = sm_channel_get_id(channel);
    mix_idx = (sm_channel_get_mix_id(channel) -'A') % 2;
    mix_idx2 = (mix_idx + 1) % 2;
    priv->channel[mix_idx] = channel;
    priv->channel[mix_idx2] = NULL;
    priv->mix_ids[2] = '\0';
    priv->mix_ids[mix_idx] = sm_channel_get_mix_id(channel);
    if (mix_idx == 0)
        priv->mix_ids[mix_idx2] = priv->mix_ids[0] + 1;
    else
        priv->mix_ids[mix_idx2] = priv->mix_ids[0] - 1;

    idx = 0;
    gtk_editable_delete_text(GTK_EDITABLE(priv->name_entry), 0, -1);
    gtk_editable_insert_text(GTK_EDITABLE(priv->name_entry), sm_channel_get_display_name(priv->channel[mix_idx]), -1 ,&idx);

    list = sm_channel_source_get_item_names(priv->channel[mix_idx], SND_MIXER_SCHN_MONO);
    for(list = g_list_first(list); list; list = g_list_next(list))
    {
        gtk_combo_box_text_append_text(priv->source_comboboxtext, (gchar*)list->data);
        g_free(list->data);
    }
    g_list_free(list);
    idx = sm_channel_source_get_selected_item_index(priv->channel[mix_idx], SND_MIXER_SCHN_MONO);
    if (idx < 0)
    {
        g_warning("Could not get selected item!");
    }
    else
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(priv->source_comboboxtext), idx);
    }

    if (sm_channel_volume_get_range_db(priv->channel[mix_idx], &min_db, &max_db))
    {
        gtk_adjustment_set_lower(priv->volume_adjustment, vol_to_value(min_db));
        gtk_adjustment_set_upper(priv->volume_adjustment, vol_to_value(max_db));
        gtk_level_bar_set_min_value(priv->levelbar, vol_to_value(min_db));
        gtk_level_bar_set_max_value(priv->levelbar, vol_to_value(max_db));
        gtk_level_bar_add_offset_value(priv->levelbar, GTK_LEVEL_BAR_OFFSET_HIGH, 1.01);
    }

    // Update volume and balance scale
    sm_mix_strip_set_balance(strip);

    priv->changed_handler_id[mix_idx] = g_signal_connect(
            SM_CHANNEL(priv->channel[mix_idx]),
            "changed",
            G_CALLBACK(sm_mix_strip_channel_changed_cb),
            strip);
    return strip;
}

static void
sm_mix_strip_init(SmMixStrip *strip)
{
    SmMixStripPrivate *priv;

    priv = sm_mix_strip_get_instance_private(strip);
    gtk_widget_init_template(GTK_WIDGET(strip));
    gtk_scale_add_mark(priv->volume_scale, vol_to_value(0.0), GTK_POS_RIGHT, "0");
    gtk_scale_add_mark(priv->balance_scale, -1, GTK_POS_BOTTOM, "L");
    gtk_scale_add_mark(priv->balance_scale, 0, GTK_POS_BOTTOM, "C");
    gtk_scale_add_mark(priv->balance_scale, 1, GTK_POS_BOTTOM, "R");
}

gchar*
sm_mix_strip_get_mix_ids(SmMixStrip *strip)
{
    SmMixStripPrivate *priv;

    priv = sm_mix_strip_get_instance_private(strip);
    return priv->mix_ids;
}
