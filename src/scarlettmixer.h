#ifndef __SCARLETTMIXER_H
#define __SCARLETTMIXER_H

#include <glib.h>
#include <alsa/asoundlib.h>

struct fader {
    snd_mixer_elem_t *elem;
    int is_mono;
    int has_switch;
    int has_switch_joined;
    int has_volume_joined;
    const char *name;
    long min;
    long max;
    long db_min;
    long db_max;
};

struct select {
    snd_mixer_elem_t *elem;
    const char *name;
    unsigned short num;
    char **names;
};

struct playback {
    struct fader *volume;
    struct select *src_left;
    struct select *src_right;
    unsigned int id;
    char *name;
};

struct mix {
    GList *volume;
    char name;
};

extern struct playback *master;
extern GList *playback;
extern GList *capture;
extern GList *mix;
extern GList *mix_sources;

#endif /* __SCARLETTMIXER_H */
