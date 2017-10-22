#ifndef __SM_STRIP_H
#define __SM_STRIP_H
/**
 * @file
 * @brief Header file for strip widget.
 *
 * The strip represents a mixer strip for the output channels
 * of the Scarlett card.
 */
#include <gtk/gtk.h>

#include "sm-channel.h"

/**
 * @brief Macro to get the type information of the strip object.
 */
#define SM_STRIP_TYPE (sm_strip_get_type ())
/**
 * @brief Macro to cast obj to strip object.
 */
#define SM_STRIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SM_STRIP_TYPE, SmStrip))

/**
 * @brief Type definition for strip object.
 */
typedef struct _SmStrip      SmStrip;
/**
 * @brief Type definition for strip class.
 */
typedef struct _SmStripClass SmStripClass;

/**
 * @brief Get the type of the strip object.
 * @return The type of the strip object.
 */
GType    sm_strip_get_type(void);

/**
 * @brief Create a new strip instance.
 * @return Pointer to the new strip instance.
 */
SmStrip *sm_strip_new(SmChannel *channel);

#endif /* __SM_STRIP_H */
