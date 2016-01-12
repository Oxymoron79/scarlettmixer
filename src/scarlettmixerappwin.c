#include <gtk/gtk.h>

#include "scarlettmixer.h"
#include "scarlettmixerapp.h"
#include "scarlettmixerappwin.h"
#include "scarlettmixerstrip.h"
#include "sm-channel.h"
#include "sm-source.h"

struct _ScarlettMixerAppWindowClass
{
    GtkApplicationWindowClass parent_class;
};

struct _ScarlettMixerAppWindow
{
    GtkApplicationWindow parent;
};

typedef struct _ScarlettMixerAppWindowPrivate ScarlettMixerAppWindowPrivate;

struct _ScarlettMixerAppWindowPrivate
{
    ScarlettMixerApp *app;
    GtkBox *output_box;
    GtkBox *input_sources_box;
    GtkBox *input_switches_box;
    GtkBox *mix_sources_box;
};

G_DEFINE_TYPE_WITH_PRIVATE(ScarlettMixerAppWindow, sm_app_window,
        GTK_TYPE_APPLICATION_WINDOW);

static void
sm_app_window_class_init(ScarlettMixerAppWindowClass *class)
{
    printf("sm_app_window_class_init.\n");
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/scarlettmixerappwin.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, output_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, input_sources_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, input_switches_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, mix_sources_box);
}

static void
sm_app_window_init(ScarlettMixerAppWindow *win)
{
    printf("sm_app_window_init.\n");
    gtk_widget_init_template(GTK_WIDGET(win));
}

static void
sm_app_window_comboboxtext_changed_cb(GtkComboBox *combo, gpointer user_data)
{
    SmSource *src = SM_SOURCE(user_data);
    int active_idx;

    active_idx = gtk_combo_box_get_active(combo);
    sm_source_set_selected_item_index(src, active_idx);
}

static void
sm_app_window_source_changed_cb(SmSource *src, gpointer user_data)
{
    GtkComboBoxText *comboboxtext = GTK_COMBO_BOX_TEXT(user_data);
    int active_idx;

    active_idx = sm_source_get_selected_item_index(src);
    gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), active_idx);
}

ScarlettMixerAppWindow *
sm_app_window_new(ScarlettMixerApp *app)
{
    ScarlettMixerAppWindow *win;
    ScarlettMixerAppWindowPrivate *priv;
    ScarlettMixerStrip *strip;
    GtkBox *box;
    GtkLabel *label;
    GtkComboBoxText *comboboxtext;
    GList *list, *item;
    int idx;

    printf("sm_app_window_new.\n");
    win = g_object_new(SCARLETTMIXER_APP_WINDOW_TYPE, "application", app, NULL);
    priv = sm_app_window_get_instance_private(win);
    priv->app = app;

    for (list = g_list_first(sm_app_get_channels(priv->app)); list; list = g_list_next(list))
    {
        SmChannel *ch = SM_CHANNEL(list->data);
        switch (sm_channel_get_channel_type(ch))
        {
            case SM_CHANNEL_MASTER:
                strip = sm_strip_new(ch);
                gtk_box_pack_end(priv->output_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
                break;
            case SM_CHANNEL_OUTPUT:
                strip = sm_strip_new(ch);
                gtk_box_pack_start(priv->output_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
                break;
        }
    }

    for (list = g_list_first(sm_app_get_input_sources(priv->app)); list; list = g_list_next(list))
    {
        SmSource *src = SM_SOURCE(list->data);
        box = (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label = (GtkLabel*)gtk_label_new(sm_source_get_name(src));
        gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
        comboboxtext = (GtkComboBoxText*)gtk_combo_box_text_new();
        for (item = g_list_first(sm_source_get_item_names(src)); item; item = g_list_next(item))
        {
            gtk_combo_box_text_append_text(comboboxtext, item->data);
        }
        idx = sm_source_get_selected_item_index(src);
        if (idx < 0) {
            g_warning("Could not get active enum item!");
        }
        else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), idx);
        }
        g_signal_connect (GTK_WIDGET(comboboxtext), "changed", G_CALLBACK(sm_app_window_comboboxtext_changed_cb), src);
        g_signal_connect(src, "changed", G_CALLBACK(sm_app_window_source_changed_cb), comboboxtext);
        gtk_box_pack_start(box, GTK_WIDGET(comboboxtext), FALSE, FALSE, 0);

        gtk_box_pack_start(priv->input_sources_box, GTK_WIDGET(box), FALSE, FALSE, 5);
    }
    gtk_widget_show_all(GTK_WIDGET(priv->input_sources_box));
/*
    for (list = g_list_first(capture); list; list = g_list_next(list)) {
        ss = list->data;
        box = (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label = (GtkLabel*)gtk_label_new(ss->name);
        gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
        comboboxtext = (GtkComboBoxText*)gtk_combo_box_text_new();
        for(idx = 0; idx < ss->num; idx++) {
            gtk_combo_box_text_append_text(comboboxtext, ss->names[idx]);
        }
        err = snd_mixer_selem_get_enum_item(ss->elem, SND_MIXER_SCHN_FRONT_LEFT, &idx);
        if (err < 0) {
            g_warning("Could not get active enum item!");
        }
        else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), idx);
        }
        gtk_box_pack_start(box, GTK_WIDGET(comboboxtext), FALSE, FALSE, 0);

        gtk_box_pack_start(priv->input_sources_box, GTK_WIDGET(box), FALSE, FALSE, 5);
    }
    gtk_widget_show_all(GTK_WIDGET(priv->input_sources_box));

    for (list = g_list_first(mix_sources); list; list = g_list_next(list)) {
        ss = list->data;
        box = (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        label = (GtkLabel*)gtk_label_new(ss->name);
        gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
        comboboxtext = (GtkComboBoxText*)gtk_combo_box_text_new();
        for(idx = 0; idx < ss->num; idx++) {
            gtk_combo_box_text_append_text(comboboxtext, ss->names[idx]);
        }
        err = snd_mixer_selem_get_enum_item(ss->elem, SND_MIXER_SCHN_FRONT_LEFT, &idx);
        if (err < 0) {
            g_warning("Could not get active enum item!");
        }
        else {
            gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), idx);
        }
        gtk_box_pack_start(box, GTK_WIDGET(comboboxtext), FALSE, FALSE, 0);

        gtk_box_pack_start(priv->mix_sources_box, GTK_WIDGET(box), FALSE, FALSE, 5);
    }
    gtk_widget_show_all(GTK_WIDGET(priv->mix_sources_box));
*/
    return win;
}

void
sm_app_window_open(ScarlettMixerAppWindow *win,
        GFile *file)
{
    ScarlettMixerAppWindowPrivate *priv;

    printf("sm_app_window_open.\n");
    priv = sm_app_window_get_instance_private(win);
}
