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

/* Driver includes */
#include "mcp2515_defs.h"
#include "mcp2515_private.h"

/* Network layer includes */
#include "can.h"

/* Protocol layer includes */
#include "isotp/isotp.h"
#include "iso-tp-hal.h"

/* Application includes */
#include "logger.h"

IsoTpShims shims =
{
        loggerf,
        send_can,
        set_timer
};

/**
 * iso-tp library CAN-Write routine
 * @param arbitration_id
 * @param data
 * @param size
 * @return
 */
bool send_can(const uint32_t arbitration_id, const uint8_t* data,
        const uint8_t size)
{
    can_t message;

    message.flags.rtr = 0;
    message.flags.extended = 0;
    message.id = arbitration_id;
    message.length = size;
    message.data[0] = data[0];
    message.data[1] = data[1];
    message.data[2] = data[2];
    message.data[3] = data[3];
    message.data[4] = data[4];
    message.data[5] = data[5];
    message.data[6] = data[6];
    message.data[7] = data[7];

    uint8_t status;
    do
    {
        /* terminate only when *every* buffer is available
         * otherwise messages can be sent out of order */
        status = mcp2515_read_status(SPI_READ_STATUS);
    } while ((status & 0x54) != 0);

    can_send_message(&message);

    _delay_ms(5);   /* the delay is vital for the BID display! */

    return true;
}

/**
 * Unused iso-tp stub, to be implemented
 * @param time_ms
 * @return
 */
bool set_timer(uint16_t time_ms, void (*callback))
{
    (void)time_ms;
    (void)callback;
    return false;
}
