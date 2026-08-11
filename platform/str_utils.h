#ifndef PLAT_STR_UTILS_H
#define PLAT_STR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
extern const char * DAYS_OF_WEEK[];

extern const char * AUDIO_FILE_EXT[];
extern const char * IMAGE_FILE_EXT[];
extern const char * VIDEO_FILE_EXT[];
extern const char * MIDI_FILE_EXT[];
extern const char * TEXT_FILE_EXT[];

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
bool file_ext_match(const char * file_name, const char * file_ext[]);
bool str_begin_with(const char * str, const char * begin, bool case_sensitivity);
bool str_end_with(const char * str, const char * begin, bool case_sensitivity);
char to_upper_case(char c);
bool is_lower_letter(char c);
bool is_upper_letter(char c);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
