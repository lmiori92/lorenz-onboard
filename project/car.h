/*
 * car.h
 *
 *  Created on: 29 set 2019
 *      Author: lorenzo
 */

#ifndef CAR_H_
#define CAR_H_

#include <stdbool.h>
#include <stdint.h>

/* All operations are performed in integer computations.
 * Floats are definitely too slow on the AVR platform. */

/*
 * Gear ratios:
 * 1st ratio: 3.82
 * 2nd ratio: 2.05
 * 3rd ratio: 1.30
 * 4th ratio: 0.96
 * 5th ratio: 0.74
 * 6th ratio: 0.61
 * reverse ratio: 3.55
 * final drive ratio: 3.65
 */
#define Z19DTH_M32_GEARBOX_RATIO_1ST    (382U)
#define Z19DTH_M32_GEARBOX_RATIO_2ND    (205U)
#define Z19DTH_M32_GEARBOX_RATIO_3RD    (130U)
#define Z19DTH_M32_GEARBOX_RATIO_4TH    ( 96U)
#define Z19DTH_M32_GEARBOX_RATIO_5TH    ( 74U)
#define Z19DTH_M32_GEARBOX_RATIO_6TH    ( 61U)
#define Z19DTH_M32_GEARBOX_RATIO_REV    (355U)
#define Z19DTH_M32_GEARBOX_RATIO_AXLE   (365U)

#define GEAR_RATIO_ERROR_RANGE          (8U)

#define Z19DTH_M32_THR_GEARBOX_RATIO_1ST    (Z19DTH_M32_GEARBOX_RATIO_1ST - GEAR_RATIO_ERROR_RANGE)
#define Z19DTH_M32_THR_GEARBOX_RATIO_2ND    (Z19DTH_M32_GEARBOX_RATIO_2ND - GEAR_RATIO_ERROR_RANGE)
#define Z19DTH_M32_THR_GEARBOX_RATIO_3RD    (Z19DTH_M32_GEARBOX_RATIO_3RD - GEAR_RATIO_ERROR_RANGE)
#define Z19DTH_M32_THR_GEARBOX_RATIO_4TH    (Z19DTH_M32_GEARBOX_RATIO_4TH - GEAR_RATIO_ERROR_RANGE)
#define Z19DTH_M32_THR_GEARBOX_RATIO_5TH    (Z19DTH_M32_GEARBOX_RATIO_5TH - GEAR_RATIO_ERROR_RANGE)
#define Z19DTH_M32_THR_GEARBOX_RATIO_6TH    (Z19DTH_M32_GEARBOX_RATIO_6TH - GEAR_RATIO_ERROR_RANGE)
#define Z19DTH_M32_THR_GEARBOX_RATIO_REV    (Z19DTH_M32_GEARBOX_RATIO_REV - GEAR_RATIO_ERROR_RANGE)


void car_logic(void);
void car_print_debug(void);

#endif /* CAR_H_ */
