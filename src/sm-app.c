/*
 * sm-app.c - GtkApplication class.
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
#include <alsa/asoundlib.h>
#include <json-glib/json-glib.h>
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include "sm-app.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "sm-prefs.h"
#include "sm-appwin.h"
#include "sm-channel.h"
#include "sm-source.h"
#include "sm-switch.h"

struct _SmAppClass
{
    GtkApplicationClass parent_class;
};

struct _SmApp
{
    GtkApplication parent;

    GSettings *settings;
    snd_hctl_t *hctl;
    snd_ctl_card_info_t *card_info;
    const char *card_name;
    snd_mixer_t *mixer;
    GList *channels;
    GList *input_sources;
    GList *input_switches;
    SmSwitch *clock_source;
    SmSwitch *sync_status;
    SmSwitch *usb_sync;
};

G_DEFINE_TYPE(SmApp, sm_app, GTK_TYPE_APPLICATION);

static const gchar *prefix_scarlett = "Scarlett";
static const gchar *prefix_intel = "HDA Intel";
static const gchar *prefix = "Scarlett";


static void
sm_app_preferences_activated(GSimpleAction *action,
        GVariant *parameter,
        gpointer app)
{
    SmApp *sm_app;
    GList *windows;
    SmAppWin *win;
    SmPrefs *prefs;

    g_debug("sm_app_preferences_activated.");
    sm_app = SM_APP(app);
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    win = SM_APPWIN(g_list_first(windows)->data);
    prefs = sm_prefs_new(win);
    gtk_window_present(GTK_WINDOW(prefs));
}

static void
sm_app_about_activated(GSimpleAction *action,
        GVariant *parameter,
        gpointer app)
{
    GList *windows = NULL;
    GtkWindow *win;
    const gchar *authors[2];
    authors[0] = "Martin Rösch";
    authors[1] = NULL;

    g_debug("about_activated");
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    win = GTK_WINDOW(g_list_first(windows)->data);
    gtk_show_about_dialog(win,
            "logo-icon-name", gtk_window_get_icon_name(win),
            "authors", authors,
            "version", PACKAGE_VERSION,
            "comments", "Mixer for the Scarlett USB audio interfaces.",
            "copyright", "Copyright 2016 - Martin Rösch",
            "license-type", GTK_LICENSE_GPL_3_0,
            NULL);
}

static void
sm_app_quit_activated(GSimpleAction *action,
        GVariant *parameter,
        gpointer app)
{
  g_application_quit(G_APPLICATION(app));
}

static GActionEntry app_actions[] =
{
    { "preferences", sm_app_preferences_activated, NULL, NULL, NULL },
    { "about", sm_app_about_activated, NULL, NULL, NULL },
    { "quit", sm_app_quit_activated, NULL, NULL, NULL }
};

static void
sm_app_activate(GApplication *app)
{
    SmApp *sm_app;
    GtkWindow *gtkwin;
    SmAppWin *win;

    g_debug("sm_app_activate.");

    sm_app = SM_APP(app);
    gtkwin = gtk_application_get_active_window(GTK_APPLICATION(app));
    if (!gtkwin)
    {
        g_application_mark_busy(G_APPLICATION(app));
        win = sm_appwin_new(sm_app, prefix);
        gtkwin = GTK_WINDOW(win);
    }
    gtk_window_present(gtkwin);
}

static void
sm_app_startup(GApplication *app)
{
    SmApp *sm_app;
    gchar *path;
    GSettingsBackend *gs_backend;
    GtkBuilder *builder;
    GMenuModel *app_menu;
    const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };

    g_debug("sm_app_startup.");
    sm_app = SM_APP(app);

    G_APPLICATION_CLASS(sm_app_parent_class)->startup(app);

    g_action_map_add_action_entries(G_ACTION_MAP(app),
            app_actions,
            G_N_ELEMENTS(app_actions),
            app);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app),
            "app.quit",
            quit_accels);

    //Setup keyfile backend for GSettings in XDG_CONFIG_HOME
    path = g_build_filename(g_get_user_config_dir(), PACKAGE, "config", NULL);
    gs_backend = g_keyfile_settings_backend_new(path, "/org/alsa/scarlettmixer/", "Preferences");
    sm_app->settings = g_settings_new_with_backend("org.alsa.scarlettmixer", gs_backend);

    builder = gtk_builder_new_from_resource("/org/alsa/scarlettmixer/sm-appmenu.ui");
    app_menu = G_MENU_MODEL(gtk_builder_get_object(builder, "app_menu"));
    gtk_application_set_app_menu(GTK_APPLICATION(app), app_menu);
    g_object_unref(builder);
}

static void
sm_app_shutdown(GApplication *app)
{
    SmApp *sm_app;
    GList* item;
    int err;

    g_debug("sm_app_shutdown.");
    sm_app = SM_APP(app);

    for (item = g_list_first(sm_app->channels); item; item = g_list_next(item))
    {
        g_object_unref(item->data);
    }
    g_list_free(sm_app->channels);
    sm_app->channels = NULL;
    for (item = g_list_first(sm_app->input_sources); item; item = g_list_next(item))
    {
        g_object_unref(item->data);
    }
    g_list_free(sm_app->input_sources);
    sm_app->input_sources = NULL;
    for (item = g_list_first(sm_app->input_switches); item; item = g_list_next(item))
    {
        g_object_unref(item->data);
    }
    g_list_free(sm_app->input_switches);
    sm_app->input_switches = NULL;
    if (sm_app->clock_source)
    {
        g_object_unref(sm_app->clock_source);
    }
    sm_app->clock_source = NULL;
    if (sm_app->sync_status)
    {
        g_object_unref(sm_app->sync_status);
    }
    sm_app->sync_status = NULL;
    if (sm_app->usb_sync)
    {
        g_object_unref(sm_app->usb_sync);
    }
    sm_app->usb_sync = NULL;
    if (sm_app->card_info)
    {
        snd_ctl_card_info_free(sm_app->card_info);
    }
    if (sm_app->mixer)
    {
        err = snd_mixer_close(sm_app->mixer);
        if (err < 0)
        {
            g_debug("sm_app_shutdown: Failed to close mixer: %s", snd_strerror(err));
        }
    }
    G_APPLICATION_CLASS(sm_app_parent_class)->shutdown(app);
}

static void
sm_app_class_init(SmAppClass *class)
{
    g_debug("sm_app_class_init.");
    g_set_prgname(PACKAGE_NAME);
    g_set_application_name(PACKAGE_NAME);
    G_APPLICATION_CLASS(class)->activate = sm_app_activate;
    G_APPLICATION_CLASS(class)->startup = sm_app_startup;
    G_APPLICATION_CLASS(class)->shutdown = sm_app_shutdown;
}

gint
sm_app_find_card(const gchar* prefix)
{
    gint number, err, ret;
    snd_ctl_t *ctl;
    snd_ctl_card_info_t *cinfo;
    gchar hw_buf[8];

    snd_ctl_card_info_malloc(&cinfo);
    number = -1;
    while (1)
    {
        err = snd_card_next(&number);
        if (err < 0)
        {
            // Cannot enumerate sound cards
            ret = err;
            break;
        }
        if (number == -1)
        {
            // No Scarlett sound card found
            ret = number;
            break;
        }
        sprintf(hw_buf, "hw:%d", number);
        err = snd_ctl_open(&ctl, hw_buf, 0);
        if (err < 0)
        {
            // Cannot open sound card
            continue;
        }
        err = snd_ctl_card_info(ctl, cinfo);
        if (err < 0)
        {
            // Cannot read info for sound card
            snd_ctl_close(ctl);
            continue;
        }
        snd_ctl_close(ctl);
        if (g_str_has_prefix(snd_ctl_card_info_get_name(cinfo), prefix))
        {
            ret = number;
            break;
        }
    }
    snd_ctl_card_info_free(cinfo);
    return ret;
}

static int
sm_app_mixer_elem_callback(snd_mixer_elem_t *elem, unsigned int mask)
{
    SmApp *app;
    SmChannel *ch;
    SmSource *src;
    SmSwitch *sw;
    GList *list;

    if (mask == SND_CTL_EVENT_MASK_REMOVE)
    {
        g_debug("sm_app_mixer_elem_callback: %s removed",
                snd_mixer_selem_get_name(elem));
    }
    if (mask & SND_CTL_EVENT_MASK_VALUE)
    {
        g_debug("sm_app_mixer_elem_callback: %s value changed.",
                        snd_mixer_selem_get_name(elem));
        app = SM_APP(g_application_get_default());
        if (!app)
        {
            g_debug("sm_app_mixer_elem_callback: app == NULL");
            return 0;
        }
        for (list = g_list_first(app->channels); list; list = g_list_next(list))
        {
            ch = SM_CHANNEL(list->data);
            if (sm_channel_has_mixer_elem(ch, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Channel %s has element %s",
                        sm_channel_get_name(ch),
                        snd_mixer_selem_get_name(elem));
                sm_channel_mixer_elem_changed(ch, elem);
            }
        }
        for (list = g_list_first(app->input_sources); list; list = g_list_next(list))
        {
            src = SM_SOURCE(list->data);
            if (sm_source_has_mixer_elem(src, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Source %s has element %s",
                        sm_source_get_name(src),
                        snd_mixer_selem_get_name(elem));
                sm_source_mixer_elem_changed(src, elem);
                break;
            }
        }
        for (list = g_list_first(app->input_switches); list; list = g_list_next(list))
        {
            sw = SM_SWITCH(list->data);
            if (sm_switch_has_mixer_elem(sw, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Switch %s has element %s",
                        sm_switch_get_name(sw),
                        snd_mixer_selem_get_name(elem));
                sm_switch_mixer_elem_changed(sw, elem);
                break;
            }
        }
        if (app->clock_source)
        {
            if (sm_switch_has_mixer_elem(app->clock_source, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Switch %s has element %s",
                        sm_switch_get_name(app->clock_source),
                        snd_mixer_selem_get_name(elem));
                sm_switch_mixer_elem_changed(app->clock_source, elem);
            }
        }
        if (app->sync_status)
        {
            if (sm_switch_has_mixer_elem(app->sync_status, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Switch %s has element %s",
                        sm_switch_get_name(app->sync_status),
                        snd_mixer_selem_get_name(elem));
                sm_switch_mixer_elem_changed(app->sync_status, elem);
            }
        }
    }
    if (mask & SND_CTL_EVENT_MASK_INFO)
    {
        g_debug("sm_app_mixer_elem_callback: %s info changed.",
                snd_mixer_selem_get_name(elem));
    }
    if (mask & SND_CTL_EVENT_MASK_ADD)
    {
        g_debug("sm_app_mixer_elem_callback: %s added.",
                snd_mixer_selem_get_name(elem));
    }
    return 0;
}

static int
sm_app_mixer_callback(snd_mixer_t *mixer,
        unsigned int mask,
        snd_mixer_elem_t *elem)
{
    if (mask & SND_CTL_EVENT_MASK_REMOVE)
    {
        g_debug("sm_app_mixer_callback: %s removed.",
                snd_mixer_selem_get_name(elem));
    }
    if (mask & SND_CTL_EVENT_MASK_VALUE)
    {
        g_debug("sm_app_mixer_callback: %s value changed.",
                snd_mixer_selem_get_name(elem));
    }
    if (mask & SND_CTL_EVENT_MASK_INFO)
    {
        g_debug("sm_app_mixer_callback: %s info changed.",
                snd_mixer_selem_get_name(elem));
    }
    if (mask & SND_CTL_EVENT_MASK_ADD)
    {
        g_debug("sm_app_mixer_callback: %s added.",
                snd_mixer_selem_get_name(elem));
        snd_mixer_elem_set_callback(elem, sm_app_mixer_elem_callback);
    }
    return 0;
}

static gboolean
sm_app_gioch_mixer_callback(GIOChannel *source,
        GIOCondition condition,
        gpointer data)
{
    SmApp *app = SM_APP(data);
    snd_mixer_handle_events(app->mixer);
    return TRUE;
}

const gchar*
sm_app_open_mixer(SmApp *app, int card_number)
{
    int err, idx;
    struct snd_mixer_selem_regopt selem_regopt = {
            .ver = 1,
            .abstract = SND_MIXER_SABSTRACT_NONE,
            .device = g_strdup_printf("hw:%d", card_number)
    };
    snd_mixer_elem_t *elem;
    GList *item;
    int npfds;
    struct pollfd *pfds;
    GIOChannel *gioch;

    err = snd_mixer_open(&(app->mixer), 0);
    if (err < 0)
    {
        g_critical("Cannot open mixer: %s", snd_strerror(err));
        g_free((gpointer)selem_regopt.device);
        return NULL;
    }

    err = snd_mixer_selem_register(app->mixer, &selem_regopt, NULL);
    if (err < 0)
    {
        g_critical("Cannot register simple mixer: %s", snd_strerror(err));
        g_free((gpointer)selem_regopt.device);
        snd_mixer_close(app->mixer);
        return NULL;
    }
    snd_mixer_set_callback(app->mixer, sm_app_mixer_callback);

    err = snd_mixer_load(app->mixer);
    if (err < 0)
    {
        g_critical("Cannot load mixer controls: %s", snd_strerror(err));
        g_free((gpointer)selem_regopt.device);
        snd_mixer_close(app->mixer);
        return NULL;
    }

    err = snd_mixer_get_hctl(app->mixer, selem_regopt.device, &(app->hctl));
    g_free((gpointer)selem_regopt.device);
    if (err < 0)
    {
        g_critical("Cannot get HCTL: %s", snd_strerror(err));
        snd_mixer_close(app->mixer);
        return NULL;
    }/*
    else
    {
        snd_hctl_elem_t *hel;
        g_debug("hctl: %d elements.", snd_hctl_get_count(app->hctl));
        for(hel=snd_hctl_first_elem(app->hctl); hel; hel=snd_hctl_elem_next(hel))
        {
            g_debug("list helem: numid=%u, name=%s, interface=%s",
                    snd_hctl_elem_get_numid(hel),
                    snd_hctl_elem_get_name(hel),
                    snd_ctl_elem_iface_name(snd_hctl_elem_get_interface(hel)));
        }
    }*/
    snd_ctl_card_info_malloc(&(app->card_info));
    err = snd_ctl_card_info(snd_hctl_ctl(app->hctl), app->card_info);
    if (err < 0)
    {
        g_critical("Cannot read information from sound card: %s", snd_strerror(err));
        snd_ctl_card_info_free(app->card_info);
        snd_mixer_close(app->mixer);
        snd_hctl_free(app->hctl);
        return NULL;
    }
    app->card_name = snd_ctl_card_info_get_name(app->card_info);

    npfds = snd_mixer_poll_descriptors_count(app->mixer);
    if (npfds > 0) {
        pfds = malloc(sizeof(*pfds) * npfds);
        npfds = snd_mixer_poll_descriptors(app->mixer, pfds, npfds);
        for (idx = 0; idx < npfds; idx++) {
            gioch = g_io_channel_unix_new(pfds[idx].fd);
            g_io_add_watch(gioch, G_IO_IN, sm_app_gioch_mixer_callback, app);
        }
        g_free(pfds);
    }
/*
    g_debug("%d mixer elements.", snd_mixer_get_count(mixer));
    g_debug("%d hctl elements.", snd_hctl_get_count(hctl));
*/
    for (elem = snd_mixer_first_elem(app->mixer);
            elem;
            elem = snd_mixer_elem_next(elem))
    {
        gboolean elem_added = FALSE;
        for (item = g_list_first(app->channels);
                item;
                item = g_list_next(item))
        {
            elem_added = sm_channel_add_mixer_elem(SM_CHANNEL(item->data), elem);
            if (elem_added) {
                g_debug("Added mixer element %s to channel %s.",
                        snd_mixer_selem_get_name(elem),
                        sm_channel_get_name(SM_CHANNEL(item->data)));
                break;
            }
        }
        if (!elem_added) {
            if (snd_mixer_selem_is_enumerated(elem)
                    && !snd_mixer_selem_is_enum_playback(elem)
                    && !snd_mixer_selem_is_enum_capture(elem))
            {
                /* Input Switch */
                SmSwitch *sw = sm_switch_new();
                if (sm_switch_add_mixer_elem(sw, elem))
                {
                    g_debug("Created input switch for mixer element %s.",
                            snd_mixer_selem_get_name(elem));
                    switch (sm_switch_get_switch_type(sw))
                    {
                        case SM_SWITCH_INPUT_IMPEDANCE:
                        case SM_SWITCH_INPUT_PAD:
                            app->input_switches = g_list_prepend(app->input_switches, sw);
                            break;
                        case SM_SWITCH_CLOCK_SOURCE:
                            app->clock_source = sw;
                            break;
                        case SM_SWITCH_SYNC_STATUS:
                            app->sync_status = sw;
                            break;
                        case SM_SWITCH_USB_SYNC:
                            app->usb_sync = sw;
                            break;
                        default:
                            g_warning("Unhandled switch: %s", sm_switch_get_name(sw));
                            g_object_unref(sw);
                            break;
                    }
                }
            }
            else if (snd_mixer_selem_is_enumerated(elem)
                    && !snd_mixer_selem_is_enum_playback(elem)
                    && snd_mixer_selem_is_enum_capture(elem))
            {
                /* Input source */
                SmSource *src = sm_source_new();
                if (sm_source_add_mixer_elem(src, elem))
                {
                    g_debug("Created input source for mixer element %s.",
                            snd_mixer_selem_get_name(elem));
                    app->input_sources = g_list_prepend(app->input_sources, src);
                }
            }
            else
            {
                /* Channel */
                SmChannel *ch = sm_channel_new();
                if (sm_channel_add_mixer_elem(ch, elem))
                {
                    g_debug("Created channel for mixer element %s.",
                            snd_mixer_selem_get_name(elem));
                    g_debug("    Type: %d.", sm_channel_get_channel_type(ch));
                    g_debug("    Name: %s.", sm_channel_get_name(ch));
                    g_debug("    Has left channel: %d.",
                            sm_channel_has_volume(ch, SND_MIXER_SCHN_FRONT_LEFT));
                    g_debug("    Has right channel: %d.",
                            sm_channel_has_volume(ch, SND_MIXER_SCHN_FRONT_RIGHT));
                    app->channels = g_list_prepend(app->channels, ch);
                }
                else
                {
                    g_warning("Could not create channel for mixer element %s",
                            snd_mixer_selem_get_name(elem));
                    g_object_unref(ch);
                }
            }
        }
    }
    app->input_switches = g_list_reverse(app->input_switches);
    app->input_sources = g_list_reverse(app->input_sources);
    app->channels = g_list_reverse(app->channels);
    /* Go through list again to add remaining Matrix XX Input Sources */
    for (elem = snd_mixer_first_elem(app->mixer); elem; elem = snd_mixer_elem_next(elem))
    {
        gboolean elem_added;
        if (!snd_mixer_selem_is_enumerated(elem)
                || !snd_mixer_selem_is_enum_playback(elem))
        {
            continue;
        }
        for (item = g_list_first(app->channels); item; item = g_list_next(item))
        {
            elem_added = sm_channel_add_mixer_elem(SM_CHANNEL(item->data), elem);
            if (elem_added) {
                g_debug("Added mixer element %s to channel %s.",
                        snd_mixer_selem_get_name(elem),
                        sm_channel_get_name(SM_CHANNEL(item->data)));
            }
        }
    }
    return app->card_name;
}

static void
sm_app_init(SmApp *app)
{
    g_debug("sm_app_init.");
}

SmApp *
sm_app_new()
{
    g_debug("sm_app_new.");
    return g_object_new(SM_APP_TYPE,
                        "application-id", "org.alsa.scarlettmixer",
                        "flags", G_APPLICATION_FLAGS_NONE,
                        NULL);
}

GSettings*
sm_app_get_settings(SmApp *app)
{
    //TODO: Should the object reference be incremented?
    return app->settings;
}

GList*
sm_app_get_channels(SmApp *app)
{
    //TODO: Should the object reference be incremented?
    return app->channels;
}

GList*
sm_app_get_input_sources(SmApp *app)
{
    //TODO: Should the object reference be incremented?
    return app->input_sources;
}

GList*
sm_app_get_input_switches(SmApp *app)
{
    //TODO: Should the object reference be incremented?
    return app->input_switches;
}

SmSwitch*
sm_app_get_clock_source(SmApp *app)
{
    //TODO: Should the object reference be incremented?
    return app->clock_source;
}

SmSwitch*
sm_app_get_sync_status(SmApp *app)
{
    //TODO: Should the object reference be incremented?
    return app->sync_status;
}

gboolean
sm_app_write_config_file(SmApp *app, const char *filename)
{
    JsonBuilder *jb;
    JsonNode *jn;
    JsonGenerator *jg;
    GError *e = NULL;
    GList *item;

    jb = json_builder_new();
    jb = json_builder_begin_object(jb);

    jb = json_builder_set_member_name(jb, "card_name");
    jb = json_builder_add_string_value(jb, app->card_name);

    jb = json_builder_set_member_name(jb, "input_sources");
    jb = json_builder_begin_array(jb);
    for (item = g_list_first(app->input_sources);
            item;
            item = g_list_next(item))
    {
        jn = sm_source_to_json_node(SM_SOURCE(item->data));
        jb = json_builder_add_value(jb, jn);
    }
    jb = json_builder_end_array(jb);


    jb = json_builder_set_member_name(jb, "input_switches");
    jb = json_builder_begin_array(jb);
    for (item = g_list_first(app->input_switches);
            item;
            item = g_list_next(item))
    {
        jn = sm_switch_to_json_node(SM_SWITCH(item->data));
        jb = json_builder_add_value(jb, jn);
    }
    jb = json_builder_end_array(jb);

    jb = json_builder_set_member_name(jb, "channels");
    jb = json_builder_begin_array(jb);
    for (item = g_list_first(app->channels);
            item;
            item = g_list_next(item))
    {
        jn = sm_channel_to_json_node(SM_CHANNEL(item->data));
        jb = json_builder_add_value(jb, jn);
    }
    jb = json_builder_end_array(jb);

    jb = json_builder_end_object(jb);
    jn = json_builder_get_root(jb);
    jg = json_generator_new();
    json_generator_set_root(jg, jn);
    json_generator_set_pretty(jg, TRUE);
    if(!json_generator_to_file(jg, filename, &e))
    {
        g_print("Failed to write %s\n", filename);
        return FALSE;
    }
    return TRUE;
}

//TODO: return errno or use a GError and give information on the error.
gboolean
sm_app_read_config_file(SmApp *app, const char *filename)
{
    JsonParser *jp;
    JsonNode *jn;
    JsonObject *jo;
    JsonArray *ja;
    GError *e = NULL;
    guint al, i;
    GList *gl;
    const gchar *card_name;
    SmSource *src;
    SmSwitch *sw;
    SmChannel *ch;

    jp = json_parser_new();
    if(!json_parser_load_from_file(jp, filename, &e))
    {
        g_warning("Could not read file %s.", filename);
        return FALSE;
    }
    g_debug("Successfully read %s.", filename);
    jn = json_parser_get_root(jp);
    if (!JSON_NODE_HOLDS_OBJECT(jn))
    {
        g_warning("Invalid file format.");
        return FALSE;
    }
    jo = json_node_get_object(jn);
    if (!json_object_has_member(jo, "card_name"))
    {
        g_warning("Invalid file format: No card_name member found!");
        return FALSE;
    }
    if (!json_object_has_member(jo, "input_sources"))
    {
        g_warning("Invalid file format: No input_sources member found!");
        return FALSE;
    }
    if (!json_object_has_member(jo, "input_switches"))
    {
        g_warning("Invalid file format: No input_switches member found!");
        return FALSE;
    }
    if (!json_object_has_member(jo, "channels"))
    {
        g_warning("Invalid file format: No channels member found!");
        return FALSE;
    }

    card_name = json_object_get_string_member(jo, "card_name");
    if (g_strcmp0(card_name, app->card_name) != 0)
    {
        g_warning("Configuration not applicable for card %s.", app->card_name);
        return FALSE;
    }

    ja = json_object_get_array_member(jo, "input_sources");
    al = json_array_get_length(ja);
    for (i=0; i < al; i++)
    {
        for (gl = g_list_first(app->input_sources); gl; gl = g_list_next(gl))
        {
            src = SM_SOURCE(gl->data);
            if (sm_source_load_from_json_node(src, json_array_get_element(ja, i)))
            {
                break;
            }
        }
    }

    ja = json_object_get_array_member(jo, "input_switches");
    al = json_array_get_length(ja);
    for (i=0; i < al; i++)
    {
        for (gl = g_list_first(app->input_switches); gl; gl = g_list_next(gl))
        {
            sw = SM_SWITCH(gl->data);
            if (sm_switch_load_from_json_node(sw, json_array_get_element(ja, i)))
            {
                break;
            }
        }
    }

    ja = json_object_get_array_member(jo, "channels");
    al = json_array_get_length(ja);
    for (i=0; i < al; i++)
    {
        for (gl = g_list_first(app->channels); gl; gl = g_list_next(gl))
        {
            ch = SM_CHANNEL(gl->data);
            if (sm_channel_load_from_json_node(ch, json_array_get_element(ja, i)))
            {
                break;
            }
        }
    }
    g_object_unref(jp);
    return TRUE;
}

gchar*
sm_app_read_card_name_from_config_file(const gchar *filename)
{
    JsonParser *jp;
    JsonNode *jn;
    JsonObject *jo;
    GError *e = NULL;
    gchar *card_name;

    jp = json_parser_new();
    if(!json_parser_load_from_file(jp, filename, &e))
    {
        g_object_unref(jp);
        return g_strdup_printf("Could not read file %s.", filename);
    }
    g_debug("Successfully read %s.", filename);
    jn = json_parser_get_root(jp);
    if (!JSON_NODE_HOLDS_OBJECT(jn))
    {
        g_object_unref(jp);
        return g_strdup("Invalid file format.");
    }
    jo = json_node_get_object(jn);
    if (!json_object_has_member(jo, "card_name"))
    {
        g_object_unref(jp);
        return g_strdup("Invalid file format: No card_name member found!");
    }
    card_name = g_strdup(json_object_get_string_member(jo, "card_name"));
    g_object_unref(jp);
    return card_name;
}
