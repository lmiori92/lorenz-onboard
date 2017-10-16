/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Lorenzo Miori (C) 2017 [ 3M4|L: memoryS60<at>gmail.com ]

*/

/**
 * @file
 * @author Lorenzo Miori
 * @date March 2017
 * @brief Display HAL to interface the APIs to the Opel Astra H BID display
 */

#ifndef DRIVER_OPELBID_OPELBID_H_
#define DRIVER_OPELBID_OPELBID_H_

#include "deasplay/deasplay_hal.h"

/* Enable library features */
#define HAS_CHARACTER_INTERFACE

void opelbid_display_hal_state_callback(e_deasplay_state state);
void opelbid_display_hal_init(void);
void opelbid_display_hal_power(e_deasplay_HAL_power state);
void opelbid_display_hal_set_cursor(uint8_t line, uint8_t chr);
void opelbid_display_hal_write_char(uint8_t chr);
void opelbid_display_hal_cursor_visibility(bool visible);
void opelbid_hal_write_extended(uint8_t id);
void opelbid_hal_set_extended(uint8_t id, uint8_t *data, uint8_t len);

/* bindings */
#define deasplay_hal_state_callback     opelbid_display_hal_state_callback
#define deasplay_hal_init               opelbid_display_hal_init
#define deasplay_hal_power              opelbid_display_hal_power
#define deasplay_hal_set_cursor         opelbid_display_hal_set_cursor
#define deasplay_hal_write_char         opelbid_display_hal_write_char
#define deasplay_hal_cursor_visibility  opelbid_display_hal_cursor_visibility
#define deasplay_hal_write_extended     opelbid_write_extended
#define deasplay_hal_set_extended       opelbid_set_extended

#endif /* DRIVER_OPELBID_OPELBID_H_ */
