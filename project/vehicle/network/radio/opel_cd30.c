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

#include "opel_cd30.h"
#warning "we could also decode the encoder wheel of the ECS controller"
/* Specifications
 * 208#0816FF000000 -> ECS LEFT
 * 208#081601000000 -> ECS RIGHT
 * 208#011700000000 -> ECS PRESS
 * 208#001700000000 -> ECS RELEASE
 * */

e_button_name button_decode(uint8_t *can_data, uint8_t len)
{
    e_button_name button;

    switch(can_data[1])
    {
    case 0x01:
        button = BTN_BC;
        break;
    case 0x6F:
        button = BTN_OK;
        break;
    case 0x6D:
        button = BTN_LEFT;
        break;
    case 0x6C:
        button = BTN_RIGHT;
        break;
    case 0xE0:
        button = BTN_FM_OR_CD;
        break;
    case 0xFF:
        button = BTN_SETTINGS;
        break;
    default:
        button = BTN_ERROR;
    }

    if (can_data[0] == 0)
    {
        button = BTN_RELEASED;
    }

    return button;
}

e_button_name button_decode_wheel(uint8_t *can_data, uint8_t len)
{
    e_button_name button = BTN_ERROR;

    if (can_data[0] == 0)
    {
        button = BTN_RELEASED;
    }
    else
    {
        switch(can_data[1])
        {
            case 0x81:
                button = BTN_WHEEL_BTN_LEFT_UP;
                break;
            case 0x82:
                button = BTN_WHEEL_BTN_LEFT_DOWN;
                break;
            case 0x83:
                if (can_data[0] == 0x08)
                {
                    if (can_data[2] == 0x01)
                    {
                        button = BTN_WHEEL_DIAL_LEFT_UP;
                    }
                    else if (can_data[2] == 0xFF)
                    {
                        button = BTN_WHEEL_DIAL_LEFT_DOWN;
                    }
                }
                else if (can_data[0] == 0x01)
                {
                    button = BTN_WHEEL_DIAL_LEFT_CLICK;
                }
                break;
            case 0x91:
                button = BTN_WHEEL_BTN_RIGHT_UP;
                break;
            case 0x92:
                button = BTN_WHEEL_BTN_RIGHT_DOWN;
                break;
            case 0x93:
                if (can_data[0] == 0x08)
                {
                    if (can_data[2] == 0x01)
                    {
                        button = BTN_WHEEL_DIAL_RIGHT_UP;
                    }
                    else if (can_data[2] == 0xFF)
                    {
                        button = BTN_WHEEL_DIAL_RIGHT_DOWN;
                    }
                }
                break;
            default:
                break;
        }
    }

    return button;
}

