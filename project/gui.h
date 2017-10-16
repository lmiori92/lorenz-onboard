/*
 * gui.h
 *
 *  Created on: 11 ott 2017
 *      Author: lorenzo
 */

#ifndef GUI_H_
#define GUI_H_

/** Enumeration of menu pages */
typedef enum _e_psu_gui_menu
{
    MENU_MAIN,
    PSU_MENU_DEBUG
} e_psu_gui_menu;

void app_gui_init(void);

#endif /* GUI_H_ */
