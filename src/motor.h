/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

extern void initPWM (void);
extern void moveAbsolute (uint16_t microSteps);
extern void moveRightRelative (uint16_t microSteps);
extern void moveLeftRelative (uint16_t microSteps);
extern void rotate (void);

#endif /* MOTOR_H_ */
