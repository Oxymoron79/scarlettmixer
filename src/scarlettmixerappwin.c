#include <gtk/gtk.h>

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
    GtkLabel *card_name_label;
    GtkNotebook *output_mix_notebook;
    GList *mix_pages;
    GtkBox *output_channel_box;
    GtkBox *input_sources_box;
    GtkBox *input_switches_box;
};

G_DEFINE_TYPE_WITH_PRIVATE(ScarlettMixerAppWindow, sm_app_window,
        GTK_TYPE_APPLICATION_WINDOW);

static void
sm_app_window_class_init(ScarlettMixerAppWindowClass *class)
{
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_resource(GTK_CSS_PROVIDER(provider),
            "/org/alsa/scarlettmixer/scarlettmixer.css");
    g_object_unref(provider);
    g_debug("sm_app_window_class_init.");
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/scarlettmixerappwin.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, card_name_label);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, output_mix_notebook);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, output_channel_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, input_sources_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, input_switches_box);
}

static void
sm_app_window_init(ScarlettMixerAppWindow *win)
{
    g_debug("sm_app_window_init.");
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
sm_app_window_new(ScarlettMixerApp *app, const gchar* card_name)
{
    ScarlettMixerAppWindow *win;
    ScarlettMixerAppWindowPrivate *priv;
    ScarlettMixerStrip *strip;
    GtkBox *box;
    GtkLabel *label;
    GtkComboBoxText *comboboxtext;
    GList *list, *item;
    int idx;

    g_debug("sm_app_window_new.");
    win = g_object_new(SCARLETTMIXER_APP_WINDOW_TYPE, "application", app, NULL);
    priv = sm_app_window_get_instance_private(win);
    priv->app = app;

    if(card_name)
    {
        gtk_label_set_label(priv->card_name_label, card_name);
    }
    for (list = g_list_first(sm_app_get_channels(priv->app)); list; list = g_list_next(list))
    {
        SmChannel *ch = SM_CHANNEL(list->data);
        switch (sm_channel_get_channel_type(ch))
        {
            case SM_CHANNEL_MASTER:
                strip = sm_strip_new(ch);
                gtk_box_pack_end(priv->output_channel_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
                break;
            case SM_CHANNEL_OUTPUT:
                strip = sm_strip_new(ch);
                gtk_box_pack_start(priv->output_channel_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
                break;
            case SM_CHANNEL_MIX:
                strip = sm_strip_new(ch);
                for (item = g_list_first(priv->mix_pages); item; item = g_list_next(item))
                {
                    if (g_strcmp0(gtk_widget_get_name(GTK_WIDGET(item->data)), g_strdup_printf("Mix %c", sm_channel_get_mix_id(ch))) == 0)
                    {
                        g_debug("Found page for Mix %c.", sm_channel_get_mix_id(ch));
                        break;
                    }
                }
                if (!item)
                {
                    box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2));
                    gtk_widget_set_margin_start(GTK_WIDGET(box), 5);
                    gtk_widget_set_margin_end(GTK_WIDGET(box), 5);
                    gtk_widget_set_margin_top(GTK_WIDGET(box), 5);
                    gtk_widget_set_margin_bottom(GTK_WIDGET(box), 5);
                    gtk_widget_set_name(GTK_WIDGET(box), g_strdup_printf("Mix %c", sm_channel_get_mix_id(ch)));
                    priv->mix_pages = g_list_append(priv->mix_pages, box);
                    label = GTK_LABEL(gtk_label_new(g_strdup_printf("Mix %c", sm_channel_get_mix_id(ch))));
                    idx = gtk_notebook_append_page(priv->output_mix_notebook, GTK_WIDGET(box), GTK_WIDGET(label));
                }
                else
                {
                    for (idx = 0; idx < gtk_notebook_get_n_pages(priv->output_mix_notebook); idx++)
                    {
                        if (gtk_notebook_get_nth_page(priv->output_mix_notebook, idx) == GTK_WIDGET(item->data))
                        {
                            g_debug("Found notebook page.");
                            box = GTK_BOX(item->data);
                            break;
                        }
                    }
                }
                gtk_box_pack_start(box, GTK_WIDGET(strip), FALSE, FALSE, 0);
                gtk_widget_show(GTK_WIDGET(box));
                break;
        }
    }

    for (list = g_list_first(sm_app_get_input_sources(priv->app)); list; list = g_list_next(list))
    {
        SmSource *src = SM_SOURCE(list->data);
        box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        label = GTK_LABEL(gtk_label_new(sm_source_get_name(src)));
        gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
        comboboxtext = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
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

        gtk_box_pack_start(priv->input_sources_box, GTK_WIDGET(box), FALSE, FALSE, 2);
    }
    gtk_widget_show_all(GTK_WIDGET(priv->input_sources_box));
    return win;
}

void
sm_app_window_open(ScarlettMixerAppWindow *win,
        GFile *file)
{
    ScarlettMixerAppWindowPrivate *priv;

    g_debug("sm_app_window_open.");
    priv = sm_app_window_get_instance_private(win);
}
