#include <gtk/gtk.h>
#include <math.h>

#include "scarlettmixerstrip.h"

struct _ScarlettMixerStripClass
{
    GtkBoxClass parent_class;
};

struct _ScarlettMixerStrip
{
    GtkBox parent;
};

typedef struct _ScarlettMixerStripPrivate ScarlettMixerStripPrivate;

struct _ScarlettMixerStripPrivate
{
    SmChannel *channel;
    GtkEntry *name_entry;
    GtkComboBoxText *left_scale_source_comboboxtext;
    GtkScale *left_scale;
    GtkAdjustment *left_adjustment;
    GtkLevelBar *left_levelbar;
    GtkComboBoxText *right_scale_source_comboboxtext;
    GtkScale *right_scale;
    GtkAdjustment *right_adjustment;
    GtkLevelBar *right_levelbar;
    GtkToggleButton *join_togglebutton;
    GtkToggleButton *mute_togglebutton;
};

G_DEFINE_TYPE_WITH_PRIVATE(ScarlettMixerStrip, sm_strip, GTK_TYPE_BOX);

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
    ScarlettMixerStripPrivate *priv;
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
    ScarlettMixerStripPrivate *priv;
    GtkRange *other_range;
    gdouble value;
    gdouble vol_db;
    gboolean joined;
    int err;
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
    ScarlettMixerStripPrivate *priv;
    gboolean active = FALSE;
    int err;

    priv = sm_strip_get_instance_private(user_data);
    active = gtk_toggle_button_get_active(togglebutton);
    if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
    {
        // Set mute state: 0 = Muted, 1 = Unmuted
        if(!sm_channel_volume_set_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, !active))
        {
            g_warning("scale_value_changed_cb: Cannot set volume mute.");
        }
    }
    if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
    {
        // Set mute state: 0 = Muted, 1 = Unmuted
        if(!sm_channel_volume_set_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, !active))
        {
            g_warning("scale_value_changed_cb: Cannot set volume mute.");
        }
    }
}

static void
sm_strip_channel_changed_cb(SmChannel *channel, gpointer user_data)
{
    ScarlettMixerStripPrivate *priv;
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
            gtk_toggle_button_set_active(priv->mute_togglebutton, mute == 0);
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
            gtk_toggle_button_set_active(priv->mute_togglebutton, mute == 0);
        }
    }
}

static void
sm_strip_class_init(ScarlettMixerStripClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/scarlettmixerstrip.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, name_entry);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, left_scale_source_comboboxtext);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, left_scale);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, left_adjustment);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, left_levelbar);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, right_scale_source_comboboxtext);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, right_scale);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, right_adjustment);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, right_levelbar);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, join_togglebutton);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerStrip, mute_togglebutton);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_source_comboboxtext_changed_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_format_value_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            scale_value_changed_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            mute_togglebutton_toggled_cb);
}

ScarlettMixerStrip *
sm_strip_new(SmChannel *channel)
{
    ScarlettMixerStrip *strip;
    ScarlettMixerStripPrivate *priv;
    GList *list;
    gdouble vol_db, min_db, max_db;
    int err, mute;
    int idx;

    strip = g_object_new(SCARLETTMIXER_STRIP_TYPE, NULL);
    priv = sm_strip_get_instance_private(strip);
    priv->channel = channel;

    idx = 0;
    gtk_editable_delete_text(GTK_EDITABLE(priv->name_entry), 0, -1);
    gtk_editable_insert_text(GTK_EDITABLE(priv->name_entry), sm_channel_get_display_name(priv->channel), -1 ,&idx);
    switch (sm_channel_get_channel_type(channel))
    {
        case SM_CHANNEL_MASTER:
        case SM_CHANNEL_OUTPUT:
            gtk_editable_set_editable(GTK_EDITABLE(priv->name_entry), FALSE);
            break;
        default:
            break;
    }
    if (sm_channel_has_source(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
    {
        list = sm_channel_source_get_item_names(priv->channel, SND_MIXER_SCHN_FRONT_LEFT);
        for(list = g_list_first(list); list; list = g_list_next(list))
        {
            gchar name[16];
            gchar num[3];
            if (sscanf(list->data, "%s %s", name, num) == 2)
            {
                gtk_combo_box_text_append_text(priv->left_scale_source_comboboxtext, g_strdup_printf("%c %s", name[0], num));
            }
            else
            {
                gtk_combo_box_text_append_text(priv->left_scale_source_comboboxtext, g_strdup(name));
            }
        }
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
        gtk_widget_set_visible(GTK_WIDGET(priv->left_scale_source_comboboxtext), FALSE);
    }
    if (sm_channel_has_source(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
    {
        list = sm_channel_source_get_item_names(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT);
        for(list = g_list_first(list); list; list = g_list_next(list))
        {
            gchar name[16];
            gchar num[3];
            if (sscanf(list->data, "%s %s", name, num) == 2)
            {
                gtk_combo_box_text_append_text(priv->right_scale_source_comboboxtext, g_strdup_printf("%c %s", name[0], num));
            }
            else
            {
                gtk_combo_box_text_append_text(priv->right_scale_source_comboboxtext, g_strdup(name));
            }
        }
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
        gtk_widget_set_visible(GTK_WIDGET(priv->right_scale_source_comboboxtext), FALSE);
    }

    if (sm_channel_volume_get_range_db(priv->channel, &min_db, &max_db))
    {
        gtk_adjustment_set_lower(priv->left_adjustment, vol_to_value(min_db));
        gtk_adjustment_set_upper(priv->left_adjustment, vol_to_value(max_db));
        gtk_level_bar_set_min_value(priv->left_levelbar, vol_to_value(min_db));
        gtk_level_bar_set_max_value(priv->left_levelbar, vol_to_value(max_db));
        gtk_level_bar_add_offset_value (priv->left_levelbar, GTK_LEVEL_BAR_OFFSET_HIGH, 1.01);
    }

    if (sm_channel_has_volume(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
    {
        gtk_adjustment_set_lower(priv->right_adjustment, vol_to_value(min_db));
        gtk_adjustment_set_upper(priv->right_adjustment, vol_to_value(max_db));
        gtk_level_bar_set_min_value(priv->right_levelbar, vol_to_value(min_db));
        gtk_level_bar_set_max_value(priv->right_levelbar, vol_to_value(max_db));
        gtk_level_bar_add_offset_value (priv->right_levelbar, GTK_LEVEL_BAR_OFFSET_HIGH, 1.01);

        sm_channel_volume_get_db(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, &vol_db);
        gtk_range_set_value(GTK_RANGE(priv->right_scale), vol_to_value(vol_db));
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT))
        {
            // Get mute state: 0 = Muted, 1 = Unmuted
            sm_channel_volume_get_mute(priv->channel, SND_MIXER_SCHN_FRONT_RIGHT, &mute);
            gtk_toggle_button_set_active(priv->mute_togglebutton, mute == 0);
        }
        else
        {
            gtk_widget_set_visible(GTK_WIDGET(priv->mute_togglebutton), FALSE);
        }
    }
    else
    {
        gtk_widget_set_visible(GTK_WIDGET(priv->right_scale_source_comboboxtext), FALSE);
        gtk_widget_set_visible(GTK_WIDGET(priv->right_scale), FALSE);
        gtk_widget_set_visible(GTK_WIDGET(priv->right_levelbar), FALSE);
        gtk_widget_set_visible(GTK_WIDGET(priv->join_togglebutton), FALSE);
    }

    if (sm_channel_has_volume(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
    {
        sm_channel_volume_get_db(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, &vol_db);
        gtk_range_set_value(GTK_RANGE(priv->left_scale), vol_to_value(vol_db));
        if (sm_channel_has_volume_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT))
        {
            // Get mute state: 0 = Muted, 1 = Unmuted
            sm_channel_volume_get_mute(priv->channel, SND_MIXER_SCHN_FRONT_LEFT, &mute);
            gtk_toggle_button_set_active(priv->mute_togglebutton, mute == 0);
        }
        else
        {
            gtk_widget_set_visible(GTK_WIDGET(priv->mute_togglebutton), FALSE);
        }
    }

    g_signal_connect(SM_CHANNEL(priv->channel), "changed", G_CALLBACK(sm_strip_channel_changed_cb), strip);
    return strip;
}

static void
sm_strip_init(ScarlettMixerStrip *win)
{
    ScarlettMixerStripPrivate *priv;

    priv = sm_strip_get_instance_private(win);
    gtk_widget_init_template(GTK_WIDGET(win));
    gtk_scale_add_mark(priv->left_scale, vol_to_value(0.0), GTK_POS_RIGHT, "0");
    gtk_scale_add_mark(priv->right_scale, vol_to_value(0.0), GTK_POS_LEFT, "0");
}
