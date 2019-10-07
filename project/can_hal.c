
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>
#include <stdbool.h>

#include "logger.h"

/* CAN includes */
#include "mcp2515_defs.h"
#include "mcp2515_private.h"
#include "can.h"
#include "avr-can-lib/src/can_buffer.h"     // to be replaced with lorenz-utils

#define CAN_RX_BUFFER_COUNT     (16)

can_buffer_t mybuffer;
can_t        mybuffer_items[CAN_RX_BUFFER_COUNT];
bool         mybuffer_overrun = false;

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

void can_hal_init(void)
{
    /* Setup the RX FIFO */
    can_buffer_init(&mybuffer, CAN_RX_BUFFER_COUNT, mybuffer_items);

    /* Setup the PCINT1 CAN Controller interrupt pin */
    PCICR |= (1 << PCIE0);     // set PCIE0 to enable PCMSK0 scan
    PCMSK0 |= (1 << PCINT0);   // set PCINT0 to trigger an interrupt on state change

    bool ok = FALSE;
    do
    {
        /* it can happen that we have a failure at boot e.g. CAN controller is starting
         * Retry indefinitely: the application won't run without a CANbus network! */
        ok = can_init(BITRATE_95_238_KBPS);
    }
    while(ok == false);
    logger("initialized canbus");

    /* load filters and masks */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        can_static_filter(can_filter);
    }
}

// Disable the RX interrupt first and then transmit. Finally, re-enable the isr
void can_send_message_safe(can_t *frame)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        (void)can_send_message(frame);
    }
}

/* CAN RX buffer: 12 items, as
 * we can have roughly 6 frames in 10 milliseconds
 * and we double that to account for errors and to be on the safe
 * side:
 * With STD. frames, 11 bytes min. are necessary (8 data, 1 dlc, 2 ID)
 * Hence, 132 byte of SRAM
 */
uint32_t msgrxcnt = 0;
ISR (PCINT0_vect)
{
    can_t        *rx_can_frame;
    uint8_t maxrx = 0;
    /* Check if the INT pin is LOW (Frame Rx):
     * keep doing that for at most 2 times in a row (2 RX buffers) */
    while (((PINB & (1<<PINB0)) == 0) && (maxrx <= 2))
    {
        maxrx++;
        msgrxcnt++;
        /* The operation takes 102us (measured with oscilloscope),
         * half the main timer tick */
        rx_can_frame = can_buffer_get_enqueue_ptr(&mybuffer);
        if (rx_can_frame != NULL)
        {
            can_get_message(rx_can_frame);
            can_buffer_enqueue(&mybuffer);
        }
        else
        {
            /* Read and discard frame */
            can_t asd;
            can_get_message(&asd);
            mybuffer_overrun = true;
        }
    }
}
