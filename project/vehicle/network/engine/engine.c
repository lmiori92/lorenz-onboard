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

#include "engine.h"

uint8_t vehicle_speed(uint8_t *data)
{
    return (uint8_t)(data[4] << 1U);    /* times 2 */
}

uint8_t vehicle_direction(uint8_t *data)
{
    return (uint8_t)(data[6] & 0x7U);
}

uint16_t engine_rpm(uint8_t *data)
{
    uint16_t retval = data[3];
    retval |= (uint16_t)(data[2] << 8);
    /* 0.250 factor, aka divide by 4, aka shift left 2 times */
    return (uint16_t)retval >> 2;
}

int8_t  engine_coolant(uint8_t *data)
{
    int8_t retval;
    /* -40°C offset */
    retval = data[2] - 40U;
    return retval;
}
