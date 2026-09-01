#include "battery_manager.h"

uint8_t battery_get_capacity(void)
{
    int capacity       = 0;
    FILE * fp_capacity = fopen("/sys/class/power_supply/axp803-battery/capacity", "r");
    if(fp_capacity != NULL) {
        fscanf(fp_capacity, "%d", &capacity);
        fclose(fp_capacity);
    }
    return capacity;
}

double battery_get_voltage(void)
{
    int voltage       = 0;
    FILE * fp_voltage = fopen("/sys/class/power_supply/axp803-battery/voltage_now", "r");
    if(fp_voltage != NULL) {
        fscanf(fp_voltage, "%d", &voltage);
        fclose(fp_voltage);
    }
    return voltage / 1000000.0;
}

battery_status_t battery_get_status(void)
{
    char status[24];
    FILE * fp_status = fopen("/sys/class/power_supply/axp803-battery/status", "r");
    if(fp_status != NULL) {
        fscanf(fp_status, "%s", status);
        fclose(fp_status);
    }

    if(strcmp(status, "Full") == 0)
        return BATTERY_FULL;
    else if(strcmp(status, "Charging") == 0)
        return BATTERY_CHARGING;
    else if(strcmp(status, "Not Charging") == 0)
        return BATTERY_NOT_CHARGING;
    else return BATTERY_DISCHARGING;
}