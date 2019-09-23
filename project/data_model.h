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
    DISPLAY_PAGE_WELCOME_DATE_TIME
} e_display_page;

typedef struct
{
    int8_t              eng_coolant;
    uint16_t            eng_speed;
    e_key_state         key_state;
    e_button_name       btn_state;
    bool                btn_fresh;
    uint8_t             btn_state_timeout;
    e_display_page      display_page;
} t_input;

extern t_input g_app_data_model;

#endif /* DATA_MODEL_H_ */
