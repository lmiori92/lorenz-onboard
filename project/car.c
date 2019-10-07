#include "car.h"
#include "data_model.h"

/**
 *
 * The following formulas are implemented with integer logic.
 *
 * K1 = KM per INCH / (1-hour-in-minutes * PI)
 *    = 39370.1 / 60 * PI = 208.8
 * Speed (KM/h) = (RPM * tire diameter) / (Axle Ratio * Gear Ratio * K1)
 *
 * So, to retrieve the gear ratio from speed and RPM:
 *
 * Gear Ratio = (RPM * tire diameter) / (Axle Ratio * Speed * K1)
 *
 * As the MCUs are slow computing floating point arithmetics, integer arithmetic
 * is used and to do so the formula is calculated in a split fashion:
 * (considering: RPM Max = 8000 and Vehicle Speed = 200
 *
 * V1 = (RPM * 10000) / ((AxleRatio*100) * Vehicle Speed)
 * (GearRatio*100) = V1 * 27 / 209
 *
 *
 * @param vehicle_speed
 * @param engine_speed
 * @return
 */
static uint16_t gearbox_ratio_calc(uint8_t vehicle_speed, uint16_t engine_speed)
{
    uint16_t retval = UINT16_MAX;
    uint32_t V1     = 0U;

    if (vehicle_speed > 0U)
    {
        V1 = (uint32_t)((uint32_t)engine_speed * 10000UL);
        V1 /= (uint32_t)((uint32_t)vehicle_speed * (uint32_t)Z19DTH_M32_GEARBOX_RATIO_AXLE);
        V1 *= 27UL;
        V1 /= 209UL;
    }

    if (V1 < UINT16_MAX)
    {
        retval = (uint16_t)V1;
    }

    return retval;
}

static uint8_t gearbox_ratio_to_gear_number(uint16_t gear_ratio, bool reverse)
{
    uint8_t retval = 0xFF;

    if (reverse == true)
    {
        if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_REV)
        {
            retval = 0xFEU;
        }
    }
    else if (gear_ratio > (Z19DTH_M32_GEARBOX_RATIO_1ST + GEAR_RATIO_ERROR_RANGE))
    {
        /* Error out of range*/
    }
    else if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_1ST)
    {
        retval = 1U;
    }
    else if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_2ND)
    {
        retval = 2U;
    }
    else if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_3RD)
    {
        retval = 3U;
    }
    else if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_4TH)
    {
        retval = 4U;
    }
    else if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_5TH)
    {
        retval = 5U;
    }
    else if (gear_ratio > Z19DTH_M32_THR_GEARBOX_RATIO_6TH)
    {
        retval = 6U;
    }

    return retval;
}

void car_logic(void)
{
    /** Gearbox Ratio Calculation **/
    g_app_data_model.gearbox_calc_ratio = gearbox_ratio_calc(g_app_data_model.vehicle_speed, g_app_data_model.eng_speed);
    g_app_data_model.gearbox_calc_gear  = gearbox_ratio_to_gear_number(g_app_data_model.gearbox_calc_ratio, (g_app_data_model.vehicle_direction == 0x04U));
}
#include <stdio.h>
#include "logger.h"
void car_print_debug(void)
{
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "SPD: %u; RPM: %u GR: %u G: %ud",
             g_app_data_model.vehicle_speed,
             g_app_data_model.eng_speed,
             g_app_data_model.gearbox_calc_ratio,
             g_app_data_model.gearbox_calc_gear
             );
    logger(buffer);
}
