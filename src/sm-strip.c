/*
 * sm-strip.c - GTK+ widget for a mixer strip.
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

#include "sm-strip.h"

/**
 * @brief Structure representing the strip widget class.
 */
struct _SmStripClass
{
    GtkBoxClass parent_class; ///< Parent class.
};

/**
 * @brief Structure holding the public attributes of the strip widget.
 */
struct _SmStrip
{
    GtkBox parent; ///< Parent object.
};

/**
 * @brief Type definition for private object of the strip widget.
 */
typedef struct _SmStripPrivate SmStripPrivate;

/**
 * @brief Structure holding the private attributes of the strip widget object.
 */
struct _SmStripPrivate
{
    SmChannel *channel; ///< SmChannel associated with this strip widget.
    gulong changed_handler_id; ///< Signal handler for the "changed" signal of the associated SmChannel.
    GtkEntry *name_entry; ///< Widget to set the name of this mix strip widget.
    GtkComboBoxText *left_scale_source_comboboxtext; ///< Drop down widget to select the input channel for the left channel of the associated SmChannel.
    GtkScale *left_scale; ///< Widget to set the volume of the left channel.
    GtkAdjustment *left_adjustment; ///< Widget to display the volume ticks of the left channel.
    GtkLevelBar *left_levelbar; ///< Widget to show the volume level of the left channel.
    GtkToggleButton *left_mute_togglebutton; ///< Widget to mute the left channel.
    GtkComboBoxText *right_scale_source_comboboxtext; ///< Drop down widget to select the input channel for the right channel of the associated SmChannel.
    GtkScale *right_scale; ///< Widget to set the volume of the right channel.
    GtkAdjustment *right_adjustment; ///< Widget to display the volume ticks of the right channel.
    GtkLevelBar *right_levelbar; ///< Widget to show the volume level of the right channel.
    GtkToggleButton *right_mute_togglebutton; ///< Widget to mute the right channel.
    GtkToggleButton *join_togglebutton; ///< Widget to join the actions of both channels.
};

G_DEFINE_TYPE_WITH_PRIVATE(SmStrip, sm_strip, GTK_TYPE_BOX);

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
    SmStripPrivate *priv;
    int active_idx;

    priv = sm_strip_get_instance_private(user_data);
    active_idx = gtk_combo_box_get_active(combo);
    if(GTK_WIDGET(combo) == GTK_WIDGET(priv->left_scale_source_comboboxtext)) {
        sm_channel_source_set_selected_item_index(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, active_idx);
    }
    else if(GTK_WIDGET(combo) == GTK_WIDGET(priv->right_scale_source_comboboxtext)) {
        sm_channel_source_set_selected_item_index(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, active_idx);
    }
}

static gchar*
scale_format_value_cb(GtkScale *scale,
        gdouble value)
{
    return g_strdup_printf("%0.*f",
            gtk_scale_get_digits(scale), value_to_vol(value));
}

static void
scale_value_changed_cb(GtkRange *range, gpointer user_data)
{
    SmStripPrivate *priv;
    GtkRange *other_range;
    gdouble value;
    gdouble vol_db;
    snd_mixer_selem_channel_id_t ch;

    priv = sm_strip_get_instance_private(user_data);
    value = gtk_range_get_value(range);
    vol_db = value_to_vol(value);
    if(GTK_WIDGET(range) == GTK_WIDGET(priv->left_scale)) {
        g_debug("scale_value_changed_cb: Left - %f dB", vol_db);
        ch = SND_MIXER_SCHN_FRONT_LEFT;
        other_range = GTK_RANGE(priv->right_scale);
        gtk_level_bar_set_value(priv->left_levelbar, value);
    }
    if(GTK_WIDGET(range) == GTK_WIDGET(priv->right_scale)) {
        g_debug("scale_value_changed_cb: Right - %f dB", vol_db);
        ch = SND_MIXER_SCHN_FRONT_RIGHT;
        other_range = GTK_RANGE(priv->left_scale);
        gtk_level_bar_set_value(priv->right_levelbar, value);
    }
    if(!sm_channel_volume_set_db(priv->channel, ch, vol_db))
    {
        g_warning("scale_value_changed_cb: Cannot set volume in dB.");
    }
    if (gtk_toggle_button_get_active(priv->join_togglebutton)
            && gtk_widget_is_visible(GTK_WIDGET(other_range)))
    {
        gtk_range_set_value(other_range, value);
    }
}

static void
mute_togglebutton_toggled_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
    SmStripPrivate *priv;
    gboolean active = FALSE;

    priv = sm_strip_get_instance_private(user_data);
    active = gtk_toggle_button_get_active(togglebutton);
    if (togglebutton == priv->left_mute_togglebutton)
    {
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
        {
            // Set mute state: 0 = Muted, 1 = Unmuted
            if(!sm_channel_volume_set_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, !active))
            {
                g_warning("mute_togglebutton_toggled_cb: Cannot set volume mute.");
            }
        }
    }
    if (togglebutton == priv->right_mute_togglebutton)
    {
        g_debug("mute_togglebutton_toggled_cb: right_mute_togglebutton");
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
        {
            // Set mute state: 0 = Muted, 1 = Unmuted
            if(!sm_channel_volume_set_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, !active))
            {
                g_warning("mute_togglebutton_toggled_cb: Cannot set volume mute.");
            }
        }
    }
}

static void
join_togglebutton_toggled_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
    SmStripPrivate *priv;
    gboolean active;

    priv = sm_strip_get_instance_private(user_data);
    active = gtk_toggle_button_get_active(togglebutton);
    sm_channel_set_joint_volume(priv->channel, active);
}

static void
sm_strip_channel_changed_cb(SmChannel *channel, gpointer user_data)
{
    SmStripPrivate *priv;
    gdouble vol_db;
    int mute;
    int idx;

    priv = sm_strip_get_instance_private(user_data);
    g_debug("sm_strip_channel_changed_cb: %s.", gtk_editable_get_chars(GTK_EDITABLE(priv->name_entry), 0, -1));
    if (sm_channel_has_source(channel, SND_MIXER_SCHN_FRONT_LEFT)) {
        idx = sm_channel_source_get_selected_item_index(channel, SND_MIXER_SCHN_FRONT_LEFT);
        if (idx < 0) {
            g_warning("Could not get selected item!");
        }
        else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(priv->left_scale_source_comboboxtext), idx);
        }
    }
    if (sm_channel_has_source(channel, SND_MIXER_SCHN_FRONT_RIGHT)) {
        idx = sm_channel_source_get_selected_item_index(channel, SND_MIXER_SCHN_FRONT_RIGHT);
        if (idx < 0) {
            g_warning("Could not get selected item!");
        }
        else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(priv->right_scale_source_comboboxtext), idx);
        }
    }
    if (sm_channel_has_volume(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
    {
        sm_channel_volume_get_db(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, &vol_db);
        gtk_range_set_value(GTK_RANGE(priv->left_scale), vol_to_value(vol_db));
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
        {
            // Get mute state: 0 = Muted, 1 = Unmuted
            sm_channel_volume_get_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, &mute);
            gtk_toggle_button_set_active(priv->left_mute_togglebutton, mute == 0);
        }
    }

    if (sm_channel_has_volume(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
    {
        sm_channel_volume_get_db(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, &vol_db);
        gtk_range_set_value(GTK_RANGE(priv->right_scale), vol_to_value(vol_db));
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
        {
            // Get mute state: 0 = Muted, 1 = Unmuted
            sm_channel_volume_get_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, &mute);
            gtk_toggle_button_set_active(priv->right_mute_togglebutton, mute == 0);
        }
    }
}

static void
sm_strip_dispose(GObject *object)
{
    SmStripPrivate *priv;

    priv = sm_strip_get_instance_private(SM_STRIP(object));
    if (priv->channel)
        g_debug("sm_strip_dispose: %s", sm_channel_get_name(priv->channel));
    if (priv->channel)
    {
        g_signal_handler_disconnect(priv->channel, priv->changed_handler_id);
        priv->channel = NULL;
    }
    G_OBJECT_CLASS(sm_strip_parent_class)->dispose(object);
}

static void
sm_strip_class_init(SmStripClass *class)
{
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    G_OBJECT_CLASS(class)->dispose = sm_strip_dispose;

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
            "/org/alsa/scarlettmixer/sm-strip.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, name_entry);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, left_scale_source_comboboxtext);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, left_scale);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, left_adjustment);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, left_levelbar);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, left_mute_togglebutton);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, right_scale_source_comboboxtext);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, right_scale);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, right_adjustment);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, right_levelbar);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, right_mute_togglebutton);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmStrip, join_togglebutton);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_source_comboboxtext_changed_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_format_value_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_value_changed_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            mute_togglebutton_toggled_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            join_togglebutton_toggled_cb);
}

SmStrip*
sm_strip_new(SmChannel *channel)
{
    SmStrip *strip;
    SmStripPrivate *priv;
    GList *list;
    gdouble vol_db, min_db, max_db;
    int idx, mute;

    strip = g_object_new(SM_STRIP_TYPE, NULL);
    priv = sm_strip_get_instance_private(strip);
    priv->channel = channel;

    idx = 0;
    gtk_editable_delete_text(GTK_EDITABLE(priv->name_entry), 0, -1);
    gtk_editable_insert_text(GTK_EDITABLE(priv->name_entry), sm_channel_get_display_name(priv->channel), -1 ,&idx);
    gtk_toggle_button_set_active(priv->join_togglebutton, sm_channel_get_joint_volume(priv->channel));
    switch (sm_channel_get_channel_type(channel))
    {
        case SM_CHANNEL_MASTER:
        case SM_CHANNEL_OUTPUT:
            gtk_editable_set_editable(GTK_EDITABLE(priv->name_entry), FALSE);
            gtk_widget_set_sensitive(GTK_WIDGET(priv->name_entry), FALSE);
            break;
        default:
            break;
    }
    if (sm_channel_has_source(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
    {
        list = sm_channel_source_get_item_names(priv->channel, SND_MIXER_SCHN_FRONT_LEFT);
        for(list = g_list_first(list); list; list = g_list_next(list))
        {
            gtk_combo_box_text_append_text(priv->left_scale_source_comboboxtext, (gchar*)list->data);
            g_free(list->data);
        }
        g_list_free(list);
        idx = sm_channel_source_get_selected_item_index(priv->channel, SND_MIXER_SCHN_FRONT_LEFT);
        if (idx < 0)
        {
            g_warning("Could not get selected item!");
        }
        else
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(priv->left_scale_source_comboboxtext), idx);
        }
    }
    else
    {
        gtk_widget_hide(GTK_WIDGET(priv->left_scale_source_comboboxtext));
    }
    if (sm_channel_has_source(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
    {
        list = sm_channel_source_get_item_names(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT);
        for(list = g_list_first(list); list; list = g_list_next(list))
        {
            gtk_combo_box_text_append_text(priv->right_scale_source_comboboxtext, (gchar*)list->data);
            g_free(list->data);
        }
        g_list_free(list);
        idx = sm_channel_source_get_selected_item_index(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT);
        if (idx < 0)
        {
            g_warning("Could not get selected item!");
        }
        else
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(priv->right_scale_source_comboboxtext), idx);
        }
    }
    else
    {
        gtk_widget_hide(GTK_WIDGET(priv->right_scale_source_comboboxtext));
    }

    if (sm_channel_volume_get_range_db(priv->channel, &min_db, &max_db))
    {
        gtk_adjustment_set_lower(priv->left_adjustment, vol_to_value(min_db));
        gtk_adjustment_set_upper(priv->left_adjustment, vol_to_value(max_db));
        gtk_level_bar_set_min_value(priv->left_levelbar, vol_to_value(min_db));
        gtk_level_bar_set_max_value(priv->left_levelbar, vol_to_value(max_db));
        gtk_level_bar_add_offset_value(priv->left_levelbar, GTK_LEVEL_BAR_OFFSET_HIGH, 1.01);
    }

    if (sm_channel_has_volume(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
    {
        gtk_adjustment_set_lower(priv->right_adjustment, vol_to_value(min_db));
        gtk_adjustment_set_upper(priv->right_adjustment, vol_to_value(max_db));
        gtk_level_bar_set_min_value(priv->right_levelbar, vol_to_value(min_db));
        gtk_level_bar_set_max_value(priv->right_levelbar, vol_to_value(max_db));
        gtk_level_bar_add_offset_value(priv->right_levelbar, GTK_LEVEL_BAR_OFFSET_HIGH, 1.01);

        sm_channel_volume_get_db(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, &vol_db);
        gtk_range_set_value(GTK_RANGE(priv->right_scale), vol_to_value(vol_db));
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
        {
            // Get mute state: 0 = Muted, 1 = Unmuted
            sm_channel_volume_get_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, &mute);
            gtk_toggle_button_set_active(priv->right_mute_togglebutton, mute == 0);
        }
        else
        {
            gtk_widget_hide(GTK_WIDGET(priv->right_mute_togglebutton));
        }
    }
    else
    {
        gtk_widget_hide(GTK_WIDGET(priv->right_scale_source_comboboxtext));
        gtk_widget_hide(GTK_WIDGET(priv->right_scale));
        gtk_widget_hide(GTK_WIDGET(priv->right_levelbar));
        gtk_widget_hide(GTK_WIDGET(priv->right_mute_togglebutton));
        gtk_widget_hide(GTK_WIDGET(priv->join_togglebutton));
    }

    if (sm_channel_has_volume(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
    {
        sm_channel_volume_get_db(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, &vol_db);
        gtk_range_set_value(GTK_RANGE(priv->left_scale), vol_to_value(vol_db));
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
        {
            // Get mute state: 0 = Muted, 1 = Unmuted
            sm_channel_volume_get_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, &mute);
            gtk_toggle_button_set_active(priv->left_mute_togglebutton, mute == 0);
        }
        else
        {
            gtk_widget_hide(GTK_WIDGET(priv->left_mute_togglebutton));
        }
    }

    priv->changed_handler_id = g_signal_connect(SM_CHANNEL(priv->channel),
            "changed",
            G_CALLBACK(sm_strip_channel_changed_cb),
            strip);
    return strip;
}

static void
sm_strip_init(SmStrip *strip)
{
    SmStripPrivate *priv;

    priv = sm_strip_get_instance_private(strip);
    gtk_widget_init_template(GTK_WIDGET(strip));
    gtk_scale_add_mark(priv->left_scale, vol_to_value(0.0), GTK_POS_RIGHT, "0");
    gtk_scale_add_mark(priv->right_scale, vol_to_value(0.0), GTK_POS_LEFT, "0");
}
