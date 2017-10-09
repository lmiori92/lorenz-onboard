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

#ifndef VEHICLE_NETWORK_DISPLAY_OPEL_BID_H_
#define VEHICLE_NETWORK_DISPLAY_OPEL_BID_H_

#include <stdint.h>

typedef struct
{
    uint16_t command;
    uint8_t  num_utf16_chars;
    uint8_t  *data;
} t_display_component;

#define DISPLAY_MAX_COMPONENTS      (5)

typedef struct
{
    uint16_t command;
    uint8_t  len;
    t_display_component components[DISPLAY_MAX_COMPONENTS];
} t_display_command;

uint16_t display_get_component_data(t_display_command *cmd, uint8_t component);
uint8_t display_get_components_number(t_display_command *cmd);
void display_decode(const uint8_t *data, uint16_t len, t_display_command *cmd);
void display_encode(uint8_t *data, uint16_t len, const t_display_command *cmd);

uint16_t display_message(uint8_t *buffer, uint16_t len, uint8_t *message, uint8_t message_len);

#endif /* VEHICLE_NETWORK_DISPLAY_OPEL_BID_H_ */
