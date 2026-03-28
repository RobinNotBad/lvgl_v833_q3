#ifndef PLAT_BAT_MANAGER_H
#define PLAT_BAT_MANAGER_H

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

/**********************
 *      TYPEDEFS
 **********************/

typedef enum 
{ 
    BATTERY_UNKNOWN = 0,
    BATTERY_DISCHARGING = 1, 
    BATTERY_CHARGING = 2, 
    BATTERY_FULL = 3
} battery_status_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

uint8_t battery_get_capacity(void);

double battery_get_voltage(void);

battery_status_t battery_get_status(void);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

