#ifndef AUDIO_ALSA_H
#define AUDIO_ALSA_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "alsa/asoundlib.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int audio_init(void);
int audio_write(void * data, size_t size);
int audio_release(void);
int audio_volume(int pct);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
