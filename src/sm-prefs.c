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

#include "sm-prefs.h"
#include "sm-app.h"

struct _SmPrefsClass
{
    GtkDialogClass parent_class;
};

struct _SmPrefs
{
    GtkDialog parent;
};

typedef struct _SmPrefsPrivate SmPrefsPrivate;

struct _SmPrefsPrivate
{
    GSettings *settings;
    GtkFileChooserButton *configfile_fchbtn;
    GtkLabel *compatible_card_lbl;
};

G_DEFINE_TYPE_WITH_PRIVATE(SmPrefs, sm_prefs, GTK_TYPE_WINDOW);

static void
configfile_fchbtn_selection_changed_cb(GtkFileChooser *button, gpointer data)
{
    SmPrefs *prefs;
    SmPrefsPrivate *priv;
    gchar *card_name;
    GError *err = NULL;
    gchar *configfile;
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));

    prefs = SM_PREFS(data);
    priv = sm_prefs_get_instance_private(prefs);
    if (filename)
    {
        configfile = g_settings_get_string(priv->settings, "configfile");
        if (g_strcmp0(configfile, filename) != 0)
        {
            g_debug("Config file preference changed: %s", filename);
            g_settings_set_string(priv->settings, "configfile", filename);
        }
        g_free(configfile);
        card_name = sm_app_read_card_name_from_config_file(filename, &err);
        g_free(filename);
        if (card_name != NULL)
        {
            gtk_label_set_text(priv->compatible_card_lbl, card_name);
            g_free(card_name);
        }
        else
        {
            gtk_label_set_text(priv->compatible_card_lbl, err->message);
        }
    }
    else
    {
        g_debug("Config file preference cleared:");
        g_settings_set_string(priv->settings, "configfile", "");
        gtk_label_set_text(priv->compatible_card_lbl, "");
    }
}

static void
clear_btn_clicked_cb(GtkButton *button, gpointer data)
{
    SmPrefs *prefs;
    SmPrefsPrivate *priv;

    g_debug("Clear config file preference.");
    prefs = SM_PREFS(data);
    priv = sm_prefs_get_instance_private(prefs);
    gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(priv->configfile_fchbtn));
}

static void
sm_prefs_dispose(GObject *object)
{
    SmPrefsPrivate *priv;

    priv = sm_prefs_get_instance_private(SM_PREFS(object));
    G_OBJECT_CLASS(sm_prefs_parent_class)->dispose(object);
}

static void
sm_prefs_class_init(SmPrefsClass *class)
{
    G_OBJECT_CLASS(class)->dispose = sm_prefs_dispose;

    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
            "/org/alsa/scarlettmixer/sm-prefsdialog.ui");

    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmPrefs, configfile_fchbtn);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
            SmPrefs, compatible_card_lbl);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            clear_btn_clicked_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
            configfile_fchbtn_selection_changed_cb);
}

SmPrefs*
sm_prefs_new(SmAppWin *win)
{
    SmPrefs *prefs;
    SmPrefsPrivate *priv;
    GObject* app;
    gchar *configfile;

    prefs = g_object_new(SM_PREFS_TYPE, "transient-for", win, NULL);
    priv = sm_prefs_get_instance_private(prefs);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(priv->configfile_fchbtn), sm_appwin_get_file_filter(win));
    // Get GSettings from application
    g_object_get(win, "application", &app, NULL);
    priv->settings = sm_app_get_settings(SM_APP(app));
    configfile = g_settings_get_string(priv->settings, "configfile");
    if (g_utf8_strlen(configfile, -1) > 0)
    {
        g_debug("sm_prefs_new: configfile: %s", configfile);
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(priv->configfile_fchbtn), configfile);
    }
    g_free(configfile);
    return prefs;
}

static void
sm_prefs_init(SmPrefs *prefs)
{
    SmPrefsPrivate *priv;

    priv = sm_prefs_get_instance_private(prefs);
    gtk_widget_init_template(GTK_WIDGET(prefs));
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(priv->configfile_fchbtn), g_get_home_dir());
}
