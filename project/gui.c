/*
 * gui.c
 *
 *  Created on: 11 ott 2017
 *      Author: lorenzo
 */

#include <avr/pgmspace.h>

#include "gui.h"
#include "megnu/menu.h"
#include "data_model.h"

static void app_gui_load_page(uint8_t page);

const PROGMEM t_menu_item menu_main_page[] =
{
    { "DEBUG",    (void*)(uint8_t)PSU_MENU_DEBUG,        MENU_TYPE_GOTO },
    { "HALLO",    (void*)(uint8_t)PSU_MENU_DEBUG,        MENU_TYPE_GOTO },
    { "CAVALLO",  (void*)(uint8_t)PSU_MENU_DEBUG,        MENU_TYPE_GOTO },
    { "RPM",      (void*)&g_app_data_model.eng_speed,        MENU_TYPE_NUMERIC_16 },
    { "TEMP",     (void*)&g_app_data_model.eng_coolant,        MENU_TYPE_NUMERIC_8 },
};

const PROGMEM t_menu_item menu_debug_page[] =
{
    { "DATE", (void*)0,        MENU_TYPE_NUMERIC_16  },
};

static void app_set_page(const t_menu_item *page, uint8_t count)
{
    uint8_t i = 0;

    for (i = 0; i < count; i++)
    {
        menu_item_add(page + i);
    }
}

static void app_set_page_progmem(const PROGMEM t_menu_item menu[], uint8_t count)
{
    uint8_t i = 0;
    t_menu_item item;

    for (i = 0; i < count; i++)
    {
        memcpy_P(&item, menu + i, sizeof(item));
        menu_item_add(&item);
    }
}

void app_gui_init(void)
{
    menu_init(1U);  /* lines */
    menu_set_page(MENU_MAIN);
    menu_set_diff(1);
    app_gui_load_page(menu_get_page());
}

static void app_gui_add_back(e_psu_gui_menu page)
{
    t_menu_item back = {"BACK", (void*)(uint8_t)page, MENU_TYPE_GOTO };
    menu_item_add(&back);
}

static void app_gui_load_page(uint8_t page)
{
    menu_clear();

    switch(page)
    {
    case MENU_MAIN:
        app_set_page_progmem(menu_main_page, sizeof(menu_main_page)/sizeof(menu_main_page[0]));
        break;
    case PSU_MENU_DEBUG:
        app_set_page_progmem(menu_debug_page, sizeof(menu_debug_page)/sizeof(menu_debug_page[0]));
        app_gui_add_back(MENU_MAIN);
        break;
    default:
        break;
    }

    /* set the new page value for the application layer */
    menu_set_page(page);
}

void menu_event_callback(e_menu_output_event event, uint8_t index, uint8_t page, uint8_t info)
{
    switch(event)
    {
    case MENU_EVENT_OUTPUT_GOTO:
        /* select page */
        app_gui_load_page(info);
        break;
    case MENU_EVENT_CLICK_LONG:
        break;
    case MENU_EVENT_OUTPUT_EXTRA_EDIT:
        break;
    case MENU_EVENT_CLICK:
    case MENU_EVENT_OUTPUT_DESELECT:
        break;
    default:
        break;
    }
}
