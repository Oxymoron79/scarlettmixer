/*
 ============================================================================
 Name        : scarlettmixer.c
 Author      : Martin Rösch
 Version     :
 Copyright   : Copyright (C) 2015
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm-channel.h"

#include "scarlettmixer.h"
#include "scarlettmixerapp.h"

struct playback *master;
/* List of struct playback */
GList *playback = NULL;
/* List of struct select */
GList *capture = NULL;
/* List of struct mix */
GList *mix = NULL;
/* List of struct select */
GList *mix_sources = NULL;

GList *channels = NULL;

static struct fader*
sm_fader_new(snd_mixer_elem_t *elem) {
    struct fader *sf = malloc(sizeof(struct fader));
    sf->elem = elem;
    sf->is_mono = snd_mixer_selem_is_playback_mono(elem);
    sf->has_switch = snd_mixer_selem_has_playback_switch(elem);
    sf->has_switch_joined = snd_mixer_selem_has_playback_switch_joined(elem);
    sf->has_volume_joined = snd_mixer_selem_has_playback_volume_joined(elem);
    sf->name = snd_mixer_selem_get_name(elem);
    snd_mixer_selem_get_playback_volume_range(elem, &(sf->min), &(sf->max));
    snd_mixer_selem_get_playback_dB_range(elem, &(sf->db_min), &(sf->db_max));
    return sf;
}

static struct select*
sm_select_new(snd_mixer_elem_t *elem) {
    struct select *ss = malloc(sizeof(struct select));
    int idx;
    char buf[16];
    ss->elem = elem;
    ss->name = snd_mixer_selem_get_name(elem);
    ss->num = snd_mixer_selem_get_enum_items(elem);
    ss->names = malloc(ss->num * sizeof(char*));
    for (idx = 0; idx < ss->num; idx++) {
        if(snd_mixer_selem_get_enum_item_name(elem, idx, 16, buf) == 0) {
            ss->names[idx] = g_strdup(buf);
        }
    }
    return ss;
}

static struct playback*
sm_playback_new(snd_mixer_elem_t *elem) {
    char buf[16];
    struct playback *sp;

    if (!g_str_has_prefix(snd_mixer_selem_get_name(elem), "Master")) {
        printf("sm_playback_new: elem name does not begin with Master!");
        return NULL;
    }
    sp  = malloc(sizeof(struct playback));
    sp->volume = NULL;
    sp->src_left = NULL;
    sp->src_right = NULL;
    sp->volume = sm_fader_new(elem);
    if(sscanf(sp->volume->name, "Master %u (%[^)])", &(sp->id), buf) > 0) {
        sp->name = g_strdup(buf);
    }
    else {
        sp->id = 0;
        sp->name = g_strdup(sp->volume->name);
    }
    return sp;
}

static int
sm_find_card(const char* prefix) {
    int number, err, ret;
    snd_ctl_t *ctl;
    snd_ctl_card_info_t *cinfo;
    char hw_buf[8];
    unsigned int num, i;

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
sm_mixer_elem_callback(snd_mixer_elem_t *elem, unsigned int mask) {
    g_message("sm_mixer_elem_callback: %s - Mask: 0x%X", snd_mixer_selem_get_name(elem), mask);
    if (mask == SND_CTL_EVENT_MASK_REMOVE) {

    }
    if (mask & SND_CTL_EVENT_MASK_VALUE) {
        if (snd_mixer_selem_has_playback_switch(elem)
                && snd_mixer_selem_has_playback_switch_joined(elem)
                && snd_mixer_selem_has_playback_volume(elem)
                && snd_mixer_selem_has_playback_volume_joined(elem)) {
            long db_val;
            snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_FRONT_LEFT, &db_val);
            g_message("sm_mixer_elem_callback: Master volume changed: %ddB.", db_val/100);
        }
    }
    if (mask & SND_CTL_EVENT_MASK_INFO) {

    }
    return 0;
}

static int
sm_mixer_callback(snd_mixer_t *mixer, unsigned int mask, snd_mixer_elem_t *elem) {
    g_message("sm_mixer_callback: %s - Mask: 0x%X", snd_mixer_selem_get_name(elem), mask);
    if (mask & SND_CTL_EVENT_MASK_ADD) {
        snd_mixer_elem_set_callback(elem, sm_mixer_elem_callback);
    }
    return 0;
}

static gboolean
sm_gioch_mixer_callback(GIOChannel *source,
            GIOCondition condition,
            gpointer data)
{
    snd_mixer_t *mixer = (snd_mixer_t*)data;
    snd_mixer_handle_events(mixer);
    return TRUE;
}

static void
sm_open_mixer(int card_number) {
    int err, idx;
    unsigned int id;
    char ch;
    char buf[16];
    snd_mixer_t *mixer;
    snd_mixer_elem_t *elem;
    struct snd_mixer_selem_regopt selem_regopt = {
            .ver = 1,
            .abstract = SND_MIXER_SABSTRACT_NONE,
            .device = g_strdup_printf("hw:%d", card_number)
    };
    snd_hctl_t *hctl;
    struct fader *sf;
    struct select *ss;
    struct playback *sp;
    struct mix *sm;
    GList *item;
    int npfds;
    struct pollfd *pfds;
    GIOChannel *gioch;


    err = snd_mixer_open(&mixer, 0);
    if (err < 0) {
        printf("Cannot open mixer.");
        return;
    }

    err = snd_mixer_selem_register(mixer, &selem_regopt, NULL);
    if (err < 0) {
        printf("Cannot register simple mixer.");
        return;
    }

    snd_mixer_set_callback(mixer, sm_mixer_callback);

    err = snd_mixer_load(mixer);
    if (err < 0) {
        printf("Cannot load mixer controls.");
        return;
    }

    err = snd_mixer_get_hctl(mixer, selem_regopt.device, &hctl);
    if (err < 0) {
        printf("Cannot get HCTL.\n");
        return;
    }/*
    else {
        snd_hctl_elem_t *hel;
        g_message("hctl: %d elements.", snd_hctl_get_count(hctl));
        for(hel=snd_hctl_first_elem(hctl); hel; hel=snd_hctl_elem_next(hel)) {
            g_message("list helem: numid=%u, name=%s",
                                snd_hctl_elem_get_numid(hel), snd_hctl_elem_get_name(hel));
        }
    }
*/
    npfds = snd_mixer_poll_descriptors_count(mixer);
    if (npfds > 0) {
        pfds = alloca(sizeof(*pfds) * npfds);
        npfds = snd_mixer_poll_descriptors(mixer, pfds, npfds);
        for (idx = 0; idx < npfds; idx++) {
            gioch = g_io_channel_unix_new(pfds[idx].fd);
            g_io_add_watch(gioch, G_IO_IN, sm_gioch_mixer_callback, mixer);
        }
    }
/*
    printf("%d mixer elements.\n", snd_mixer_get_count(mixer));
    printf("%d hctl elements.\n", snd_hctl_get_count(hctl));
*/
    for (elem = snd_mixer_first_elem(mixer); elem; elem = snd_mixer_elem_next(elem))
    {
        gboolean elem_added;
        elem_added = FALSE;
        for (item = g_list_first(channels); item; item = g_list_next(item))
        {
            elem_added = sm_channel_add_mixer_elem(SM_CHANNEL(item->data), elem);
            if (elem_added) {
                g_message("Added mixer element %s to channel %s.", snd_mixer_selem_get_name(elem), sm_channel_get_name(SM_CHANNEL(item->data)));
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
            }
            else
            {
                SmChannel *ch = sm_channel_new();
                if(sm_channel_add_mixer_elem(ch, elem))
                {
                    g_message("Created channel for mixer element %s.", snd_mixer_selem_get_name(elem));
                    g_message("    Type: %d.", sm_channel_get_channel_type(ch));
                    g_message("    Name: %s.", sm_channel_get_name(ch));
                    channels = g_list_append(channels, ch);
                }
                else
                {
                    g_message("Could not create channel for mixer element %s", snd_mixer_selem_get_name(elem));
                    g_object_unref(ch);
                }
            }
        }
    }
    /* Go through list again to add remaining Matrix Input Sources */
    for (elem = snd_mixer_first_elem(mixer); elem; elem = snd_mixer_elem_next(elem))
    {
        gboolean elem_added;
        if (!snd_mixer_selem_is_enumerated(elem)
                || !snd_mixer_selem_is_enum_playback(elem))
        {
            continue;
        }
        for (item = g_list_first(channels); item; item = g_list_next(item))
        {
            elem_added = sm_channel_add_mixer_elem(SM_CHANNEL(item->data), elem);
            if (elem_added) {
                g_message("Added mixer element %s to channel %s.", snd_mixer_selem_get_name(elem), sm_channel_get_name(SM_CHANNEL(item->data)));
            }
        }
    }
    for (elem = snd_mixer_first_elem(mixer); elem; elem = snd_mixer_elem_next(elem)) {
        // Master elements
        if (g_str_has_prefix(snd_mixer_selem_get_name(elem), "Master")) {
            // Master fader
            if (snd_mixer_selem_has_playback_switch(elem)
                    && snd_mixer_selem_has_playback_switch_joined(elem)
                    && snd_mixer_selem_has_playback_volume(elem)
                    && snd_mixer_selem_has_playback_volume_joined(elem)) {
                master = sm_playback_new(elem);
                continue;
            }
            // Master playback fader
            if (snd_mixer_selem_has_playback_switch(elem)
                    && !snd_mixer_selem_has_playback_switch_joined(elem)
                    && snd_mixer_selem_has_playback_volume(elem)
                    && !snd_mixer_selem_has_playback_volume_joined(elem)) {
                sp = sm_playback_new(elem);
                playback = g_list_append(playback, sp);
                continue;
            }
            // Master playback source select
            if (snd_mixer_selem_is_enumerated(elem)
                    && snd_mixer_selem_is_enum_playback(elem)
                    && !snd_mixer_selem_is_enum_capture(elem)) {
                ss = sm_select_new(elem);
                sscanf(ss->name, "Master %u%c (%[^)])", &id, &ch, buf);
                for (item = g_list_first(playback); item; item = g_list_next(item)) {
                    if (((struct playback*)item->data)->id == id) {
                        switch(ch) {
                            case 'L':
                                ((struct playback*)item->data)->src_left = ss;
                                break;
                            case 'R':
                                ((struct playback*)item->data)->src_right = ss;
                                break;
                        }
                    }
                }
                continue;
            }
        }
        // Input sources
        if (g_str_has_prefix(snd_mixer_selem_get_name(elem), "Input Source")) {
            ss = sm_select_new(elem);
            capture = g_list_append(capture, ss);
            continue;
        }
        // Matrix
        if (g_str_has_prefix(snd_mixer_selem_get_name(elem), "Matrix")) {
            sscanf(snd_mixer_selem_get_name(elem), "Matrix %u", &id);
            // Matrix input source select
            if (snd_mixer_selem_is_enumerated(elem)
                    && snd_mixer_selem_is_enum_playback(elem)
                    && !snd_mixer_selem_is_enum_capture(elem)) {
                ss = sm_select_new(elem);
                mix_sources = g_list_append(mix_sources, ss);
            }
            // Matrix input fader
            if (!snd_mixer_selem_has_playback_switch(elem)
                    && !snd_mixer_selem_has_playback_switch_joined(elem)
                    && snd_mixer_selem_has_playback_volume(elem)
                    && snd_mixer_selem_has_playback_volume_joined(elem)) {
                sf = sm_fader_new(elem);
                sscanf(sf->name, "Matrix %*u Mix %c", &ch);
                for (item = g_list_first(mix); item; item = g_list_next(item)) {
                    if (((struct mix*)item->data)->name == ch) {
                        sm = (struct mix*)item->data;
                        break;
                    }
                }
                if (item == NULL) {
                    sm = malloc(sizeof(struct mix));
                    sm->name = ch;
                    sm->volume = NULL;
                    mix = g_list_append(mix, sm);
                }
                sm->volume = g_list_append(sm->volume, sf);
            }
            continue;
        }
    }
}

void print_fader(struct fader* f) {
    int psw;

    snd_mixer_selem_get_playback_switch(f->elem, SND_MIXER_SCHN_FRONT_LEFT, &psw);
    printf("  Fader %s:\n", f->name);
    printf("    Mono: %d\n", f->is_mono);
    printf("    Switch: %d - Value: %d\n", f->has_switch, psw);
    printf("    Joined switch: %d\n", f->has_switch_joined);
    printf("    Joined volume: %d\n", f->has_volume_joined);
    printf("    Range: %u (%ddB) - %d (%ddB)\n",
            f->min, f->db_min / 100, f->max, f->db_max / 100);
}

void print_select(struct select* s) {
    int idx;
    printf("  Select %s\n", s->name);
    printf("    %d items: ", s->num);
    for (idx = 0; idx < s->num; idx++) {
        printf("%s, ", s->names[idx]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int card_number;
    GList *gl1, *gl2;
    struct playback *sp;
    struct select *ss;
    struct mix *sm;
    const char *prefix_scarlett = "Scarlett";
    const char *prefix_intel = "HDA Intel";
    const char *prefix = prefix_scarlett;
/*
    card_number = sm_find_card(prefix);
    if (card_number < 0) {
        printf("No %s card found!\n", prefix);
        return EXIT_FAILURE;
    }

    printf("Found %s card: hw:%d\n", prefix, card_number);
    sm_open_mixer(card_number);

    if (!master) {
        printf("Master:\n");
        print_fader(master->volume);
    }
    if (!playback) {
        for (gl1 = g_list_first(playback); gl1; gl1 = g_list_next(gl1)) {
            sp = (struct playback*)gl1->data;
            printf("Playback %d - %s:\n", sp->id, sp->name);
            print_fader(sp->volume);
            print_select(sp->src_left);
            print_select(sp->src_right);
        }
    }
    if (!capture) {
        for (gl1 = g_list_first(capture); gl1; gl1 = g_list_next(gl1)) {
            ss = (struct select*)gl1->data;
            printf("Capture:\n");
            print_select(ss);
        }
    }
    if (!mix) {
        for (gl1 = g_list_first(mix); gl1; gl1 = g_list_next(gl1)) {
            sm = (struct mix*)gl1->data;
            printf("Mix %c:\n", sm->name);
            for (gl2 = g_list_first(sm->volume); gl2; gl2 = g_list_next(gl2)) {
                print_fader((struct fader*)gl2->data);
            }
        }
    }
*/
    return g_application_run(G_APPLICATION(sm_app_new()), argc, argv);
}
