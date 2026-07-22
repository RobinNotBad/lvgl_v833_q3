#ifndef PROJ_PAGE_UPGRADE_H
#define PROJ_PAGE_UPGRADE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../lvgl/lvgl.h"
#include "page_manager.h"
#include "page_txt.h"
#include "../cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <pthread.h>

#define UPGRADE_DEBUG(fmt, ...) printf("[Upgrade] " fmt "\n", ##__VA_ARGS__)
#define CA_BUNDLE_PATH "./tools/ca-bundle.crt"
#define LOCAL_VERSION_PATH  "./setting/version"
#define TMP_DIR             "/tmp/dendro"
#define TMP_UPGRADE_INFO    "/tmp/dendro/upgrade_info.txt"
#define TMP_ERROR_LOG       "/tmp/dendro/error.txt"

BasePage * page_upgrade_create(const char * json_url);

#ifdef __cplusplus
}
#endif
#endif