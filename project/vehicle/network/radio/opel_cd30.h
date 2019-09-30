/*
    MIT License

    Copyright (c) 2017 Lorenzo Miori

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Lorenzo Miori (C) 2017 [ 3M4|L: memoryS60<at>gmail.com ]

*/

#ifndef VEHICLE_NETWORK_RADIO_OPEL_CD30_H_
#define VEHICLE_NETWORK_RADIO_OPEL_CD30_H_

#include <stdint.h>

#define OPEL_CD30_BUTTON_CAN_ID             (0x201UL)
#define OPEL_CD30_BUTTON_WHEEL_CAN_ID       (0x206UL)

typedef enum
{
    BTN_ERROR,
    BTN_BC,
    BTN_OK,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_FM_OR_CD,
    BTN_SETTINGS,
    BTN_WHEEL_DIAL_LEFT_UP,
    BTN_WHEEL_DIAL_LEFT_DOWN,
    BTN_WHEEL_DIAL_LEFT_CLICK,
    BTN_WHEEL_DIAL_RIGHT_UP,
    BTN_WHEEL_DIAL_RIGHT_DOWN,
    BTN_WHEEL_BTN_LEFT_UP,
    BTN_WHEEL_BTN_LEFT_DOWN,
    BTN_WHEEL_BTN_RIGHT_UP,
    BTN_WHEEL_BTN_RIGHT_DOWN,
    BTN_RELEASED
} e_button_name;

e_button_name button_decode(uint8_t *can_data, uint8_t len);
e_button_name button_decode_wheel(uint8_t *can_data, uint8_t len);

#endif /* VEHICLE_NETWORK_RADIO_OPEL_CD30_H_ */
