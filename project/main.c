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

/* Application layer includes */
#include "vehicle/network/display/opel_bid.h"
#include "vehicle/network/radio/opel_cd30.h"
#include "vehicle/network/engine/chassis.h"
#include "vehicle/network/engine/engine.h"

/* Application includes */
#include "logger.h"
#include "keypad/keypad.h"
#include "timer.h"
#include "data_model.h"

/* Debugging includes */
#include "debug.h"

#warning "TODO to remove"
#include <util/atomic.h>

/* Cycle Time Definitions */
#define CYCLE_TIME_US       (10000UL)
#define CYCLE_TIME_MS       (CYCLE_TIME_US / 1000UL)

t_input g_app_data_model = { 0, 0, KEY_NA, BTN_ERROR };
uint8_t can_intr_cnt = 0;
ISR(TIMER0_COMPA_vect)
{
#warning "Please use an additional interrupt for the same counter timer (5 times the 1st)"
    // 5kHz timer (200us)
    static uint8_t millisecond_temp_timer = 0U;
    /* increment the global variable */
    millisecond_temp_timer++;
    if (millisecond_temp_timer >= 5U) { milliseconds_since_boot += 1UL; millisecond_temp_timer = 0; }
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
//void message_received(const IsoTpMessage* message)
//{
//    t_display_command cmd;
//    uint8_t components = 0;
//
//    /* decode the message and get compoenents count */
//    display_decode(message->payload, message->size, &cmd);
//    components = display_get_components_number(&cmd);
//
//    DEBUG_EXEC( { loggerf("len %d d.cmd %d d.len %d", message->size, cmd.command, cmd.len); } );
//
//    if (cmd.command == 0x0050)
//    {
//        DEBUG_EXEC( { logger("icon"); });
//    }
//    else if ((cmd.command == 0x0040) || (cmd.command == 0x00C0))
//    {
//        DEBUG_EXEC( { logger("text"); });
//    }
//
//    loggerf("d.components %d", components);
//
//    for (uint8_t i = 0; i < components; i++)
//    {
//        DEBUG_EXEC( { loggerf("c.cmd %d c.chr %d", cmd.components[i].command, cmd.components[i].num_utf16_chars); });
//
//        uint16_t data_id = display_get_component_data(&cmd, i);
//
//        DEBUG_EXEC( {
//            for (uint16_t j = 0; j < cmd.components[i].num_utf16_chars * 2; j++)
//            {
//                if (isprint(message->payload[data_id + j]))
//                    uart_putchar(message->payload[data_id + j], 0);
//            }
//
//            uart_putchar('\r',0);
//            uart_putchar('\n',0);
//            }
//        );
//    }
//}

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
    APP_MAIN_STATE_OFF,
    APP_MAIN_STATE_OPERATIONAL,
    APP_MAIN_STATE_MAX
} e_app_main_state;

typedef enum
{
    TEXTBOX_TYPE_LEFT,
    TEXTBOX_TYPE_CENTER,
    TEXTBOX_TYPE_RIGHT,
} e_textbox_type;

typedef enum
{
    TEXTBOX_TX_STATE_IDLE,
    TEXTBOX_TX_STATE_WAIT_FC,
    TEXTBOX_TX_STATE_CONSECUTIVE_FRAME_1,
    TEXTBOX_TX_STATE_CONSECUTIVE_FRAME_2,
} e_textbox_tx_state;

typedef struct
{
    e_textbox_type     current_type;
    e_textbox_tx_state popup_tx_state;
    char               popup_right_text[5];
    char               popup_center_text[5];
    char               popup_left_text[5];
} s_textbox_tx_state;

s_textbox_tx_state textbox_tx_state = { TEXTBOX_TYPE_LEFT, TEXTBOX_TX_STATE_IDLE, { '\0','\0','\0','\0','\0' } };

// State of the application to place somewhere
e_app_main_state app_state = APP_MAIN_STATE_OFF;

static void process_textbox(can_t *frame, e_textbox_tx_state *state)
{
    switch(*state)
    {
        case TEXTBOX_TX_STATE_WAIT_FC:
            /* Check the given can frame if it is a consecutive frame */
            if (frame->id == 0x2C8)
            {
                if (frame->data[0] == 0x30)
                {
                    /* Continue */
                    *state = TEXTBOX_TX_STATE_CONSECUTIVE_FRAME_1;
                }
                else
                {
                    /* Abort */
                    *state = TEXTBOX_TX_STATE_IDLE;
                }
            }
            break;
    }
}

/**
 * This function implements the transmission of the textbox for the GID display.
 * It implements a very simple and reduced logic for the isotp transport layer.
 * DLC=0 don't send; >0: send
 * @param type
 * @param text
 * @param frame
 * @param state
 */
static void show_textbox(e_textbox_type type, char* text, can_t *frame, e_textbox_tx_state *state)
{
    uint8_t text_len;
    bool done = false;

    text_len = strlen(text);

    if (text_len == 0) return;  // no text !

    switch(*state)
    {
        case TEXTBOX_TX_STATE_IDLE:
            /* IDLE and request to transmit: prepare the First Frame (FF)   */
            frame->data[0] = 0x10U;             /* Multi-frame              */
            frame->data[1] = 0x06U + (text_len*2);  /* size of the data         */
            frame->data[2] = 0x40U;             /* display general command  */
            frame->data[3] = 0x00U;             /* display general command  */
            frame->data[4] = 0x03U + (text_len*2);  /* size of the command data */
            frame->data[5] = 0x0AU;             /* AC Section               */
            if (type == TEXTBOX_TYPE_RIGHT)        frame->data[6] = 0xA5U;  /* popup type selection */
            else if (type == TEXTBOX_TYPE_CENTER)  frame->data[6] = 0xA4U;  /* popup type selection */
            else if (type == TEXTBOX_TYPE_LEFT)    frame->data[6] = 0xA3U;  /* popup type selection */
            else                                   frame->data[6] = 0xFFU;  /* unknown popup type selection */
            frame->data[7] = text_len;          /* length of the popup text data */
            frame->length = 8;

            /* Go on with the next state */
            *state = TEXTBOX_TX_STATE_WAIT_FC;
            TIMER_RESET(SOFT_TIMER_2);

            break;
        case TEXTBOX_TX_STATE_WAIT_FC:
            ON_TIMER_EXPIRED(1000, SOFT_TIMER_2, { *state = TEXTBOX_TX_STATE_IDLE; });
            break;
        case TEXTBOX_TX_STATE_CONSECUTIVE_FRAME_1:
            frame->data[0] = 0x21U;                 /* Consecutive Frame 1 */
            if (text_len > 0)
            {
                frame->data[1] = 0x00U;             /* text data */
                frame->data[2] = text[0];           /* text data */
            }
            if (text_len > 1)
            {
                frame->data[3] = 0x00U;             /* text data */
                frame->data[4] = text[1];           /* text data */
            }
            if (text_len > 2)
            {
                frame->data[5] = 0x00U;             /* text data */
                frame->data[6] = text[2];           /* text data */
            }
            if (text_len > 3)
            {
                frame->data[7] = 0x00U;             /* text data */
                *state = TEXTBOX_TX_STATE_CONSECUTIVE_FRAME_2;
            }
            else
            {
                done = true;
            }
            frame->length = 8;

            break;
        case TEXTBOX_TX_STATE_CONSECUTIVE_FRAME_2:
            frame->data[0] = 0x22U;                 /* Consecutive Frame 2 */
            if (text_len > 3)
            {
                frame->data[1] = text[3];             /* text data */
                frame->data[2] = 0xCCU;
                frame->data[3] = 0xCCU;
                frame->data[4] = 0xCCU;
                frame->data[5] = 0xCCU;
                frame->data[6] = 0xCCU;
                frame->data[7] = 0xCCU;
            }
            frame->length = 8;
            done = true;
            break;
    }
    if (done == true)
    {
        text[0] = '\0';     // do not send the text any longer
        *state = TEXTBOX_TX_STATE_IDLE;
    }
}

static void process_frame(can_t *frame)
{
    if (frame != NULL)
    {
        /* display isotp FC frame */
        process_textbox(frame, &textbox_tx_state.popup_tx_state);

        if (frame->id == 0x6C8UL)
        {
            /* Someone else in the network is transmitting to the display.
             * Abort our current transaction and inhibit transactions for
             * 50ms after the last received frame.
             */
            TIMER_RESET(SOFT_TIMER_4);
            DEBUG_EXEC({loggerf("abort isotp tx");});
        }
        else if (frame->id == OPEL_CD30_BUTTON_CAN_ID)
        {
            g_app_data_model.btn_fresh = true;
            g_app_data_model.btn_state = button_decode(frame->data, frame->length);
        }
        else if (frame->id == OPEL_CD30_BUTTON_WHEEL_CAN_ID)
        {
            g_app_data_model.btn_wheel_fresh = true;
            g_app_data_model.btn_wheel_state = button_decode_wheel(frame->data, frame->length);
        }
        else if (frame->id == 0x450)
        {
            static e_key_state old_key_state = KEY_NA;
            g_app_data_model.key_state = ignition_decode(frame->data, frame->length);
            if (g_app_data_model.key_state != old_key_state)
            {
                loggerf("new key position %d", g_app_data_model.key_state);
                old_key_state = g_app_data_model.key_state;
            }
        }
        else if (frame->id == 0x4EC)
        {
            g_app_data_model.eng_coolant = engine_coolant(frame->data);
        }
        else if (frame->id == 0x4E8)
        {
            g_app_data_model.vehicle_speed = vehicle_speed(frame->data);
            g_app_data_model.eng_speed = engine_rpm(frame->data);
            g_app_data_model.vehicle_direction = vehicle_direction(frame->data);
        }
        else if (frame->id == 0x696)
        {
            /* display to radio message */
            if (frame->data[4] == 0x85)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_BOARD_COMPUTER;
            }
            else if (frame->data[4] == 0x81)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_RADIO;
            }
            else if (frame->data[4] == 0xA1)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_POPUP;
            }
            else if (frame->data[4] == 0x80)
            {
                g_app_data_model.display_page = DISPLAY_PAGE_WELCOME_DATE_TIME;
            }
            else
            {
                g_app_data_model.display_page = DISPLAY_PAGE_UNKNOWN;
            }
            static e_display_page oldpage = DISPLAY_PAGE_UNKNOWN;
            if (oldpage != g_app_data_model.display_page)
            {
                oldpage = g_app_data_model.display_page;
                loggerf("page: %d", g_app_data_model.display_page);
            }
        }
        else
        {
            /* Unknown frame */
        }
    }
}

void keypad_set_input_from_model(e_button_name button)
{
    e_key key;

    switch(button)
    {
    case BTN_BC:
        key = KEYPAD_BTN_BC;
        break;
    case BTN_FM_OR_CD:
        key = KEYPAD_BTN_FM_OR_CD;
        break;
    case BTN_LEFT:
        key = KEYPAD_BTN_LEFT;
        break;
    case BTN_OK:
        key = KEYPAD_BTN_OK;
        break;
    case BTN_RIGHT:
        key = KEYPAD_BTN_RIGHT;
        break;
    case BTN_SETTINGS:
        key = KEYPAD_BTN_SETTINGS;
        break;
    case BTN_WHEEL_DIAL_LEFT_UP:
        key = KEYPAD_BTN_WHEEL_DIAL_UP;
        break;
    case BTN_WHEEL_DIAL_LEFT_DOWN:
        key = KEYPAD_BTN_WHEEL_DIAL_DOWN;
        break;
    case BTN_ERROR:
    default:
        key = NUM_BUTTONS;
        break;
    }

    for (uint8_t i = 0; i < (uint8_t)NUM_BUTTONS; i++)
    {
        keypad_set_input((e_key)i, false);
    }

    /* set the keypad input state to "true" i.e. pressed */
    if (key != NUM_BUTTONS)
    {
        keypad_set_input(key, true);
    }
}

void keypad_reset_input(void)
{
    for (uint8_t i = 0; i < (uint8_t)NUM_BUTTONS; i++)
    {
        keypad_set_input((e_key)i, false);
    }
}

void periodic_logic(void)
{
    can_t *pbuf;

    /********* INPUT PROCESSING ***********/

    /* Process incoming frames */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        pbuf = can_buffer_get_dequeue_ptr(&mybuffer);
    }

    if (pbuf != NULL)
    {
        process_frame(pbuf);
        can_buffer_dequeue(&mybuffer);
    }

    /********* LOGIC PROCESSING ************/

    if ((g_app_data_model.key_state == KEY_LOCKED) || (g_app_data_model.key_state == KEY_NA))
    {
        /* Terminal15 is no longer available, enter the deinit phase */
        app_state = APP_MAIN_STATE_OFF;
    }
    else
    {
        app_state = APP_MAIN_STATE_OPERATIONAL;
    }

//    if (app_state == APP_MAIN_STATE_OPERATIONAL)
//    {
//
//    }
    car_logic();

    if (g_app_data_model.btn_fresh == true)
    {
        keypad_set_input_from_model(g_app_data_model.btn_state);
        /* data received, restart timer */
        TIMER_RESET(SOFT_TIMER_1);
        g_app_data_model.btn_fresh = false;
    }
    if (g_app_data_model.btn_wheel_fresh == true)
    {
        keypad_set_input_from_model(g_app_data_model.btn_wheel_state);
        /* data received, restart timer */
        TIMER_RESET(SOFT_TIMER_1);
        g_app_data_model.btn_wheel_fresh = false;
    }

    /* Process timeout at some point */
    ON_TIMER_EXPIRED(500, SOFT_TIMER_1, keypad_reset_input());

    /* Execute the keypad logic */
    keypad_periodic(true);
static uint8_t send_cardata = 0;
    if (keypad_clicked(KEYPAD_BTN_WHEEL_DIAL_DOWN) == KEY_CLICK)
    {
        strcpy(textbox_tx_state.popup_left_text,   "1234");
        strcpy(textbox_tx_state.popup_center_text, "5678");
        strcpy(textbox_tx_state.popup_right_text,  "9123");
        loggerf("DialDown");
    }
    if (keypad_clicked(KEYPAD_BTN_WHEEL_DIAL_DOWN) == KEY_HOLD)
    {
        loggerf("DialDown Hold");
    }
    if (keypad_clicked(KEYPAD_BTN_WHEEL_DIAL_UP) == KEY_CLICK)
    {
        send_cardata++;
        loggerf("DialUp");
    }
    if (keypad_clicked(KEYPAD_BTN_WHEEL_DIAL_UP) == KEY_HOLD)
    {
        loggerf("DialUp Hold");
    }

    if (textbox_tx_state.popup_tx_state == TEXTBOX_TX_STATE_IDLE)
    {
        {
            if (send_cardata == 1)
            {
                snprintf(textbox_tx_state.popup_left_text, sizeof(textbox_tx_state.popup_left_text), "Key");
                snprintf(textbox_tx_state.popup_right_text, sizeof(textbox_tx_state.popup_left_text), "%d", g_app_data_model.key_state);
            }
            else if (send_cardata == 2)
            {
                snprintf(textbox_tx_state.popup_left_text, sizeof(textbox_tx_state.popup_left_text), "RPM");
                snprintf(textbox_tx_state.popup_right_text, sizeof(textbox_tx_state.popup_left_text), "%d", g_app_data_model.eng_speed);
            }
            else if (send_cardata == 3)
            {
                snprintf(textbox_tx_state.popup_left_text, sizeof(textbox_tx_state.popup_left_text), "TEMP");
                snprintf(textbox_tx_state.popup_right_text, sizeof(textbox_tx_state.popup_left_text), "%d", g_app_data_model.eng_coolant);
            }
            else if (send_cardata == 4)
            {
                snprintf(textbox_tx_state.popup_left_text, sizeof(textbox_tx_state.popup_left_text), "PAGE");
                snprintf(textbox_tx_state.popup_right_text, sizeof(textbox_tx_state.popup_left_text), "%d", g_app_data_model.display_page);
            }
            else
            {
                send_cardata = 0;
            }
        }
    }

    /********* OUTPUT PROCESSING ***********/

    /* Execute the logic only after 50ms after the last transaction on the network */
    ON_TIMER(50, SOFT_TIMER_4, {
            can_t display_frame;
            display_frame.id = 0x6C8UL;
            display_frame.length = 0;
            display_frame.flags.extended = 0;
            display_frame.flags.rtr = 0;

            if (textbox_tx_state.current_type == TEXTBOX_TYPE_LEFT)
            {
                show_textbox(TEXTBOX_TYPE_LEFT,
                             textbox_tx_state.popup_left_text,
                             &display_frame,
                             &textbox_tx_state.popup_tx_state);
                if (textbox_tx_state.popup_tx_state == TEXTBOX_TX_STATE_IDLE)   textbox_tx_state.current_type = TEXTBOX_TYPE_CENTER;
            }
            else if (textbox_tx_state.current_type == TEXTBOX_TYPE_CENTER)
            {
                show_textbox(TEXTBOX_TYPE_CENTER,
                             textbox_tx_state.popup_center_text,
                             &display_frame,
                             &textbox_tx_state.popup_tx_state);
                if (textbox_tx_state.popup_tx_state == TEXTBOX_TX_STATE_IDLE)   textbox_tx_state.current_type = TEXTBOX_TYPE_RIGHT;
            }
            else if (textbox_tx_state.current_type == TEXTBOX_TYPE_RIGHT)
            {
                show_textbox(TEXTBOX_TYPE_RIGHT,
                             textbox_tx_state.popup_right_text,
                             &display_frame,
                             &textbox_tx_state.popup_tx_state);
                if (textbox_tx_state.popup_tx_state == TEXTBOX_TX_STATE_IDLE)   textbox_tx_state.current_type = TEXTBOX_TYPE_LEFT;
            }

            if (display_frame.length > 0)
            {
                can_send_message_safe(&display_frame);
            }
    }, {
            /* Abort any transaction */
            textbox_tx_state.popup_tx_state = TEXTBOX_TX_STATE_IDLE;
    });



    return;

can_t rcv_msg;  /* the reception buffer */

if (/*can_check_message()*/false )// can_get_message(&rcv_msg))
{

    if (rcv_msg.id == 0x666)
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
        asd.data[7] = (uint8_t)(cpu_usage_us >> 24);
        //can_send_message_safe(&asd);

        /* Diagnose the FIFO full situation */
        // should be marked by the interrupt !!!
//        DEBUG_EXEC(
//                   ok = can_buffer_full(&mybuffer);
//                   if (ok == true) logger("RX FIFO FULL\n");
//                  );



        periodic_logic();
        extern uint32_t msgrxcnt;
        ON_TIMER_EXPIRED(1000, SOFT_TIMER_9, { loggerf("Alive %dl", msgrxcnt); car_print_debug(); TIMER_RESET(SOFT_TIMER_9); });

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
