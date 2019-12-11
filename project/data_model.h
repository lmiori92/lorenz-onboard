/*
 * data_model.h
 *
 *  Created on: 23 nov 2017
 *      Author: lorenzo
 */

#ifndef DATA_MODEL_H_
#define DATA_MODEL_H_

#include <stdbool.h>
#include <stdint.h>
#include "vehicle/network/radio/opel_cd30.h"
#include "vehicle/network/engine/chassis.h"

//to be moved
typedef enum
{
    DISPLAY_PAGE_UNKNOWN,
    DISPLAY_PAGE_RADIO,
    DISPLAY_PAGE_BOARD_COMPUTER,
    DISPLAY_PAGE_POPUP,
    DISPLAY_PAGE_WELCOME_DATE_TIME,
    DISPLAY_PAGE_ECS,
    DISPLAY_PAGE_SETTINGS
} e_display_page;

typedef struct
{
    int8_t              eng_coolant;            /**< Engine coolant temperature [°C]  */
    uint16_t            eng_speed;              /**< Engine rotation speed [RPM]      */
    uint8_t             vehicle_speed;          /**< Vehicle speed [Km/h]             */
    uint8_t             vehicle_direction;      /**< 1: stand; 2: forward; 4: reverse */
    e_key_state         key_state;
    e_button_name       btn_state;
    bool                btn_fresh;
    e_button_name       btn_wheel_state;
    bool                btn_wheel_fresh;
    uint8_t             btn_state_timeout;
    e_display_page      display_page;
    uint16_t            gearbox_calc_ratio;
    uint8_t             gearbox_calc_gear;
} t_input;

extern t_input g_app_data_model;

#endif /* DATA_MODEL_H_ */
