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

#include <stdbool.h>
#include <stdint.h>

/* Application includes */
#include "opelbid.h"
#include "deasplay/deasplay.h"
#include "vehicle/network/display/opel_bid.h"
#include "timer.h"

/* Network includes */
#include "isotp/isotp.h"
#include "iso-tp-hal.h"

static uint8_t pos = 0;
static bool refresh = false;
static uint8_t display_utf16[DEASPLAY_CHARS * 2] = { 0 };
/** Buffer for the iso-tp transfer: some overhead plus the display data */
static uint8_t isotp_send_buffer[16U + sizeof(display_utf16)] = { 0 };
static uint8_t isotp_send_buffer_len = 0;
static IsoTpSendHandle handle;

void opelbid_display_hal_state_callback(e_deasplay_state state)
{
    /* the BID display won't accept messages that are faster that a couple of milliseconds,
     * therefore wait */
//    ON_TIMER(5,SOFT_TIMER_0,TIMER_RESET(SOFT_TIMER_0),return );

    if ((refresh == true) && (isotp_send_buffer_len == 0) && (state == DEASPLAY_STATE_PERIODIC_END))
    {
        /* send information over CANbus */
        isotp_send_buffer_len = display_message(isotp_send_buffer,
                                                sizeof(isotp_send_buffer),
                                                display_utf16,
                                                sizeof(display_utf16)/2);

        /* initiate transfer */
        handle = isotp_send(&shims,
                            DISPLAY_PROTOCOL_CAN_ID,
                            isotp_send_buffer,
                            isotp_send_buffer_len,
                            NULL);

        if(handle.completed) {

            /* get out of the update routine even on error ... */
            isotp_send_buffer_len = 0;
            refresh = false;

            if(!handle.success) {
                /* something happened and it already failed - possibly we aren't able to send CAN messages */
            } else {
                /* If the message fit in a single frame, it's already been sent and you're done */
            }
        }
    }
    else if ((refresh == true) && (isotp_send_buffer_len != 0))
    {
        /* done */
        refresh = false;

        bool complete = isotp_continue_send(&shims,
                                            &handle,
                                            DISPLAY_PROTOCOL_CAN_ID,
                                            isotp_send_buffer,
                                            isotp_send_buffer_len);

        if(complete && handle.completed) {

            /* get out of the update routine even on error ... */
            isotp_send_buffer_len = 0;
            refresh = false;

            if(handle.success) {
                // All frames of the message have now been sent, following
                // whatever flow control feedback it got from the receiver
            } else {
                // the message was unable to be sent and we bailed - fatal
                // error!
            }
        }
    }
}

void opelbid_display_hal_init(void)
{
}

void opelbid_display_hal_power(e_deasplay_HAL_power state)
{
    switch(state)
    {
    case DEASPLAY_POWER_OFF:
        break;
    case DEASPLAY_POWER_ON:
        break;
    default:
        break;
    }
}

void opelbid_display_hal_set_cursor(uint8_t line, uint8_t chr)
{
    /* just keep the character position */
    pos = chr;
}

void opelbid_display_hal_write_char(uint8_t chr)
{
    if (pos < DEASPLAY_CHARS)
    {
        display_utf16[(pos * 2)] = 0;
        display_utf16[1 + (pos * 2)] = chr;
        /* at least one char has changed and therefore we need to refresh the display */
        refresh = true;
    }
}

void opelbid_display_hal_cursor_visibility(bool visible)
{
    /* Not available on the LC75710 controller */
}

void opelbid_hal_write_extended(uint8_t id)
{
    /* not implemented */
}

void opelbid_hal_set_extended(uint8_t id, uint8_t *data, uint8_t len)
{
    /* not implemented */
}
