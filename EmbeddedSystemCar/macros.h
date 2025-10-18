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
#define BASE_SPEED_PWM         (10000)   // Use same as reference PWM1_SLOW
#define TURN_SPEED_PWM         (10000)   // pivot speed during initial turn
#define SLOW_TURN_PWM          (10000)   // same fixed speed

// Discrete steering - ON/OFF only (like reference project)
#define STEER_MODE_DISCRETE    (1)       // Use discrete ON/OFF steering

// Circle following: direction preference
#define FOLLOW_DIR_CW          ('R')     // Clockwise - right sensor focus
#define FOLLOW_DIR_CCW         ('L')     // Counter-clockwise - left sensor focus

#define PWM_MAX                (PWM1_WHEEL_PERIOD - 10)
#define PWM_MIN                (0)

// 0.2s tick timing (TimerB0 ISR increments a counter every 200ms)
#define TICKS_PER_SECOND        (5u)      // 5 ticks per second (0.2s per tick)
#define TICK_PERIOD_MS          (200u)

// Latching confirm while aligning on the circle (avoid false triggers)
#define ALIGN_CONFIRM_SECONDS   (0u)      // transition immediately once aligned

// Lap timing configuration (mapped from thumbwheel ADC)
#define LAPS_TARGET             (2u)      // exactly two laps
#define LAP_SECONDS_MIN         (20u)     // min allowable lap seconds
#define LAP_SECONDS_MAX         (60u)     // max allowable lap seconds
#define LAP_SECONDS_FIXED       (23u)     // empirically tuned lap duration

// Intercept trigger tuning (counts are post >>2 from ADC ISR)
#define INTERCEPT_MARGIN_MIN     (2u)      // minimum gap above white to consider black
#define INTERCEPT_MARGIN_DIVISOR (8u)      // use 12.5% of white-to-black delta as margin

// Intercept post-detection maneuver timing (0.2s ticks)
#define INTERCEPT_BACKUP_TICKS      (2u)   // back up ~0.6s before pivot
#define INTERCEPT_PAUSE_BEFORE_REV  (1u)   // pause after detection before reversing
#define INTERCEPT_PAUSE_BEFORE_TURN (1u)   // pause after reverse before pivot

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
#define EXIT_PIVOT_TICKS       (8)      // 2.0 seconds pivot
#define EXIT_DRIVE_TICKS       (10)      // 2.0 seconds forward

#endif /* MACROS_H_ */
