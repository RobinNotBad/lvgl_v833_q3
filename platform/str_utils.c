#include "str_utils.h"

const char * DAYS_OF_WEEK[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char * AUDIO_FILE_EXT[] = {".mp3", ".wav", ".ogg", ".m4a", ".aac", ".pcm", NULL};
const char * IMAGE_FILE_EXT[] = {".png", ".jpg", ".jpeg", ".bmp", ".gif", NULL};
const char * VIDEO_FILE_EXT[] = {".mp4", ".avi", ".mov", ".mkv", ".flv", ".rm", ".rmvb", NULL};
const char * MIDI_FILE_EXT[] = {".mid", ".midi", NULL};
const char * TEXT_FILE_EXT[] = {".txt", ".json", ".conf", ".log", ".cfg", NULL};

bool file_ext_match(const char * file_name, const char * file_ext[])
{
    for(int i = 0; file_ext[i] != NULL; i++) {
        if(str_end_with(file_name, file_ext[i], false)) return true;
    }
    return false;
}

bool str_begin_with(const char * str, const char * begin, bool case_sensitivity)
{
    if(str == NULL || begin == NULL) return false;

    uint16_t len1 = strlen(str);
    uint16_t len2 = strlen(begin);
    if((len1 < len2) || (len1 == 0 || len2 == 0)) return false;

    uint16_t i = 0;
    char * p   = begin;
    while(*p != '\0') {
        if(case_sensitivity) {
            if(*p != str[i]) return false;
        } else {
            if(to_upper_case(*p) != to_upper_case(str[i])) return false;
        }

        p++;
        i++;
    }

    return true;
}

bool str_end_with(const char * str, const char * end, bool case_sensitivity)
{
    if(str == NULL || end == NULL) return false;

    uint16_t len1 = strlen(str);
    uint16_t len2 = strlen(end);
    if((len1 < len2) || (len1 == 0 || len2 == 0)) return false;

    while(len2 >= 1) {
        if(case_sensitivity) {
            if(end[len2 - 1] != str[len1 - 1]) return false;
        } else {
            if(to_upper_case(end[len2 - 1]) != to_upper_case(str[len1 - 1])) return false;
        }

        len2--;
        len1--;
    }

    return true;
}

char to_upper_case(char c)
{
    if(is_upper_letter(c)) return c;
    if(is_lower_letter(c)) return c + 32;
    return c;
}

bool is_lower_letter(char c)
{
    return 65 <= c && c <= 90;
}

bool is_upper_letter(char c)
{
    return 97 <= c && c <= 122;
}