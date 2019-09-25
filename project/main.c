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
#include "can_hal.h"
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
#define CYCLE_TIME_US       (10000UL)
#define CYCLE_TIME_MS       (CYCLE_TIME_US / 1000UL)

/** iso-tp buffer */
uint8_t isotp_receive_buffer[OUR_MAX_ISO_TP_MESSAGE_SIZE];

t_input g_app_data_model = { 0, 0, KEY_NA, BTN_ERROR };
uint8_t can_intr_cnt = 0;
ISR(TIMER0_COMPA_vect)
{
#warning "Please use an additional interrupt for the same counter timer (5 times the 1st)"
    // 5kHz timer (200us)
    static uint8_t millisecond_temp_timer = 0U;
    /* increment the global variable */
    millisecond_temp_timer++;
    if (millisecond_temp_timer >= 5U) milliseconds_since_boot += 1UL;
    microseconds_since_boot += 200UL;
#warning "please account for errors e.g. other interrupt routine finished a bit later e.g. 250us instead of 200us"
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

typedef enum
{
    APP_MAIN_STATE_INIT,
    APP_MAIN_STATE_OPERATIONAL,
    APP_MAIN_STATE_DEINIT,
    APP_MAIN_STATE_OFF,
} e_app_main_state;


e_menu_input_event menu_evt = MENU_EVENT_NONE;

#include "isotp/send.h"
static IsoTpSendHandle handle;
static uint8_t isotp_tx_buffer[64];
void send_data(void)
{
    static bool started = false;
    if (started == false)
    {
        (void)memset(isotp_tx_buffer, 0xA5, 64);
        started = true;
        handle = isotp_send(&shims,
                            0x6C8U,
                            isotp_tx_buffer,
                            sizeof(isotp_tx_buffer),
                            NULL);
    }
    else
    {
       bool sent = isotp_continue_send(&shims, &handle,
               0x6C8U, isotp_tx_buffer,
               sizeof(isotp_tx_buffer));
       started = !sent;
    }

}

static void process_frame(can_t *frame)
{
    if (frame != NULL)
    {
        if (frame->id == 0x2C8UL)
        {
            /* display isotp FC frame */

        }
        else
        {
            /* Unknown frame */
        }
    }
}

void periodic_logic(void)
{
    can_t *pbuf;

    /* Process incoming frames */
    pbuf = can_buffer_get_dequeue_ptr(&mybuffer);
    if (pbuf != NULL)
    {
        process_frame(pbuf);
        can_buffer_dequeue(&mybuffer);
    }

    send_data();

    return;

e_key tmp = NUM_BUTTONS;
can_t rcv_msg;  /* the reception buffer */

if (/*can_check_message()*/false )// can_get_message(&rcv_msg))
{
    if (rcv_msg.id == 0x6C1)
    {
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
            uint8_t *ramPtr = (uint8_t*)RAMSTART;
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

//                    uint8_t status;
//                    do
//                    {
//                        /* terminate only when *every* buffer is available
//                         * otherwise messages can be sent out of order */
//                        status = mcp2515_read_status(SPI_READ_STATUS);
//                    } while ((status & 0x54) != 0);
//
//                    can_send_message(&asd);
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
    //ON_TIMER_EXPIRED(50, SOFT_TIMER_1, keypad_reset_input());
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

}

inline uint32_t get_us_counter(void)
{
    uint32_t temp;
    TIMSK0 &= ~(_BV(OCIE0A));   /* re-enable the timer interrupt */
    temp = microseconds_since_boot;
    TIMSK0  = _BV(OCIE0A);   /* re-enable the timer interrupt */
    return temp;
}






int main(void)
{
    bool ok = false;
    uint32_t ts_loop;

    /* disable all interrupts */
    cli();

    /* Start 5000Hz system timer with TC0 */
    OCR0A = F_CPU / 64 / 5000 - 1;
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS00) | _BV(CS01);
    TIMSK0 = _BV(OCIE0A);

    /* Chip select for the MMC/SD */
    DDRD |= (1 << PIND7);
    PORTD &= ~(1 << PIND7);

#warning "debug pin"
    // DEBUG PIN OUTPUT
    DEBUG_EXEC({
        DDRB |= (1 << PINB1);
        PORTB &=~(1 << PINB1);
    });

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
    can_hal_init();

    ts_loop = get_us_counter();
    uint32_t cpu_usage_us = 0;
    uint32_t prev_us = 0;
    while (1)
    {
        //PORTB = PORTB ^(1 << PINB1);
        PORTB |= (1 << PINB1);
        can_t asd;
        asd.id = 0x777;
        asd.length = 8;
        asd.flags.extended = 0;
        asd.flags.rtr = 0;
        asd.data[0] = (uint8_t)milliseconds_since_boot;
        asd.data[1] = (uint8_t)(milliseconds_since_boot >> 8);
        asd.data[2] = (uint8_t)(milliseconds_since_boot >> 16);
        asd.data[3] = (uint8_t)(milliseconds_since_boot >> 24);
        asd.data[4] = (uint8_t)cpu_usage_us;
        asd.data[5] = (uint8_t)(cpu_usage_us >> 8);
        asd.data[6] = (uint8_t)(cpu_usage_us >> 16);
        asd.data[7] = can_intr_cnt;//(uint8_t)(cpu_usage_us >> 24);
        //can_send_message_safe(&asd);

        /* Diagnose the FIFO full situation */
        // should be marked by the interrupt !!!
//        DEBUG_EXEC(
//                   ok = can_buffer_full(&mybuffer);
//                   if (ok == true) logger("RX FIFO FULL\n");
//                  );



        periodic_logic();

        /* output processing */
//        ON_TIMER_EXPIRED(1000, SOFT_TIMER_8, {
//                loggerf("RX %02X\n", can_intr_cnt);
//                loggerf("stack: %d", StackCount());
//                TIMER_RESET(SOFT_TIMER_0);
//        });

        //PORTB = PORTB ^(1 << PINB1);
        PORTB &= ~(1 << PINB1);
        /* Compute the load of the CPU at the current cycle */
        cpu_usage_us = (uint32_t)(get_us_counter() - prev_us);
        DEBUG_EXEC(
            if (cpu_usage_us > CYCLE_TIME_US)
            {
                logger("WARNING: CPU Time over 100%\n");
            });

        /* Burn the rest of the CPU time */
        while(get_us_counter() < ts_loop);
        /* Preload the next cycle */
        ts_loop += CYCLE_TIME_US;
        /* Start CPU load measurement */
        prev_us = get_us_counter();

    }

    /* shall never get there, reset otherwise */
}
