/*
 * can_hal.h
 *
 *  Created on: 25 set 2019
 *      Author: lorenzo
 */

#ifndef CAN_HAL_H_
#define CAN_HAL_H_

#include "can.h"
#include "can_buffer.h"

extern can_buffer_t mybuffer;
extern bool         mybuffer_overrun;

void can_hal_init(void);
void can_send_message_safe(can_t *frame);

#endif /* CAN_HAL_H_ */
