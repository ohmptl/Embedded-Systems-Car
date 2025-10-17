//------------------------------------------------------------------------------
//  Name:           macros.h
//  Description:    Global Macros
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#ifndef MACROS_H_
#define MACROS_H_

#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define TRUE                    (0x01)
#define ON                      (0x01)
#define OFF                     (0x00)

// Homework 6 - LCD Blink timing using 5ms TB0 system tick (legacy)
#define LCD_BLINK_HALF_TICKS    (100)
#define IR_MAGIC_NUM           (750)

// Project 7 tuning constants (no magic numbers)
#define BASE_SPEED_PWM         (12000)   // nominal forward speed
#define TURN_SPEED_PWM         (9000)    // pivot speed
#define STEER_DELTA_PWM        (4000)    // differential added/subtracted for steering
#define PWM_MAX                (PWM1_WHEEL_PERIOD - 10)
#define PWM_MIN                (0)

// Lap detection: require at least N ticks (200ms each) between lap marks
#define MIN_LAP_TICKS          (25)      // 25 * 0.2s = 5 seconds minimum per lap

// Exit into center timing (rough)
#define EXIT_PIVOT_TICKS       (5)       // 1.0 seconds pivot
#define EXIT_DRIVE_TICKS       (5)       // 1.0 seconds forward

#endif /* MACROS_H_ */
