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

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

/* Network layer includes */
#include "can.h"
#include "uart.h"

/* Protocol layer includes */
#include "isotp/isotp.h"
#include "iso-tp-hal.h"

/* Application layer includes */
#include "vehicle/network/display/opel_bid.h"
#include "vehicle/network/radio/opel_cd30.h"
#include "vehicle/network/engine/chassis.h"
#include "vehicle/network/engine/engine.h"

/* Application includes */
#include "gui.h"
#include "deasplay.h"
#include "megnu/menu.h"
#include "logger.h"
#include "keypad/keypad.h"

uint32_t milliseconds_since_boot = 0;

/** iso-tp buffer */
uint8_t isotp_receive_buffer[OUR_MAX_ISO_TP_MESSAGE_SIZE];
//uint8_t isotp_send_buffer[OUR_MAX_ISO_TP_MESSAGE_SIZE];

typedef struct
{
    int8_t eng_coolant;
    uint16_t eng_speed;
    e_key_state key_state;
    e_button_name btn_state;
} t_input;

t_input input = { 0, 0, KEY_NA, BTN_ERROR };



ISR(TIMER0_COMPA_vect)
{
    milliseconds_since_boot += 10;
}

// -----------------------------------------------------------------------------
/** Set filters and masks.
 *
 * The filters are divided in two groups:
 *
 * Group 0: Filter 0 and 1 with corresponding mask 0.
 * Group 1: Filter 2, 3, 4 and 5 with corresponding mask 1.
 *
 * If a group mask is set to 0, the group will receive all messages.
 *
 * If you want to receive ONLY 11 bit identifiers, set your filters
 * and masks as follows:
 *
 *  uint8_t can_filter[] PROGMEM = {
 *      // Group 0
 *      MCP2515_FILTER(0),              // Filter 0
 *      MCP2515_FILTER(0),              // Filter 1
 *
 *      // Group 1
 *      MCP2515_FILTER(0),              // Filter 2
 *      MCP2515_FILTER(0),              // Filter 3
 *      MCP2515_FILTER(0),              // Filter 4
 *      MCP2515_FILTER(0),              // Filter 5
 *
 *      MCP2515_FILTER(0),              // Mask 0 (for group 0)
 *      MCP2515_FILTER(0),              // Mask 1 (for group 1)
 *  };
 *
 *
 * If you want to receive ONLY 29 bit identifiers, set your filters
 * and masks as follows:
 *
 * \code
 *  uint8_t can_filter[] PROGMEM = {
 *      // Group 0
 *      MCP2515_FILTER_EXTENDED(0),     // Filter 0
 *      MCP2515_FILTER_EXTENDED(0),     // Filter 1
 *
 *      // Group 1
 *      MCP2515_FILTER_EXTENDED(0),     // Filter 2
 *      MCP2515_FILTER_EXTENDED(0),     // Filter 3
 *      MCP2515_FILTER_EXTENDED(0),     // Filter 4
 *      MCP2515_FILTER_EXTENDED(0),     // Filter 5
 *
 *      MCP2515_FILTER_EXTENDED(0),     // Mask 0 (for group 0)
 *      MCP2515_FILTER_EXTENDED(0),     // Mask 1 (for group 1)
 *  };
 * \endcode
 *
 * If you want to receive both 11 and 29 bit identifiers, set your filters
 * and masks as follows:
 */
const uint8_t can_filter[] PROGMEM =
{
    // Group 0
    MCP2515_FILTER(0),              // Filter 0
    MCP2515_FILTER(0),              // Filter 1

    // Group 1
    MCP2515_FILTER_EXTENDED(0),     // Filter 2
    MCP2515_FILTER_EXTENDED(0),     // Filter 3
    MCP2515_FILTER_EXTENDED(0),     // Filter 4
    MCP2515_FILTER_EXTENDED(0),     // Filter 5

    MCP2515_FILTER(0),              // Mask 0 (for group 0)
    MCP2515_FILTER_EXTENDED(0),     // Mask 1 (for group 1)
};
// You can receive 11 bit identifiers with either group 0 or 1.

// Optional: This is your callback for when a complete ISO-TP message is
// received at the arbitration ID you specify. The completed message is
// also returned by isotp_continue_receive, which can sometimes be more
// useful since you have more context.
void message_received(const IsoTpMessage* message)
{
    t_display_command cmd;
    uint8_t components = 0;

    /* decode the message and get compoenents count */
    display_decode(message->payload, message->size, &cmd);
    components = display_get_components_number(&cmd);

    loggerf("len %d d.cmd %d d.len %d", message->size, cmd.command, cmd.len);

    if (cmd.command == 0x0050)
    {
        logger("icon");
    }
    else if ((cmd.command == 0x0040) || (cmd.command == 0x00C0))
    {
        logger("text");
    }

    loggerf("d.components %d", components);

    for (uint8_t i = 0; i < components; i++)
    {
        loggerf("c.cmd %d c.chr %d", cmd.components[i].command, cmd.components[i].num_utf16_chars);

        uint16_t data_id = display_get_component_data(&cmd, i);

        for (uint16_t j = 0; j < cmd.components[i].num_utf16_chars * 2; j++)
        {
            if (isprint(message->payload[data_id + j]))
                uart_putchar(message->payload[data_id + j], 0);
        }

        uart_putchar('\r',0);
        uart_putchar('\n',0);
    }

//    snprintf(buffer, 50, "data %02x%02x%02x%02x%02x%02x\r\n", message->payload[0], message->payload[0], message->payload[10], message->payload[11], message->payload[35], message->payload[40]);
//    uart_putstring(buffer);

}

int main(void)
{
    cli();

    /* Start 100Hz system timer with TC0 */
    OCR0A = F_CPU / 1024 / 100 - 1;
    TCCR0A = _BV(WGM01);
    TCCR0B = 0b101;
    TIMSK0 = _BV(OCIE0A);

    /* Chip select for the MMC/SD */
    DDRD |= (1 << PIND7);
    PORTD &= ~(1 << PIND7);

    sei();

    /* initializing components */
    uart_init();
    keypad_init();
    display_init();
    app_gui_init();

    logger("starting lorenz-onboard");
    logger("Opel Astra H MS-CAN");

    bool ok = false;
    logger("initializing canbus");
    do
    {
        /* it can happen that we have a failure at boot.
         * Retry indefinitely: the application won't run without a CANbus network! */
        ok = can_init(BITRATE_95_238_KBPS);
    }
    while(ok == false);
    logger("initialized canbus");

    /* load filters and masks */
    can_static_filter(can_filter);

    while (1)
    {

        // test code !
    IsoTpReceiveHandle handlercv = isotp_receive(&shims, 0x6C1, message_received);
    isotp_set_receive_buffer(&handlercv, isotp_receive_buffer);
    can_t rcv_msg;

    if(handlercv.success) {
        // something happened and it already failed - possibly we aren't able to
        // send CAN messages
        logger("failure from start");
    } else {
        //while(true) {

            if (can_check_message())
            {
                if (can_get_message(&rcv_msg))
                {
                    if (rcv_msg.id == 0x6C1)
                    {
                        // Continue to read from CAN, passing off each message to the handle
                        IsoTpMessage message = isotp_continue_receive(&shims, &handlercv, rcv_msg.id, rcv_msg.data, rcv_msg.length);
                        if(message.completed && handlercv.completed) {
                            if(handlercv.success) {
                                // A message has been received successfully
                                logger("message sent at");
                                break;
                            } else {
                                /* crash the CPU for the time being to debug most of the initial problems */
                                crashed();
                                // Fatal error - we weren't able to receive a message and
                                // gave up trying. A message using flow control may have
                                // timed out.
                            }
                        }
                    }
                    else if (rcv_msg.id == 0x201)
                    {
                        static e_button_name old_btn_state = BTN_ERROR;
                        e_key tmp;

                        input.btn_state = button_decode(rcv_msg.data, rcv_msg.length);
                        if (input.btn_state != old_btn_state)
                        {
                            loggerf("button press %d", input.btn_state);
                            old_btn_state = input.btn_state;
                        }
                        switch(input.btn_state)
                        {
                        case BTN_BC:
                            tmp = KEYPAD_BTN_BC;
                            break;
                        case BTN_FM_OR_CD:
                            tmp = KEYPAD_BTN_FM_OR_CD;
                            break;
                        case BTN_LEFT:
                            tmp = KEYPAD_BTN_LEFT;
                            break;
                        case BTN_OK:
                            tmp = KEYPAD_BTN_OK;
                            break;
                        case BTN_RIGHT:
                            tmp = KEYPAD_BTN_RIGHT;
                            break;
                        case BTN_SETTINGS:
                            tmp = KEYPAD_BTN_SETTINGS;
                            break;
                        case BTN_ERROR:
                        default:
                            tmp = NUM_BUTTONS;
                            break;
                        }
                        keypad_clicked(tmp);
                    }
                    else if (rcv_msg.id == 0x450)
                    {
                        static e_key_state old_key_state = KEY_NA;
                        input.key_state = ignition_decode(rcv_msg.data, rcv_msg.length);
                        if (input.key_state != old_key_state)
                        {
                            loggerf("new key position %d", input.key_state);
                            old_key_state = input.key_state;
                        }
                    }
                    else if (rcv_msg.id == 0x4EC)
                    {
                        input.eng_coolant = engine_coolant(rcv_msg.data);
                    }
                    else if (rcv_msg.id == 0x4E8)
                    {
                        input.eng_speed = engine_rpm(rcv_msg.data);
                    }

                    static uint32_t old_time = 0;
                    if ((uint32_t)(milliseconds_since_boot ) > (uint32_t)(old_time + 250 ))
                    {
                        old_time = milliseconds_since_boot;
//                        loggerf("engrpm  : %d", input.eng_speed);
//                        loggerf("engcool : %d", input.eng_coolant);
//                        display_engine();
                    }

                }
            }
//        }

        /* this logic shall always be run periodically */
        static uint32_t old_millis = 0;
        if (milliseconds_since_boot > (uint32_t)(old_millis + 50))
        {
            old_millis = milliseconds_since_boot;
            keypad_periodic(true);
        }
        else
        {
            keypad_periodic(false);
        }

        e_key_event evt = keypad_clicked(KEYPAD_BTN_SETTINGS);
        e_menu_input_event menu_evt = MENU_EVENT_NONE;

        /* MENU */

        /* Input event to Menu event mapping */
        /* The Left and Right events are handled in the interrupt */
        if (evt == KEY_CLICK) menu_evt = MENU_EVENT_CLICK;
        else if (evt == KEY_HOLD) menu_evt = MENU_EVENT_CLICK_LONG;

        if (keypad_clicked(KEYPAD_BTN_LEFT) == KEY_CLICK) menu_evt = MENU_EVENT_LEFT;
        if (keypad_clicked(KEYPAD_BTN_RIGHT) == KEY_CLICK) menu_evt = MENU_EVENT_RIGHT;

        if (input.btn_state == BTN_LEFT) menu_evt = MENU_EVENT_LEFT;
        if (input.btn_state == BTN_RIGHT) menu_evt = MENU_EVENT_RIGHT;
        input.btn_state = BTN_ERROR;

        (void)menu_event(menu_evt);

        /* periodic function for the menu handler */
        menu_display();

        /* periodic display function */
        display_periodic();
    }
    }

    /* shall never get there, reset otherwise */
}
