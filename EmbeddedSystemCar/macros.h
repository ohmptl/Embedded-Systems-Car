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
#define BASE_SPEED_PWM         (15000)   // nominal forward speed (increased for better momentum)
#define TURN_SPEED_PWM         (9000)    // pivot speed
#define SLOW_SPEED_PWM         (8000)    // speed during sharp corrections
#define STEER_DELTA_PWM        (6000)    // differential added/subtracted for steering (increased)
#define LOST_LINE_DELTA_PWM    (10000)   // aggressive correction when line is lost
#define CLOCKWISE_BIAS_PWM     (2000)    // constant bias for clockwise motion (left motor faster)
#define PWM_MAX                (PWM1_WHEEL_PERIOD - 10)
#define PWM_MIN                (0)

// Line following states
#define LINE_CENTER            (0)       // Both sensors on white (following center)
#define LINE_LEFT              (1)       // Left sensor sees black
#define LINE_RIGHT             (2)       // Right sensor sees black
#define LINE_BOTH              (3)       // Both sensors see black (crossing or aligned)
#define LINE_LOST              (4)       // Both sensors on white (line lost)

// Lap detection: for a 36" diameter circle at BASE_SPEED_PWM
// Circumference ≈ 113 inches; at ~3-4 inches/sec → ~30-40 seconds per lap
// Use a longer minimum time between lap detections
#define MIN_LAP_TICKS          (100)     // 100 * 0.2s = 20 seconds minimum per lap

// Exit into center timing (rough)
#define EXIT_PIVOT_TICKS       (10)      // 2.0 seconds pivot
#define EXIT_DRIVE_TICKS       (10)      // 2.0 seconds forward

#endif /* MACROS_H_ */
