/*
 * debug.h
 *
 *  Created on: 22 set 2018
 *      Author: lorenzo
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define DEBUG   1

#ifdef DEBUG
#define DEBUG_EXEC(x) do { \
                        x  \
                      } while(0);
#else
#define DEBUG_EXEC(x) while(0) { x };
#endif

#endif /* DEBUG_H_ */
