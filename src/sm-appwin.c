#include <gtk/gtk.h>

#include "sm-app.h"
#include "sm-appwin.h"
#include "sm-strip.h"
#include "sm-channel.h"
#include "sm-source.h"

#define SM_APP_WIN_INIT_TIMEOUT (100)
#define SM_APP_WIN_BOX_MARGIN (5)
#define SM_APP_WIN_BOX_PADDING (2)

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
    const gchar* prefix;
    GtkLabel *card_name_label;
    GtkToggleButton *reveal_input_sources_togglebutton;
    GtkStack *main_stack;
    GtkNotebook *output_mix_notebook;
    GList *mix_pages;
    GtkBox *output_channel_box;
    GtkBox *input_sources_box;
    GtkBox *input_switches_box;
};

typedef struct _ScarlettMixerAppWindowInitArg ScarlettMixerAppWindowInitArg;

struct _ScarlettMixerAppWindowInitArg
{
    ScarlettMixerAppWindowPrivate *priv;
    GList *list;
};

G_DEFINE_TYPE_WITH_PRIVATE(ScarlettMixerAppWindow, sm_app_window,
        GTK_TYPE_APPLICATION_WINDOW);

// Forward declarations
static void
sm_app_window_init_channels(ScarlettMixerAppWindow *win, const gchar *card_name);

static gboolean
sm_app_window_check_for_interface(gpointer win)
{
    ScarlettMixerAppWindowPrivate *priv;
    gint card_number;
    const gchar *card_name;

    priv = sm_app_window_get_instance_private(win);
    card_number = sm_app_find_card(priv->prefix);
    if (card_number >= 0)
    {
        card_name = sm_app_open_mixer(priv->app, card_number);
        sm_app_window_init_channels(win, card_name);
        g_application_unmark_busy(G_APPLICATION(priv->app));
    }
    else {
        g_debug("No interface with prefix %s found.", priv->prefix);
        gtk_stack_set_visible_child_name(priv->main_stack, "error");
        g_application_unmark_busy(G_APPLICATION(priv->app));
    }
    return FALSE;
}

static void
refresh_button_clicked_cb(GtkButton *button, gpointer data)
{
    ScarlettMixerAppWindow *win;
    ScarlettMixerAppWindowPrivate *priv;

    g_debug("Refresh interface list.");
    win = SCARLETTMIXER_APP_WINDOW(data);
    priv = sm_app_window_get_instance_private(win);
    g_application_mark_busy(G_APPLICATION(priv->app));
    gtk_stack_set_visible_child_name(priv->main_stack, "init");
    g_timeout_add(SM_APP_WIN_INIT_TIMEOUT, sm_app_window_check_for_interface, (gpointer)win);
}

static void
reveal_input_sources_togglebutton_toggled_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
    gboolean active;
    GtkRevealer *revealer = GTK_REVEALER(user_data);

    active = gtk_toggle_button_get_active(togglebutton);
    gtk_revealer_set_reveal_child(revealer, active);
}

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
            "/org/alsa/scarlettmixer/sm-appwin.css");
    g_object_unref(provider);
    g_debug("sm_app_window_class_init.");
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/sm-appwin.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, card_name_label);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, reveal_input_sources_togglebutton);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, main_stack);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, output_mix_notebook);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, output_channel_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, input_sources_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            ScarlettMixerAppWindow, input_switches_box);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            reveal_input_sources_togglebutton_toggled_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            refresh_button_clicked_cb);
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

static gboolean
sm_app_window_init_strips(gpointer data)
{
    ScarlettMixerAppWindowInitArg *arg;
    SmChannel *ch;
    ScarlettMixerStrip *strip;
    GList *item;
    GtkBox *box;
    GtkLabel *label;
    gint idx;

    arg = (ScarlettMixerAppWindowInitArg*)data;
    ch = SM_CHANNEL(arg->list->data);
    switch (sm_channel_get_channel_type(ch))
    {
        case SM_CHANNEL_MASTER:
        {
            strip = sm_strip_new(ch);
            gtk_box_pack_end(arg->priv->output_channel_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
            break;
        }
        case SM_CHANNEL_OUTPUT:
        {
            strip = sm_strip_new(ch);
            gtk_box_pack_start(arg->priv->output_channel_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
            break;
        }
        case SM_CHANNEL_MIX:
        {
            strip = sm_strip_new(ch);
            for (item = g_list_first(arg->priv->mix_pages); item; item = g_list_next(item))
            {
                if (g_strcmp0(gtk_widget_get_name(GTK_WIDGET(item->data)), g_strdup_printf("Mix %c", sm_channel_get_mix_id(ch))) == 0)
                {
                    g_debug("Found page for Mix %c.", sm_channel_get_mix_id(ch));
                    break;
                }
            }
            if (!item)
            {
                box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SM_APP_WIN_BOX_PADDING));
                gtk_widget_set_margin_start(GTK_WIDGET(box), SM_APP_WIN_BOX_MARGIN);
                gtk_widget_set_margin_end(GTK_WIDGET(box), SM_APP_WIN_BOX_MARGIN);
                gtk_widget_set_margin_top(GTK_WIDGET(box), SM_APP_WIN_BOX_MARGIN);
                gtk_widget_set_margin_bottom(GTK_WIDGET(box), SM_APP_WIN_BOX_MARGIN);
                gtk_widget_set_name(GTK_WIDGET(box), g_strdup_printf("Mix %c", sm_channel_get_mix_id(ch)));
                arg->priv->mix_pages = g_list_append(arg->priv->mix_pages, box);
                label = GTK_LABEL(gtk_label_new(g_strdup_printf("Mix %c", sm_channel_get_mix_id(ch))));
                idx = gtk_notebook_append_page(arg->priv->output_mix_notebook, GTK_WIDGET(box), GTK_WIDGET(label));
            }
            else
            {
                for (idx = 0; idx < gtk_notebook_get_n_pages(arg->priv->output_mix_notebook); idx++)
                {
                    if (gtk_notebook_get_nth_page(arg->priv->output_mix_notebook, idx) == GTK_WIDGET(item->data))
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
    arg->list = g_list_next(arg->list);
    if (arg->list)
    {
        return TRUE;
    }
    else
    {
        gtk_stack_set_visible_child_name(arg->priv->main_stack, "output");
        g_free(arg);
        return FALSE;
    }
}

static gboolean
sm_app_window_init_input_sources(gpointer data)
{
    ScarlettMixerAppWindowInitArg *arg;
    SmSource *src;
    GList *item;
    GtkBox *box;
    GtkLabel *label;
    GtkComboBoxText *comboboxtext;
    GtkStyleContext *style_ctx;
    gint idx;

    arg = (ScarlettMixerAppWindowInitArg*)data;
    src = SM_SOURCE(arg->list->data);
    box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, SM_APP_WIN_BOX_PADDING));
    sscanf(sm_source_get_name(src), "Input Source %02u", &idx);
    label = GTK_LABEL(gtk_label_new(g_strdup_printf("Input %d", idx)));
    gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
    comboboxtext = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    style_ctx = gtk_widget_get_style_context(GTK_WIDGET(comboboxtext));
    gtk_style_context_add_class(style_ctx, "small-text");
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
    gtk_box_pack_start(arg->priv->input_sources_box, GTK_WIDGET(box), FALSE, FALSE, 0);
    arg->list = g_list_next(arg->list);
    if (arg->list)
    {
        return TRUE;
    }
    else
    {
        gtk_widget_show(GTK_WIDGET(arg->priv->reveal_input_sources_togglebutton));
        gtk_widget_show_all(GTK_WIDGET(arg->priv->input_sources_box));
        g_free(arg);
        return FALSE;
    }
}

static void
sm_app_window_init_channels(ScarlettMixerAppWindow *win, const gchar *card_name)
{
    ScarlettMixerAppWindowPrivate *priv;
    ScarlettMixerAppWindowInitArg *arg;

    g_debug("sm_app_window_init_channels.");
    priv = sm_app_window_get_instance_private(win);

    if(card_name)
    {
        gtk_label_set_label(priv->card_name_label, card_name);
    }
    arg = g_malloc0(sizeof(ScarlettMixerAppWindowInitArg));
    arg->priv = priv;
    arg->list = g_list_first(sm_app_get_channels(priv->app));
    g_idle_add(sm_app_window_init_strips, arg);

    arg = g_malloc0(sizeof(ScarlettMixerAppWindowInitArg));
    arg->priv = priv;
    arg->list = g_list_first(sm_app_get_input_sources(priv->app));
    g_idle_add(sm_app_window_init_input_sources, arg);
}

ScarlettMixerAppWindow *
sm_app_window_new(ScarlettMixerApp *app, const gchar* prefix)
{
    ScarlettMixerAppWindow *win;
    ScarlettMixerAppWindowPrivate *priv;

    g_debug("sm_app_window_new.");
    win = g_object_new(SCARLETTMIXER_APP_WINDOW_TYPE, "application", app, NULL);
    priv = sm_app_window_get_instance_private(win);
    priv->app = app;
    priv->prefix = prefix;
    g_timeout_add(SM_APP_WIN_INIT_TIMEOUT, sm_app_window_check_for_interface, (gpointer)win);
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
