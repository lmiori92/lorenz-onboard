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

#include "logger.h"
#include "uart.h"
#include "timer.h"

static char buffer[64];
static char logger_buffer[64];

void logger(char* message)
{
    snprintf(logger_buffer, sizeof(logger_buffer), "[%ld] %s\r\n", milliseconds_since_boot, message);
    uart_putstring(logger_buffer);
}

// optional, provide to receive debugging log messages
void loggerf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);
    logger(buffer);
}

void crashed(void)
{
    logger("crashed. halting the CPU.");
    while (1);
}
