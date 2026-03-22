#ifndef AUDIO_ALSA_H
#define AUDIO_ALSA_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <alsa/asoundlib.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void audio_enable(void);
void audio_disable(void);
int audio_init(void);
void audio_close(void);
int audio_volume_set(int percent);
int audio_volume_get(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
