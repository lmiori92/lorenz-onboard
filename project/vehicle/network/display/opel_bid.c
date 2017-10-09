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

#include "opel_bid.h"

#include <string.h>

uint16_t display_get_component_data(t_display_command *cmd, uint8_t component)
{
    uint8_t i;
    uint16_t retval = 0;
    retval += 6;

    for (i = 0; i < component; i++)
    {
        retval += 2;
        retval += cmd->components[i].num_utf16_chars * 2;

        if (cmd->components[i].num_utf16_chars == 0)
        {
            retval = 0;
            break;
        }
    }
    return retval;
}

uint8_t display_get_components_number(t_display_command *cmd)
{
    uint8_t i;
    for (i = 0; i < DISPLAY_MAX_COMPONENTS; i++)
    {
        if (cmd->components[i].num_utf16_chars == 0)
        {
            break;
        }
    }
    return i;
}

void display_decode(const uint8_t *data, uint16_t len, t_display_command *cmd)
{
    uint16_t temp_index = 0;

    memset(cmd, 0, sizeof(t_display_command));

    if (len > 3)
    {
        cmd->command = data[0];
        temp_index++;
        cmd->command |= (uint16_t)((uint16_t)data[1] << 8U);
        temp_index++;
        cmd->len = data[2];
        temp_index++;

        temp_index++; // unknown ???

        /* search for components */
        for (uint8_t i = 0; i < DISPLAY_MAX_COMPONENTS; i++)
        {
            if ((len - 1) >= temp_index)
            {

            /* build the command - address */
            cmd->components[i].command = data[temp_index];
            temp_index++;
//            cmd->components[i].command = (uint16_t)((uint16_t)data[temp_index] << 8);
//            temp_index++;
            /* get the number of utf-16 characters */
            cmd->components[i].num_utf16_chars = data[temp_index];
            temp_index++;
            if (cmd->components[i].num_utf16_chars > cmd->len)
            {
                /* error -> bail out */
                break;
            }
            else
            {
                temp_index += 2 * cmd->components[i].num_utf16_chars;
            }
            }
            else
            {
                /* error -> bail out */
                break;
            }
        }
    }
}

void display_encode(uint8_t *data, uint16_t len, const t_display_command *cmd)
{

    if (len > 3)
    {
        *data++ = (uint8_t)cmd->command;
        *data++ = (uint8_t)((uint16_t)cmd->command >> 8U);
        *data++ = cmd->len;

        *data++ = 0x03; // unknown ???

        /* search for components */
        for (uint8_t i = 0; i < DISPLAY_MAX_COMPONENTS; i++)
        {
            if ((cmd->components[i].num_utf16_chars != 0) && (cmd->components[i].data != NULL))
            {
                /* valid command to be encoded */

                /* build the command - address */
                *data++ = cmd->components[i].command;

                /* get the number of utf-16 characters */
                *data++ = cmd->components[i].num_utf16_chars;

                /* append data if defined */
                for (uint8_t j = 0; j < 2 * cmd->components[i].num_utf16_chars; j++)
                {
                    *data++ = cmd->components[i].data[j];
                }
            }
        }
    }
}

uint16_t display_message(uint8_t *buffer, uint16_t len, uint8_t *message, uint8_t message_len)
{
    t_display_command cmd = { 0 };

    /* set the command for the display */
    cmd.command = 0x0040;
    /* set the number of characters to be sent */
    cmd.components[0].num_utf16_chars = message_len;
    cmd.components[0].data = message;
    cmd.components[0].command = 0x10;
    cmd.len = 6;
    cmd.len += cmd.components[0].num_utf16_chars;

    display_encode(buffer, len, &cmd);

    return 2 + cmd.len;
}
