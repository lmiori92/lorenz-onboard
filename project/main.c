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
#include <debug.h>
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
#include "timer.h"
#include "data_model.h"

/* Debugging includes */
#include "debug.h"

/* Cycle Time Definitions */
#define CYCLE_TIME_US       (10000U)
#define CYCLE_TIME_MS       (CYCLE_TIME_US / 1000U)

/** iso-tp buffer */
uint8_t isotp_receive_buffer[OUR_MAX_ISO_TP_MESSAGE_SIZE];

t_input g_app_data_model = { 0, 0, KEY_NA, BTN_ERROR };

ISR(TIMER0_COMPA_vect)
{
    /* increment the global variable */
    milliseconds_since_boot += 1;
}

void StackPaint(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));

void StackPaint(void)
{
#if 0
    uint8_t *p = &_end;

    while(p <= &__stack)
    {
        *p = STACK_CANARY;
        p++;
    }
#else
    __asm volatile ("    ldi r30,lo8(_end)\n"
                    "    ldi r31,hi8(_end)\n"
                    "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */
                    "    ldi r25,hi8(__stack)\n"
                    "    rjmp .cmp\n"
                    ".loop:\n"
                    "    st Z+,r24\n"
                    ".cmp:\n"
                    "    cpi r30,lo8(__stack)\n"
                    "    cpc r31,r25\n"
                    "    brlo .loop\n"
                    "    breq .loop"::);
#endif
}

extern uint8_t _end;
extern uint8_t __stack;

uint16_t StackCount(void)
{
    const uint8_t *p = &_end;
    uint16_t       c = 0;

    while(*p == 0xC5 && p <= &__stack)
    {
        p++;
        c++;
    }

    return c;
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

    DEBUG_EXEC( { loggerf("len %d d.cmd %d d.len %d", message->size, cmd.command, cmd.len); } );

    if (cmd.command == 0x0050)
    {
        DEBUG_EXEC( { logger("icon"); });
    }
    else if ((cmd.command == 0x0040) || (cmd.command == 0x00C0))
    {
        DEBUG_EXEC( { logger("text"); });
    }

    loggerf("d.components %d", components);

    for (uint8_t i = 0; i < components; i++)
    {
        DEBUG_EXEC( { loggerf("c.cmd %d c.chr %d", cmd.components[i].command, cmd.components[i].num_utf16_chars); });

        uint16_t data_id = display_get_component_data(&cmd, i);

        DEBUG_EXEC( {
            for (uint16_t j = 0; j < cmd.components[i].num_utf16_chars * 2; j++)
            {
                if (isprint(message->payload[data_id + j]))
                    uart_putchar(message->payload[data_id + j], 0);
            }

            uart_putchar('\r',0);
            uart_putchar('\n',0);
            }
        );
    }

//    snprintf(buffer, 50, "data %02x%02x%02x%02x%02x%02x\r\n", message->payload[0], message->payload[0], message->payload[10], message->payload[11], message->payload[35], message->payload[40]);
//    uart_putstring(buffer);

}

/**
 * ISR(BADISR_vect)
 *
 * @brief This interrupt handler is executed whenever an ISR is fired
 * without a defined ISR routine.
 * It tries to write a string on the display and then blocks.
 * Especially useful when implementing interrupt routines..
 */
ISR(BADISR_vect)
{
    DEBUG_EXEC( { loggerf("no ISR."); });
    crashed();
}

//typedef struct
//{
//    uint8_t id;
//    uint8_t data[8];
//} t_can_frame;
//
//#define CAN_BUFFER_0X_SIZE  (0U)
//#define CAN_BUFFER_1X_SIZE  (0U)
//#define CAN_BUFFER_2X_SIZE  (0U)
//#define CAN_BUFFER_3X_SIZE  (0U)
//#define CAN_BUFFER_4X_SIZE  (0U)
//#define CAN_BUFFER_5X_SIZE  (0U)
//#define CAN_BUFFER_6X_SIZE  (0U)
//#define CAN_BUFFER_7X_SIZE  (0U)
//
//typedef struct
//{
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x0;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x1;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x2;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x3;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x4;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x5;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x6;
//#endif
//#if CAN_BUFFER_0X_SIZE != 0
//    t_can_frame buf_0x7;
//#endif
//} t_can_asd;

typedef enum
{
    APP_MAIN_STATE_INIT,
    APP_MAIN_STATE_OPERATIONAL,
    APP_MAIN_STATE_DEINIT,
    APP_MAIN_STATE_OFF,
} e_app_main_state;
#include "mcp2515_defs.h"
int main(void)
{
    e_menu_input_event menu_evt = MENU_EVENT_NONE;
//    e_app_main_state app_state = APP_MAIN_STATE_INIT;
    bool ok = false;
    can_t rcv_msg;  /* the reception buffer */

//    uint8_t *stackPaintPtr = RAMEND;
//    stackPaintPtr -= 1024;
//    do
//    {
//        *stackPaintPtr=0xAA;
//    } while((uint16_t)(stackPaintPtr++) <= (uint16_t)(RAMEND-16));

    /* disable all interrupts */
    cli();

    /* Start 1000Hz system timer with TC0 */
    OCR0A = F_CPU / 64 / 1000 - 1;
//    OCR0B = F_CPU / 1024 / 1000 - 1;
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS00) | _BV(CS01);
    TIMSK0 = _BV(OCIE0A);
//    TIMSK0 = _BV(OCIE0B);

    /* Chip select for the MMC/SD */
    DDRD |= (1 << PIND7);
    PORTD &= ~(1 << PIND7);

    /* enable all interrupts */
    sei();

    /* initializing components */
    uart_init();
    keypad_init();
    display_init();
    app_gui_init();

    logger("starting lorenz-onboard");
    logger("Opel Astra H MS-CAN");

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

    // benchmark
//    can_t asd;
//    asd.id = 0x700;
//    asd.length = 8;
//    asd.flags.extended = false;
//    asd.flags.rtr = false;

//    while (1)
//    {
//        asd.data[0] = (uint8_t)milliseconds_since_boot;
//        can_send_timed(&asd, 3, CAN_TIMER_0);
//    }

    while (1)
    {
        /* Cycle Time Expiration Delay */
        // TODO: interrupt based CAN buffers...
//        ok = true;
//        do
//        {
//            ON_TIMER_EXPIRED(CYCLE_TIME_MS, SOFT_TIMER_3, { ok = false; });
//        } while(ok);

        menu_evt = MENU_EVENT_NONE;

        // test code !
//    IsoTpReceiveHandle handlercv = isotp_receive(&shims, 0x6C1, message_received);
//    isotp_set_receive_buffer(&handlercv, isotp_receive_buffer);


//    if(handlercv.success) {
        // something happened and it already failed - possibly we aren't able to
        // send CAN messages
//        logger("failure from start");
//    } else {

    e_key tmp = NUM_BUTTONS;

    if (can_check_message() && can_get_message(&rcv_msg))
    {
        if (rcv_msg.id == 0x6C1)
        {
//            can_t corrupt;
//            corrupt.data[0] = 0x10;
//            corrupt.data[1] = 0x2E;
//            corrupt.data[2] = 0xC0;
//            corrupt.data[3] = 0x00;
//
//            corrupt.data[4] = 0x2B;
//            corrupt.data[5] = 0x03;
//            corrupt.data[6] = 0x01;
//            corrupt.data[7] = 0x01;
//
//            corrupt.flags.extended = 0;
//            corrupt.flags.rtr = 0;
//
//            corrupt.id = 0x6C1;
//            corrupt.length = 8;

                        // Continue to read from CAN, passing off each message to the handle
//                        IsoTpMessage message = isotp_continue_receive(&shims, &handlercv, rcv_msg.id, rcv_msg.data, rcv_msg.length);
//                        if(message.completed && handlercv.completed) {
//                            if(handlercv.success) {
                                // A message has been received successfully
//                                logger("message sent at");
//                            } else {
                                /* crash the CPU for the time being to debug most of the initial problems */
//                                crashed();
                                // Fatal error - we weren't able to receive a message and
                                // gave up trying. A message using flow control may have
                                // timed out.
//                            }
//                        }
        }
        else if (rcv_msg.id == 0x201)
        {
//            static e_button_name old_btn_state = BTN_ERROR;
            g_app_data_model.btn_fresh = true;
            g_app_data_model.btn_state = button_decode(rcv_msg.data, rcv_msg.length);
//            if (input.btn_state != old_btn_state)
//            {
//                loggerf("button press %d", input.btn_state);
//                old_btn_state = input.btn_state;
//            }
        }
        else if (rcv_msg.id == 0x450)
        {
            static e_key_state old_key_state = KEY_NA;
            g_app_data_model.key_state = ignition_decode(rcv_msg.data, rcv_msg.length);
            if (g_app_data_model.key_state != old_key_state)
            {
                loggerf("new key position %d", g_app_data_model.key_state);
                old_key_state = g_app_data_model.key_state;
            }
        }
        else if (rcv_msg.id == 0x4EC)
        {
            g_app_data_model.eng_coolant = engine_coolant(rcv_msg.data);
        }
        else if (rcv_msg.id == 0x4E8)
        {
            g_app_data_model.eng_speed = engine_rpm(rcv_msg.data);
        }
        else if (rcv_msg.id == 0x696)
        {
            /* display to radio message */
            if (rcv_msg.data[4] == 0x85)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_BOARD_COMPUTER;
            }
            else if (rcv_msg.data[4] == 0x81)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_RADIO;
            }
            else if (rcv_msg.data[4] == 0xA1)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_POPUP;
            }
            else if (rcv_msg.data[4] == 0x80)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_WELCOME_DATE_TIME;
            }
            else
            {
                g_app_data_model.display_page = DISPLAY_PAGE_UNKNOWN;
            }
//            static e_display_page oldpage = DISPLAY_PAGE_UNKNOWN;
//            if (oldpage != g_app_data_model.display_page)
//            {
//                oldpage = g_app_data_model.display_page;
//                loggerf("page: %d", g_app_data_model.display_page);
//            }
        }
        else if (rcv_msg.id == 0x666)
        {
            if ((rcv_msg.data[0] == 'H') && (rcv_msg.data[1] == 'A') &&
                (rcv_msg.data[2] == 'C') && (rcv_msg.data[3] == 'K'))
            {
                /* this is a special ram dump command:
                 * - block all interrupts (be careful not to use interrupt driven devices)
                 * - perform the dump of the memory
                 * - resume normal operation */
                uint8_t *ramPtr = RAMSTART;
                can_t asd;
                asd.id = 0x666;
                asd.length = 8;
                asd.flags.extended = 0;
                asd.flags.rtr = 0;
                cli();
                do
                {
                    //if (ramPtr < (&asd))
                    {
                        asd.data[0] = *(ramPtr++);
                        asd.data[1] = *(ramPtr++);
                        asd.data[2] = *(ramPtr++);
                        asd.data[3] = *(ramPtr++);
                        asd.data[4] = *(ramPtr++);
                        asd.data[5] = *(ramPtr++);
                        asd.data[6] = *(ramPtr++);
                        asd.data[7] = *(ramPtr++);

                        uint8_t status;
                        do
                        {
                            /* terminate only when *every* buffer is available
                             * otherwise messages can be sent out of order */
                            status = mcp2515_read_status(SPI_READ_STATUS);
                        } while ((status & 0x54) != 0);

                        can_send_message(&asd);
                    }
                } while((uint16_t)ramPtr <= (uint16_t)RAMEND);
                sei();
            }
        }

//      loggerf("engrpm  : %d", input.eng_speed);
//      loggerf("engcool : %d", input.eng_coolant);


    }

//    continue; //just testing the stack usage

    if (g_app_data_model.btn_fresh == true)
    {
        switch(g_app_data_model.btn_state)
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
        /* set the keypad input state to "true" i.e. pressed */
        keypad_set_input(tmp, true);
        g_app_data_model.btn_fresh = false;
        TIMER_RESET(SOFT_TIMER_1);
    }
    else
    {
        /* if button timed out: reset keypad state */
        ON_TIMER_EXPIRED(50, SOFT_TIMER_1, keypad_reset_input());
    }

    /* this logic shall always be run periodically */
    keypad_periodic(timeout(10, SOFT_TIMER_2));

    if ((g_app_data_model.display_page == DISPLAY_PAGE_BOARD_COMPUTER)
            ||
            (g_app_data_model.display_page == DISPLAY_PAGE_POPUP))
    {
        /* MENU */

        /* Input event to Menu event mapping */
        if (keypad_clicked(KEYPAD_BTN_LEFT) == KEY_CLICK) menu_evt = MENU_EVENT_LEFT;
        if (keypad_clicked(KEYPAD_BTN_RIGHT) == KEY_CLICK) menu_evt = MENU_EVENT_RIGHT;

        (void)menu_event(menu_evt);

        /* periodic function for the menu handler */
        menu_display();

        ON_TIMER_EXPIRED(4,SOFT_TIMER_0,display_periodic());
    }
    else
    {
        display_clean();
    }

    /* output processing */
    ON_TIMER_EXPIRED(1000, SOFT_TIMER_8, { loggerf("stack: %d", StackCount()); TIMER_RESET(SOFT_TIMER_0); });

    /* periodic display function */
//    ON_TIMER(5,SOFT_TIMER_0,TIMER_RESET(SOFT_TIMER_0),return );
//    display_periodic();


    /* Testing the timer */
//    ON_TIMER_EXPIRED(1000, SOFT_TIMER_10, { logger("timer expired"); TIMER_RESET(SOFT_TIMER_10); });
//    ON_TIMER_EXPIRED(1000, SOFT_TIMER_10, logger("a"); TIMER_RESET(SOFT_TIMER_10));
//    ON_TIMER_EXPIRED(1000, SOFT_TIMER_10, TIMER_RESET(SOFT_TIMER_10));

    //ON_TIMER_EXPIRED(5, SOFT_TIMER_10, can_send_message(&rcv_msg));

//    asd.id = 0x696;
//    asd.length = 8;
//    asd.flags.extended = 0;
//    asd.flags.rtr = 0;
//    asd.data[0] = 0x46;
//    asd.data[1] = 0x00;
//    asd.data[2] = 0x60;
//    asd.data[3] = 0x82;
//    asd.data[4] = 0x85;
//    asd.data[5] = 0x00;
//    asd.data[6] = 0x38;
//    asd.data[7] = 0x10;
//    asd.data[0] = (uint8_t)milliseconds_since_boot;
//    can_send_message(&asd);

    }
    /* shall never get there, reset otherwise */
}
