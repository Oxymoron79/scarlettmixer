/*
 * sm-appwin.c - Main application window.
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

#include "sm-app.h"
#include "sm-appwin.h"
#include "sm-strip.h"
#include "sm-mix-strip.h"
#include "sm-channel.h"
#include "sm-source.h"
#include "sm-switch.h"

#define SM_APPWIN_INIT_TIMEOUT (100)
#define SM_APPWIN_BOX_MARGIN (6)
#define SM_APPWIN_BOX_PADDING (3)

struct _SmAppWin
{
    GtkApplicationWindow parent;
};

struct _SmAppWinClass
{
    GtkApplicationWindowClass parent_class;
};

typedef struct _SmAppWinPrivate SmAppWinPrivate;

struct _SmAppWinPrivate
{
    SmApp *app;
    const gchar* prefix;
    GtkFileFilter *file_filter;
    GtkLabel *card_name_label;
    GtkLabel *config_filename_label;
    GtkButton *open_config_button;
    GtkToggleButton *reveal_input_config_togglebutton;
    GtkButton *save_config_button;
    GtkMenuButton *config_menubutton;
    GtkStack *main_stack;
    GtkNotebook *output_mix_notebook;
    GList *mix_pages;
    GtkBox *output_channel_main_box;
    GtkBox *output_channel_box;
    GtkBox *input_sources_box;
    GtkBox *input_switches_box;
    GtkComboBoxText *sync_source_comboboxtext;
    GtkEntry *sync_status_entry;
};

typedef struct _SmAppWinInitArg SmAppWinInitArg;

struct _SmAppWinInitArg
{
    SmAppWinPrivate *priv;
    GList *list;
};

G_DEFINE_TYPE_WITH_PRIVATE(SmAppWin, sm_appwin,
        GTK_TYPE_APPLICATION_WINDOW);

// Forward declarations
static void
sm_appwin_init_channels(SmAppWin *win, const gchar *card_name);

static gboolean
sm_appwin_check_for_interface(gpointer win)
{
    SmAppWinPrivate *priv;
    gint card_number;
    const gchar *card_name;
    GtkWidget *msg_dialog;
    GError *err = NULL;

    priv = sm_appwin_get_instance_private(win);
    card_number = sm_app_find_card(priv->prefix);
    if (card_number >= 0)
    {
        GSettings *settings;
        gchar *configfile;
        card_name = sm_app_open_mixer(priv->app, card_number);
        //Load configuration from file set in settings
        settings = sm_app_get_settings(priv->app);
        configfile = g_settings_get_string(settings, "configfile");
        if (g_file_test(configfile, G_FILE_TEST_EXISTS) &&
            sm_app_read_config_file(priv->app, configfile, &err))
        {
            g_debug("Load configuration for from %s.", configfile);
            gtk_label_set_text(priv->config_filename_label, configfile);
        }
        else if (g_utf8_strlen(configfile, -1) > 0)
        {
            // Invalid configuration file is set in settings.
            // Show warning dialog.
            g_warning("Could not read configuration from %s.", configfile);
            msg_dialog = gtk_message_dialog_new(GTK_WINDOW(win),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_WARNING,
                    GTK_BUTTONS_CLOSE,
                    "Warning: Could not read configuration from %s!",
                    configfile);
            if (err == NULL)
            {
                gtk_message_dialog_format_secondary_text(
                        GTK_MESSAGE_DIALOG(msg_dialog),
                        "File not found.");
            }
            else
            {
                gtk_message_dialog_format_secondary_text(
                        GTK_MESSAGE_DIALOG(msg_dialog),
                        "%s", err->message);
            }
            gtk_dialog_run(GTK_DIALOG(msg_dialog));
            gtk_widget_destroy(msg_dialog);
        }
        g_free(configfile);
        sm_appwin_init_channels(win, card_name);
    }
    else
    {
        g_debug("No interface with prefix %s found.", priv->prefix);
        gtk_stack_set_visible_child_name(priv->main_stack, "error");
    }
    g_application_unmark_busy(G_APPLICATION(priv->app));
    return FALSE;
}

static void
refresh_button_clicked_cb(GtkButton *button, gpointer data)
{
    SmAppWin *win;
    SmAppWinPrivate *priv;

    g_debug("Refresh interface list.");
    win = SM_APPWIN(data);
    priv = sm_appwin_get_instance_private(win);
    g_application_mark_busy(G_APPLICATION(priv->app));
    gtk_stack_set_visible_child_name(priv->main_stack, "init");
    g_timeout_add(SM_APPWIN_INIT_TIMEOUT, sm_appwin_check_for_interface, (gpointer)win);
}

static void
reveal_input_config_togglebutton_toggled_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
    gboolean active;
    GtkRevealer *revealer = GTK_REVEALER(user_data);

    active = gtk_toggle_button_get_active(togglebutton);
    gtk_revealer_set_reveal_child(revealer, active);
}

static void
sm_appwin_dispose(GObject *object)
{
    SmAppWin *win;
    SmAppWinPrivate *priv;
    GList *li;

    win = SM_APPWIN(object);
    priv = sm_appwin_get_instance_private(win);
    g_debug("sm_appwin_dispose.");
    if (priv->mix_pages)
    {
        g_list_free(priv->mix_pages);
        priv->mix_pages = NULL;
    }
    if (priv->file_filter)
    {
        g_object_unref(priv->file_filter);
        priv->file_filter = NULL;
    }
    G_OBJECT_CLASS(sm_appwin_parent_class)->dispose(object);
}

static void
sm_appwin_class_init(SmAppWinClass *class)
{
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    G_OBJECT_CLASS(class)->dispose = sm_appwin_dispose;

    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_resource(GTK_CSS_PROVIDER(provider),
            "/org/alsa/scarlettmixer/sm-appwin.css");
    g_object_unref(provider);
    g_debug("sm_appwin_class_init.");
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/sm-appwin.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, card_name_label);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, config_filename_label);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, open_config_button);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, reveal_input_config_togglebutton);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, save_config_button);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, config_menubutton);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, main_stack);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, output_mix_notebook);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, output_channel_main_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, output_channel_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, input_sources_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, input_switches_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, sync_source_comboboxtext);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmAppWin, sync_status_entry);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            reveal_input_config_togglebutton_toggled_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            refresh_button_clicked_cb);
}

static void
sm_appwin_init(SmAppWin *win)
{
    g_debug("sm_appwin_init.");
    gtk_widget_init_template(GTK_WIDGET(win));
}

static void
sm_appwin_source_comboboxtext_changed_cb(GtkComboBox *combo, gpointer user_data)
{
    SmSource *src = SM_SOURCE(user_data);
    int active_idx;

    active_idx = gtk_combo_box_get_active(combo);
    sm_source_set_selected_item_index(src, active_idx);
}

static void
sm_appwin_source_changed_cb(SmSource *src, gpointer user_data)
{
    GtkComboBoxText *comboboxtext = GTK_COMBO_BOX_TEXT(user_data);
    int active_idx;

    active_idx = sm_source_get_selected_item_index(src);
    gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), active_idx);
}

static void
sm_appwin_switch_comboboxtext_changed_cb(GtkComboBox *combo, gpointer user_data)
{
    SmSwitch *sw = SM_SWITCH(user_data);
    int active_idx;

    active_idx = gtk_combo_box_get_active(combo);
    sm_switch_set_selected_item_index(sw, active_idx);
}

static void
sm_appwin_switch_changed_cb(SmSwitch *sw, gpointer user_data)
{
    GtkComboBoxText *comboboxtext = GTK_COMBO_BOX_TEXT(user_data);
    int active_idx;

    active_idx = sm_switch_get_selected_item_index(sw);
    gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), active_idx);
}

static void
sm_appwin_sync_changed_cb(SmSwitch *sw, gpointer user_data)
{
    gtk_entry_set_text(GTK_ENTRY(user_data), sm_switch_get_selected_item_name(sw));
}

static gboolean
sm_appwin_init_strips(gpointer data)
{
    SmAppWinInitArg *arg;
    SmChannel *ch;
    SmStrip *strip;
    SmMixStrip *mixstrip;
    GList *page, *item;
    GtkScrolledWindow *scrolled_win;
    GtkViewport *viewport;
    GtkBox *box;
    GtkLabel *label;
    gint idx;
    const gchar *mix_ids;
    gboolean pack_strip;

    arg = (SmAppWinInitArg*)data;
    ch = SM_CHANNEL(arg->list->data);
    switch (sm_channel_get_channel_type(ch))
    {
        case SM_CHANNEL_MASTER:
        {
            strip = sm_strip_new(ch);
            gtk_box_pack_end(arg->priv->output_channel_main_box, GTK_WIDGET(strip), FALSE, FALSE, 0);
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
            pack_strip = TRUE;
            mixstrip = NULL;
            page = NULL;
            item = NULL;
            box = NULL;
            for (page = g_list_first(arg->priv->mix_pages); page; page = g_list_next(page))
            {
                mix_ids = gtk_widget_get_name(GTK_WIDGET(page->data));
                if (mix_ids[0] != sm_channel_get_mix_id(ch) && mix_ids[1] != sm_channel_get_mix_id(ch))
                {
                    continue;
                }
                for (item = g_list_first(gtk_container_get_children(GTK_CONTAINER(page->data))); item; item = g_list_next(item))
                {
                    if (sm_mix_strip_add_channel(SM_MIX_STRIP(item->data), ch))
                    {
                        mixstrip = SM_MIX_STRIP(item->data);
                        pack_strip = FALSE;
                        break;
                    }
                }
                box = GTK_BOX(page->data);
                break;
            }
            if (!mixstrip)
            {
                mixstrip = sm_mix_strip_new(ch);
            }
            if (!page)
            {
                /* Create a new page widget */
                mix_ids = sm_mix_strip_get_mix_ids(mixstrip);
                box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SM_APPWIN_BOX_PADDING));
                gtk_widget_set_margin_start(GTK_WIDGET(box), SM_APPWIN_BOX_MARGIN);
                gtk_widget_set_margin_end(GTK_WIDGET(box), SM_APPWIN_BOX_MARGIN);
                gtk_widget_set_margin_top(GTK_WIDGET(box), SM_APPWIN_BOX_MARGIN);
                gtk_widget_set_margin_bottom(GTK_WIDGET(box), SM_APPWIN_BOX_MARGIN);
                gtk_widget_set_name(GTK_WIDGET(box), mix_ids);
                viewport = GTK_VIEWPORT(gtk_viewport_new(NULL, NULL));
                gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(box));
                scrolled_win = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
                gtk_scrolled_window_set_policy(scrolled_win, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
                gtk_container_add(GTK_CONTAINER(scrolled_win), GTK_WIDGET(viewport));
                gtk_widget_show_all(GTK_WIDGET(scrolled_win));
                arg->priv->mix_pages = g_list_prepend(arg->priv->mix_pages, box);
                label = GTK_LABEL(gtk_label_new(g_strdup_printf("Mix %c & %c",mix_ids[0], mix_ids[1])));
                idx = gtk_notebook_append_page(arg->priv->output_mix_notebook, GTK_WIDGET(scrolled_win), GTK_WIDGET(label));
            }
            if (pack_strip)
            {
                gtk_box_pack_start(box, GTK_WIDGET(mixstrip), FALSE, FALSE, 0);
                gtk_widget_show(GTK_WIDGET(box));
            }
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
        arg->priv->mix_pages = g_list_reverse(arg->priv->mix_pages);
        gtk_stack_set_visible_child_name(arg->priv->main_stack, "output");
        g_free(arg);
        return FALSE;
    }
}

static gboolean
sm_appwin_init_input_sources(gpointer data)
{
    SmAppWinInitArg *arg;
    SmSource *src;
    GList *item;
    GtkBox *box;
    GtkLabel *label;
    GtkComboBoxText *comboboxtext;
    GtkStyleContext *style_ctx;
    gint idx;
    gchar *name;

    arg = (SmAppWinInitArg*)data;
    src = SM_SOURCE(arg->list->data);
    box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, SM_APPWIN_BOX_PADDING));
    sscanf(sm_source_get_name(src), "Input Source %02u", &idx);
    name = g_strdup_printf("Input %d", idx);
    label = GTK_LABEL(gtk_label_new(name));
    g_free(name);
    gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
    comboboxtext = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    style_ctx = gtk_widget_get_style_context(GTK_WIDGET(comboboxtext));
    gtk_style_context_add_class(style_ctx, "small-text");
    for (item = g_list_first(sm_source_get_item_names(src)); item; item = g_list_next(item))
    {
        gtk_combo_box_text_append_text(comboboxtext, item->data);
        g_free(item->data);
    }
    idx = sm_source_get_selected_item_index(src);
    if (idx < 0) {
        g_warning("Could not get active enum item!");
    }
    else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), idx);
    }
    g_signal_connect(GTK_WIDGET(comboboxtext), "changed", G_CALLBACK(sm_appwin_source_comboboxtext_changed_cb), src);
    g_signal_connect(src, "changed", G_CALLBACK(sm_appwin_source_changed_cb), comboboxtext);
    gtk_box_pack_start(box, GTK_WIDGET(comboboxtext), FALSE, FALSE, 0);
    gtk_box_pack_start(arg->priv->input_sources_box, GTK_WIDGET(box), FALSE, FALSE, 0);
    arg->list = g_list_next(arg->list);
    if (arg->list)
    {
        return TRUE;
    }
    else
    {
        gtk_widget_show(GTK_WIDGET(arg->priv->open_config_button));
        gtk_widget_show(GTK_WIDGET(arg->priv->reveal_input_config_togglebutton));
        gtk_widget_show(GTK_WIDGET(arg->priv->save_config_button));
        gtk_widget_show(GTK_WIDGET(arg->priv->config_menubutton));
        gtk_widget_show_all(GTK_WIDGET(arg->priv->input_sources_box));
        g_free(arg);
        return FALSE;
    }
}

static gboolean
sm_appwin_init_input_switches(gpointer data)
{
    SmAppWinInitArg *arg;
    SmSwitch *sw;
    GList *item;
    GtkBox *box;
    GtkLabel *label;
    GtkComboBoxText *comboboxtext;
    GtkStyleContext *style_ctx;
    gint idx, *switch_id;
    gchar *name;
    gboolean new_box;

    arg = (SmAppWinInitArg*)data;
    sw = SM_SWITCH(arg->list->data);
    idx = sm_switch_get_id(sw);
    item = gtk_container_get_children(GTK_CONTAINER(arg->priv->input_switches_box));
    new_box = TRUE;
    for (item = g_list_first(item); item; item = g_list_next(item))
    {
        box = GTK_BOX(item->data);
        if (*((gint*)g_object_get_data(G_OBJECT(box), "switch_id")) == idx)
        {
            new_box = FALSE;
            break;
        }
    }
    if (new_box)
    {
        box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, SM_APPWIN_BOX_PADDING));
        switch_id = g_malloc0(sizeof(gint));
        *switch_id = idx;
        g_object_set_data(G_OBJECT(box), "switch_id", (gpointer)switch_id);
        new_box = TRUE;
    }
    label = GTK_LABEL(gtk_label_new(sm_switch_get_name(sw)));
    gtk_box_pack_start(box, GTK_WIDGET(label), FALSE, FALSE, 0);
    comboboxtext = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    style_ctx = gtk_widget_get_style_context(GTK_WIDGET(comboboxtext));
    gtk_style_context_add_class(style_ctx, "small-text");
    for (item = g_list_first(sm_switch_get_item_names(sw)); item; item = g_list_next(item))
    {
        gtk_combo_box_text_append_text(comboboxtext, item->data);
        g_free(item->data);
    }
    idx = sm_switch_get_selected_item_index(sw);
    if (idx < 0) {
        g_warning("Could not get active enum item!");
    }
    else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(comboboxtext), idx);
    }
    g_signal_connect(GTK_WIDGET(comboboxtext), "changed", G_CALLBACK(sm_appwin_switch_comboboxtext_changed_cb), sw);
    g_signal_connect(sw, "changed", G_CALLBACK(sm_appwin_switch_changed_cb), comboboxtext);
    gtk_box_pack_start(box, GTK_WIDGET(comboboxtext), FALSE, FALSE, 0);
    if (new_box)
    {
        gtk_box_pack_start(arg->priv->input_switches_box, GTK_WIDGET(box), FALSE, FALSE, 0);
    }
    arg->list = g_list_next(arg->list);
    if (arg->list)
    {
        return TRUE;
    }
    else
    {
        gtk_widget_show(GTK_WIDGET(arg->priv->reveal_input_config_togglebutton));
        gtk_widget_show_all(GTK_WIDGET(arg->priv->input_switches_box));
        g_free(arg);
        return FALSE;
    }
}

static void
sm_appwin_init_channels(SmAppWin *win, const gchar *card_name)
{
    SmAppWinPrivate *priv;
    SmAppWinInitArg *arg;
    SmSwitch *sw;
    GList *item;
    gint idx;

    g_debug("sm_appwin_init_channels.");
    priv = sm_appwin_get_instance_private(win);

    if(card_name)
    {
        gtk_label_set_label(priv->card_name_label, card_name);
    }
    arg = g_malloc0(sizeof(SmAppWinInitArg));
    arg->priv = priv;
    arg->list = g_list_first(sm_app_get_channels(priv->app));
    g_idle_add(sm_appwin_init_strips, arg);

    arg = g_malloc0(sizeof(SmAppWinInitArg));
    arg->priv = priv;
    arg->list = g_list_first(sm_app_get_input_sources(priv->app));
    g_idle_add(sm_appwin_init_input_sources, arg);

    arg = g_malloc0(sizeof(SmAppWinInitArg));
    arg->priv = priv;
    arg->list = g_list_first(sm_app_get_input_switches(priv->app));
    g_idle_add(sm_appwin_init_input_switches, arg);

    sw = sm_app_get_clock_source(priv->app);
    for (item = g_list_first(sm_switch_get_item_names(sw)); item; item = g_list_next(item))
    {
        gtk_combo_box_text_append_text(priv->sync_source_comboboxtext, item->data);
        g_free(item->data);
    }
    idx = sm_switch_get_selected_item_index(sw);
    if (idx < 0) {
        g_warning("Could not get active enum item!");
    }
    else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(priv->sync_source_comboboxtext), idx);
    }
    g_signal_connect(GTK_WIDGET(priv->sync_source_comboboxtext), "changed", G_CALLBACK(sm_appwin_switch_comboboxtext_changed_cb), sw);
    g_signal_connect(sw, "changed", G_CALLBACK(sm_appwin_switch_changed_cb), priv->sync_source_comboboxtext);

    sw = sm_app_get_sync_status(priv->app);
    gtk_entry_set_text(GTK_ENTRY(priv->sync_status_entry), sm_switch_get_selected_item_name(sw));
    g_signal_connect(sw, "changed", G_CALLBACK(sm_appwin_sync_changed_cb), priv->sync_status_entry);
}

SmAppWin *
sm_appwin_new(SmApp *app, const gchar* prefix)
{
    SmAppWin *win;
    SmAppWinPrivate *priv;
    GtkFileFilter *file_filter;

    g_debug("sm_appwin_new.");
    win = g_object_new(SM_APPWIN_TYPE, "application", app, NULL);
    priv = sm_appwin_get_instance_private(win);
    priv->app = app;
    priv->prefix = prefix;
    file_filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type(file_filter, "application/json");
    gtk_file_filter_set_name(file_filter, "All JSON Files");
    priv->file_filter = g_object_ref_sink(file_filter);
    g_timeout_add(SM_APPWIN_INIT_TIMEOUT, sm_appwin_check_for_interface, (gpointer)win);
    return win;
}

GtkFileFilter*
sm_appwin_get_file_filter(SmAppWin *win)
{
    SmAppWinPrivate *priv;

    priv = sm_appwin_get_instance_private(win);
    return g_object_ref(priv->file_filter);
}

void
sm_appwin_open_configfile(SmAppWin *win)
{
    SmAppWinPrivate *priv;
    SmApp *app;
    GtkWidget *dialog;
    GtkWidget *msg_dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;
    GError *err = NULL;

    priv = sm_appwin_get_instance_private(win);
    app = SM_APP(priv->app);
    dialog = gtk_file_chooser_dialog_new("Open Configuration",
            GTK_WINDOW(win), action,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT,
            NULL);
    chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_add_filter(chooser, priv->file_filter);
    gtk_file_chooser_set_current_folder(chooser, g_get_home_dir());

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename(chooser);
        gtk_widget_destroy(dialog);
        g_debug("Read configuration from %s.", filename);
        if(sm_app_read_config_file(app, filename, &err))
        {
            gtk_label_set_text(priv->config_filename_label, filename);
        }
        else
        {
            g_warning("Could not read configuration from %s.", filename);
            msg_dialog = gtk_message_dialog_new(GTK_WINDOW(win),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error: Could not read configuration from %s!",
                    filename);
            gtk_message_dialog_format_secondary_text(
                    GTK_MESSAGE_DIALOG(msg_dialog),
                    "%s", err->message);
            gtk_dialog_run(GTK_DIALOG(msg_dialog));
            gtk_widget_destroy(msg_dialog);
        }
        g_free(filename);
    }
    else
    {
        gtk_widget_destroy(dialog);
    }
}

void
sm_appwin_save_configfile(SmAppWin *win)
{
    //TODO: Test save procedure
    SmAppWinPrivate *priv;
    SmApp *app;
    GtkWidget *msg_dialog;
    const char *config_filename;
    GError *err = NULL;

    priv = sm_appwin_get_instance_private(win);
    app = SM_APP(priv->app);
    config_filename = gtk_label_get_text(priv->config_filename_label);
    if (g_file_test(config_filename, G_FILE_TEST_EXISTS))
    {
        g_debug("Save configuration to %s.", config_filename);
        if (!sm_app_write_config_file(app, config_filename, &err))
        {
            g_warning("Could not write configuration file %s.", config_filename);
            msg_dialog = gtk_message_dialog_new(GTK_WINDOW(win),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error: Could not write configuration file %s!",
                    config_filename);
            gtk_message_dialog_format_secondary_text(
                    GTK_MESSAGE_DIALOG(msg_dialog),
                    "%s", err->message);
            gtk_dialog_run(GTK_DIALOG(msg_dialog));
            gtk_widget_destroy(msg_dialog);
        }
    }
    else
    {
        sm_appwin_saveas_configfile(win);
    }
}

void
sm_appwin_saveas_configfile(SmAppWin *win)
{
    //TODO: Test saveas procedure
    SmAppWinPrivate *priv;
    SmApp *app;
    GtkWidget *dialog;
    GtkWidget *msg_dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    const char *config_filename;
    gint res;
    GError *err = NULL;

    priv = sm_appwin_get_instance_private(win);
    app = SM_APP(priv->app);
    dialog = gtk_file_chooser_dialog_new("Save Configuration",
            GTK_WINDOW(win), action,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL);
    chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_add_filter(chooser, priv->file_filter);

    config_filename = gtk_label_get_text(priv->config_filename_label);
    if (g_file_test(config_filename, G_FILE_TEST_EXISTS))
    {
        gtk_file_chooser_set_filename(chooser, config_filename);
    }
    else
    {
        gtk_file_chooser_set_current_name(chooser, "MixerConfig.json");
    }

    res = gtk_dialog_run(GTK_DIALOG (dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(chooser);
        gtk_widget_destroy(dialog);
        g_debug("Save configuration to %s.", filename);
        if (sm_app_write_config_file(app, filename, &err))
        {
            gtk_label_set_text(priv->config_filename_label, filename);
        }
        else
        {
            g_warning("Could not write configuration file %s.", filename);
            msg_dialog = gtk_message_dialog_new(GTK_WINDOW(win),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error: Could not write configuration file %s!",
                    filename);
            gtk_message_dialog_format_secondary_text(
                    GTK_MESSAGE_DIALOG(msg_dialog),
                    "%s", err->message);
            gtk_dialog_run(GTK_DIALOG(msg_dialog));
            gtk_widget_destroy(msg_dialog);
        }
        g_free(filename);
    }
    else
    {
        gtk_widget_destroy(dialog);
    }
}
