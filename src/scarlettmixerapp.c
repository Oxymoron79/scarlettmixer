#include <gtk/gtk.h>
#include <alsa/asoundlib.h>

#include "scarlettmixerapp.h"
#include "scarlettmixerappwin.h"
#include "sm-channel.h"
#include "sm-source.h"

struct _ScarlettMixerAppClass
{
    GtkApplicationClass parent_class;
};

struct _ScarlettMixerApp
{
    GtkApplication parent;

    snd_hctl_t *hctl;
    snd_ctl_card_info_t *card_info;
    const char *card_name;
    snd_mixer_t *mixer;
    GList *channels;
    GList *input_sources;
};

G_DEFINE_TYPE(ScarlettMixerApp, sm_app, GTK_TYPE_APPLICATION);

static void
sm_app_activate (GApplication *app)
{
    ScarlettMixerAppWindow *win;
    ScarlettMixerApp *sm_app;

    g_debug("sm_app_activate.");
    sm_app = SCARLETTMIXER_APP(app);
    win = sm_app_window_new(SCARLETTMIXER_APP(app), sm_app->card_name);
    gtk_window_present(GTK_WINDOW(win));
}

static void
sm_app_open (GApplication  *app,
             GFile        **files,
             gint           n_files,
             const gchar   *hint)
{
    GList *windows;
    ScarlettMixerAppWindow *win;
    ScarlettMixerApp *sm_app;
    int i;

    g_debug("sm_app_open.");
    sm_app = SCARLETTMIXER_APP(app);
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows)
        win = SCARLETTMIXER_APP_WINDOW(windows->data);
    else
        win = sm_app_window_new(SCARLETTMIXER_APP(app), sm_app->card_name);

    for (i = 0; i < n_files; i++)
        sm_app_window_open(win, files[i]);

    gtk_window_present(GTK_WINDOW(win));
}

static void
sm_app_class_init (ScarlettMixerAppClass *class)
{
    g_debug("sm_app_class_init.");
    G_APPLICATION_CLASS(class)->activate = sm_app_activate;
    G_APPLICATION_CLASS(class)->open = sm_app_open;
}

static int
sm_app_find_card(const gchar* prefix)
{
    int number, err, ret;
    snd_ctl_t *ctl;
    snd_ctl_card_info_t *cinfo;
    gchar hw_buf[8];

    snd_ctl_card_info_malloc(&cinfo);
    number = -1;
    while (1) {
        err = snd_card_next(&number);
        if (err < 0) {
            // Cannot enumerate sound cards
            ret = err;
            break;
        }
        if (number == -1) {
            // No Scarlett sound card found
            ret = number;
            break;
        }
        sprintf(hw_buf, "hw:%d", number);
        err = snd_ctl_open(&ctl, hw_buf, 0);
        if (err < 0) {
            // Cannot open sound card
            continue;
        }
        err = snd_ctl_card_info(ctl, cinfo);
        if (err < 0) {
            // Cannot read info for sound card
            snd_ctl_close(ctl);
            continue;
        }
        snd_ctl_close(ctl);
        if (g_str_has_prefix(snd_ctl_card_info_get_name(cinfo), prefix)) {
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
    ScarlettMixerApp *app;
    SmChannel *ch;
    SmSource *src;
    GList *list;

    g_debug("sm_app_mixer_elem_callback: %s - Mask: 0x%X", snd_mixer_selem_get_name(elem), mask);
    app = SCARLETTMIXER_APP(g_application_get_default());
    if (!app)
    {
        g_debug("sm_app_mixer_elem_callback: app == NULL");
        return 0;
    }
    if (mask == SND_CTL_EVENT_MASK_REMOVE) {

    }
    if (mask & SND_CTL_EVENT_MASK_VALUE) {
        for (list = g_list_first(app->channels); list; list = g_list_next(list))
        {
            ch = SM_CHANNEL(list->data);
            if (sm_channel_has_mixer_elem(ch, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Channel %s has element %s",
                        sm_channel_get_name(ch), snd_mixer_selem_get_name(elem));
                sm_channel_mixer_elem_changed(ch, elem);
            }
        }
        for (list = g_list_first(app->input_sources); list; list = g_list_next(list))
        {
            src = SM_SOURCE(list->data);
            if (sm_source_has_mixer_elem(src, elem))
            {
                g_debug("sm_app_mixer_elem_callback: Source %s has element %s",
                        sm_source_get_name(src), snd_mixer_selem_get_name(elem));
                sm_source_mixer_elem_changed(src, elem);
                break;
            }
        }
    }
    if (mask & SND_CTL_EVENT_MASK_INFO) {

    }
    return 0;
}

static int
sm_app_mixer_callback(snd_mixer_t *mixer, unsigned int mask, snd_mixer_elem_t *elem) {
    g_debug("sm_app_mixer_callback: %s - Mask: 0x%X", snd_mixer_selem_get_name(elem), mask);
    if (mask & SND_CTL_EVENT_MASK_ADD) {
        snd_mixer_elem_set_callback(elem, sm_app_mixer_elem_callback);
    }
    return 0;
}

static gboolean
sm_app_gioch_mixer_callback(GIOChannel *source,
            GIOCondition condition,
            gpointer data)
{
    ScarlettMixerApp *app = SCARLETTMIXER_APP(data);
    snd_mixer_handle_events(app->mixer);
    return TRUE;
}

static void
sm_app_open_mixer(ScarlettMixerApp *app, int card_number)
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
    if (err < 0) {
        g_critical("Cannot open mixer.");
        return;
    }

    err = snd_mixer_selem_register(app->mixer, &selem_regopt, NULL);
    if (err < 0) {
        g_critical("Cannot register simple mixer.");
        return;
    }

    snd_mixer_set_callback(app->mixer, sm_app_mixer_callback);

    err = snd_mixer_load(app->mixer);
    if (err < 0) {
        g_critical("Cannot load mixer controls.");
        return;
    }

    err = snd_mixer_get_hctl(app->mixer, selem_regopt.device, &(app->hctl));
    if (err < 0) {
        g_critical("Cannot get HCTL.\n");
        return;
    }/*
    else {
        snd_hctl_elem_t *hel;
        g_debug("hctl: %d elements.", snd_hctl_get_count(hctl));
        for(hel=snd_hctl_first_elem(hctl); hel; hel=snd_hctl_elem_next(hel)) {
            g_debug("list helem: numid=%u, name=%s",
                                snd_hctl_elem_get_numid(hel), snd_hctl_elem_get_name(hel));
        }
    }
*/
    snd_ctl_card_info_malloc(&(app->card_info));
    err = snd_ctl_card_info(snd_hctl_ctl(app->hctl), app->card_info);
    if (err < 0) {
        g_critical("Cannot read information from sound card.");
        return;
    }
    app->card_name = snd_ctl_card_info_get_name(app->card_info);

    npfds = snd_mixer_poll_descriptors_count(app->mixer);
    if (npfds > 0) {
        pfds = alloca(sizeof(*pfds) * npfds);
        npfds = snd_mixer_poll_descriptors(app->mixer, pfds, npfds);
        for (idx = 0; idx < npfds; idx++) {
            gioch = g_io_channel_unix_new(pfds[idx].fd);
            g_io_add_watch(gioch, G_IO_IN, sm_app_gioch_mixer_callback, app);
        }
    }
/*
    g_debug("%d mixer elements.", snd_mixer_get_count(mixer));
    g_debug("%d hctl elements.", snd_hctl_get_count(hctl));
*/
    for (elem = snd_mixer_first_elem(app->mixer); elem; elem = snd_mixer_elem_next(elem))
    {
        gboolean elem_added;
        elem_added = FALSE;
        for (item = g_list_first(app->channels); item; item = g_list_next(item))
        {
            elem_added = sm_channel_add_mixer_elem(SM_CHANNEL(item->data), elem);
            if (elem_added) {
                g_debug("Added mixer element %s to channel %s.", snd_mixer_selem_get_name(elem), sm_channel_get_name(SM_CHANNEL(item->data)));
                break;
            }
        }
        if (!elem_added) {
            if (snd_mixer_selem_is_enumerated(elem)
                    && !snd_mixer_selem_is_enum_playback(elem)
                    && !snd_mixer_selem_is_enum_capture(elem))
            {
                /* Switch */
            }
            else if (snd_mixer_selem_is_enumerated(elem)
                    && !snd_mixer_selem_is_enum_playback(elem)
                    && snd_mixer_selem_is_enum_capture(elem))
            {
                /* Input source */
                SmSource *src = sm_source_new();
                if (sm_source_add_mixer_elem(src, elem))
                {
                    g_debug("Created input source for mixer element %s.", snd_mixer_selem_get_name(elem));
                    app->input_sources = g_list_append(app->input_sources, src);
                }
            }
            else
            {
                SmChannel *ch = sm_channel_new();
                if (sm_channel_add_mixer_elem(ch, elem))
                {
                    g_debug("Created channel for mixer element %s.", snd_mixer_selem_get_name(elem));
                    g_debug("    Type: %d.", sm_channel_get_channel_type(ch));
                    g_debug("    Name: %s.", sm_channel_get_name(ch));
                    g_debug("    Has left channel: %d.", sm_channel_has_volume(ch, SND_MIXER_SCHN_FRONT_LEFT));
                    g_debug("    Has right channel: %d.", sm_channel_has_volume(ch, SND_MIXER_SCHN_FRONT_RIGHT));
                    app->channels = g_list_append(app->channels, ch);
                }
                else
                {
                    g_warning("Could not create channel for mixer element %s", snd_mixer_selem_get_name(elem));
                    g_object_unref(ch);
                }
            }
        }
    }
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
                g_debug("Added mixer element %s to channel %s.", snd_mixer_selem_get_name(elem), sm_channel_get_name(SM_CHANNEL(item->data)));
            }
        }
    }
}

static void
sm_app_init (ScarlettMixerApp *app)
{
    const gchar *prefix_scarlett = "Scarlett";
    const gchar *prefix_intel = "HDA Intel";
    const gchar *prefix = prefix_scarlett;
    int card_number;

    g_debug("sm_app_init.");
    card_number = sm_app_find_card(prefix);
    if (card_number >= 0)
    {
        sm_app_open_mixer(app, card_number);
    }
}

ScarlettMixerApp *
sm_app_new()
{
    g_debug("sm_app_new.");
    return g_object_new(SCARLETTMIXER_APP_TYPE,
                        "application-id", "org.alsa.scarlettmixer",
                        "flags", G_APPLICATION_HANDLES_OPEN,
                        NULL);
}

GList *
sm_app_get_channels(ScarlettMixerApp *app)
{
    return app->channels;
}

GList *
sm_app_get_input_sources(ScarlettMixerApp *app)
{
    return app->input_sources;
}
