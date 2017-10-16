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

    Lorenzo Miori (C) 2016 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

/**
 * @file configuration.h
 * @author Lorenzo Miori
 * @date Aug 2016
 * @brief Display configuration
 */

#ifndef SRC_DEASPLAY_H_
#define SRC_DEASPLAY_CONFIGURATION_H_

/** Enable this to have the full fledged printf facility
 * NOTE: this might take up to 2kB of code memory! */
/* #define DISPLAY_HAS_PRINTF */

#define DEASPLAY_OPEL_BID

#define DEASPLAY_LINES 1
#define DEASPLAY_CHARS 16

#if defined(DEASPLAY_HD44780)

#include "deasplay-HD44780/hd44780_hal.h"

#elif defined(DEASPLAY_LC75710)

#include "deasplay-LC75710/lc75710_hal.h"

#elif defined(DEASPLAY_UART)

#error "Not implemented yet!"

#elif defined(DEASPLAY_NCURSES)

#include "deasplay-ncurses/ncurses_hal.h"

#elif defined(DEASPLAY_SSD1036)

#include "deasplay-SSD1036/ssd1036.h"

#elif defined(DEASPLAY_PCD8544)

#include "deasplay-PCD8544/pcd8544.h"

#elif defined(DEASPLAY_OPEL_BID)

#include "hal/OPELBID/opelbid.h"

#else
#error "Please define a display driver."
#endif

#endif /* SRC_DEASPLAY_H_ */
