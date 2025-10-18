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
// PWM speeds (out of 50005 period)
#define BASE_SPEED_PWM         (12000)   // nominal forward speed for both wheels
#define TURN_SPEED_PWM         (10000)   // pivot speed during initial turn
#define SLOW_TURN_PWM          (9000)    // slower wheel during line follow corrections

// Steering corrections - proportional control
#define MINOR_CORRECTION_PWM   (1000)    // small adjustment when slightly off
#define MAJOR_CORRECTION_PWM   (2500)    // larger adjustment when way off
#define LOST_RECOVERY_PWM      (5000)    // aggressive when line is completely lost

// Circle following: for clockwise, right sensor tracks line
#define CLOCKWISE_BIAS_PWM     (500)     // slight left bias to maintain circular path
#define PWM_MAX                (PWM1_WHEEL_PERIOD - 10)
#define PWM_MIN                (0)

// Line position tracking (for proper correction direction)
#define LINE_NONE              (0)       // Both sensors on white (center or lost)
#define LINE_LEFT              (1)       // Left sensor sees black (too far left)
#define LINE_RIGHT             (2)       // Right sensor sees black (correct for clockwise)
#define LINE_BOTH              (3)       // Both sensors see black (crossing or lap marker)

// Lap detection: 36" diameter circle ≈ 113" circumference
// At 12000 PWM (~2-3 inches/sec) → ~40-50 seconds per lap
// Set minimum to 30 seconds to avoid false triggers
#define MIN_LAP_TICKS          (150)     // 150 * 0.2s = 30 seconds minimum per lap
#define LAP_DEBOUNCE_TICKS     (5)       // 1 second to clear lap marker

// Exit into center timing (rough)
#define EXIT_PIVOT_TICKS       (10)      // 2.0 seconds pivot
#define EXIT_DRIVE_TICKS       (10)      // 2.0 seconds forward

#endif /* MACROS_H_ */
