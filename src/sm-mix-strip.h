#ifndef __SM_MIX_STRIP_H
#define __SM_MIX_STRIP_H
/**
 * @file
 * @brief Header file for the mix strip widget.
 *
 * The mix strip represents a mixer strip for one of the Matrix Mixer channels
 * of the Scarlett card.
 */
#include <gtk/gtk.h>

#include "sm-channel.h"

/**
 * @brief Macro to get the type information of the mix strip object.
 */
#define SM_MIX_STRIP_TYPE (sm_mix_strip_get_type())
/**
 * @brief Macro to cast obj to mix strip object.
 */
#define SM_MIX_STRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SM_MIX_STRIP_TYPE, SmMixStrip))

/**
 * @brief Type definition for mix strip object.
 */
typedef struct _SmMixStrip      SmMixStrip;
/**
 * @brief Type definition for mix strip class.
 */
typedef struct _SmMixStripClass SmMixStripClass;

/**
 * @brief Get the type of the mix strip object.
 * @return The type of the mix strip object.
 */
GType       sm_mix_strip_get_type(void);
/**
 * @brief Create a new mix strip instance.
 * @param channel The SmChannel to add to the mix strip.
 * Only Matrix mixer channels are valid.
 * @return Pointer to new mix strip instance or NULL, if the channel parameter
 * is not a Matrix mixer channel.
 */
SmMixStrip *sm_mix_strip_new(SmChannel *channel);
/**
 * @brief Add an additional SmChannel to the mix strip.
 * @param strip The mix strip object.
 * @param channel The SmChannel to add to the mix strip.
 * Only Matrix mixer channels are valid.
 * @return TRUE if the channel was successfully added, FALSE otherwise.
 */
gboolean    sm_mix_strip_add_channel(SmMixStrip *strip, SmChannel *channel);

/**
 * @brief Get the Mix IDs the mix strip is part of.
 * @param strip The mix strip object.
 * @return The Mix ID characters.
 */
gchar      *sm_mix_strip_get_mix_ids(SmMixStrip *strip);

#endif /* __SM_MIX_STRIP_H */
