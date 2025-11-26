//------------------------------------------------------------------------------
//
//  Description: This file contains the movement functions
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

#include  <string.h>
#include  <stdint.h>
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "msp430.h"
#include  "motors.h"
#include  "states.h"

// Globals
extern unsigned char state;
extern unsigned char event;
extern char display_line[4][11];
extern volatile unsigned char display_changed;
extern volatile unsigned int timer200ms;

// Forward declarations for PWM calibration helpers
static unsigned int apply_trim_value(unsigned int pwm, unsigned int trim);
static void apply_forward_trim(unsigned int *left_pwm, unsigned int *right_pwm);
static void apply_reverse_trim(unsigned int *left_pwm, unsigned int *right_pwm);
static unsigned int clamp_to_period(unsigned int value);

// Joystick control helpers
static void Motor_JoystickStopInternal(void);
static unsigned int Motor_NormalizeAxis(int16_t value);
static unsigned int Motor_ComputeBasePwm(unsigned int forward_axis);
static unsigned int Motor_ComputeWheelPwm(unsigned int base_pwm, unsigned int scale_thousandths);
static int16_t Motor_ClampSignedAxis(int32_t value);
static uint8_t Motor_AxisIsActive(int16_t axis_value);
static unsigned int Motor_ComputeSignedAxisPwm(int16_t axis_value);
static void Motor_ApplyWheelAxis(int16_t axis_value, unsigned char is_left, unsigned int *applied_pwm);
static void Motor_ApplyCarSteering(int16_t throttle_axis, int16_t steering_axis,
                                   unsigned int *applied_left_pwm, unsigned int *applied_right_pwm);
static void Motor_ApplyTankTurnAxis(int16_t steering_axis,
                                   unsigned int *applied_left_pwm, unsigned int *applied_right_pwm);
static unsigned int Motor_ComputeTurnInnerScale(unsigned int steering_magnitude);
static void Motor_WriteForwardSpeeds(unsigned int left_pwm, unsigned int right_pwm,
                                     unsigned int *applied_left_pwm, unsigned int *applied_right_pwm);
static void Motor_WriteReverseSpeeds(unsigned int left_pwm, unsigned int right_pwm,
                                     unsigned int *applied_left_pwm, unsigned int *applied_right_pwm);

static volatile unsigned int joystick_last_update_tick = 0u;
static volatile unsigned char joystick_active = 0u;

//------------------------------------------------------------------------------
// Motor Control Functions
//------------------------------------------------------------------------------

void motorStop(void){
    // Ensure PWM duty cycles are cleared before cutting power to protect FETs
    PWM1_BOTH_OFF();

    // Turn OFF ALL Motors (drive pins low as a secondary safeguard)
    P6OUT &= ~R_FORWARD;
    P6OUT &= ~L_FORWARD;
    P6OUT &= ~R_REVERSE;
    P6OUT &= ~L_REVERSE;
}

//------------------------------------------------------------------------------
// PWM1 Motor Control - SLOW Speed
//------------------------------------------------------------------------------

// FORWARD
void PWM1_LEFT_FWD(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_SLOW;
}

void PWM1_RIGHT_FWD(void){
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_SLOW;
}

void PWM1_BOTH_FWD(void){
    unsigned int left_pwm = PWM1_SLOW;
    unsigned int right_pwm = PWM1_SLOW;

    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;

    apply_forward_trim(&left_pwm, &right_pwm);

    LEFT_FORWARD_SPEED = left_pwm;
    RIGHT_FORWARD_SPEED = right_pwm;
}

// OFF
void PWM1_LEFT_OFF(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
}

void PWM1_RIGHT_OFF(void){
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
}

void PWM1_BOTH_OFF(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
}

// REVERSE
void PWM1_LEFT_REV(void){
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = PWM1_SLOW;
}

void PWM1_RIGHT_REV(void){
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_SLOW;
}

void PWM1_BOTH_REV(void){
    unsigned int left_pwm = PWM1_SLOW;
    unsigned int right_pwm = PWM1_SLOW;

    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;

    apply_reverse_trim(&left_pwm, &right_pwm);

    LEFT_REVERSE_SPEED = left_pwm;
    RIGHT_REVERSE_SPEED = right_pwm;
}

//------------------------------------------------------------------------------
// Safety and Advanced Control
//------------------------------------------------------------------------------

void safetyCheck(void) {
    if ((R_FORWARD && R_REVERSE) || (L_FORWARD && L_REVERSE)) {
        motorStop(); // Emergency stop
        strcpy(display_line[0], "  SAFETY  ");
        strcpy(display_line[1], " CONFLICT ");  
        strcpy(display_line[2], "  MOTORS  ");
        strcpy(display_line[3], "          ");
        display_changed = TRUE;
        return;
    }
}

// Variable speed motor control (Project 7)
void set_motor_speeds(unsigned int left_pwm, unsigned int right_pwm) {
    unsigned int left = left_pwm;
    unsigned int right = right_pwm;

    apply_forward_trim(&left, &right);

    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = left;
    RIGHT_FORWARD_SPEED = right;
}

// Pivot left: left wheel stopped, right forward
void pivot_left_pwm(unsigned int speed) {
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = speed;
}

// Pivot right: right wheel stopped, left forward
void pivot_right_pwm(unsigned int speed) {
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = speed;
}

//------------------------------------------------------------------------------
// Joystick proportional control
//------------------------------------------------------------------------------

uint8_t Motor_ApplyJoystickVector(int16_t x_axis, int16_t y_axis, unsigned char engaged,
                                 unsigned int *applied_left_pwm, unsigned int *applied_right_pwm) {
    if (applied_left_pwm) {
        *applied_left_pwm = 0u;
    }
    if (applied_right_pwm) {
        *applied_right_pwm = 0u;
    }

    if (!engaged) {
        Motor_JoystickStopInternal();
        return 1u;
    }

    if (x_axis > MOTOR_JOYSTICK_AXIS_SCALE) {
        x_axis = MOTOR_JOYSTICK_AXIS_SCALE;
    } else if (x_axis < -MOTOR_JOYSTICK_AXIS_SCALE) {
        x_axis = (int16_t)(-MOTOR_JOYSTICK_AXIS_SCALE);
    }
    if (y_axis > MOTOR_JOYSTICK_AXIS_SCALE) {
        y_axis = MOTOR_JOYSTICK_AXIS_SCALE;
    } else if (y_axis < -MOTOR_JOYSTICK_AXIS_SCALE) {
        y_axis = (int16_t)(-MOTOR_JOYSTICK_AXIS_SCALE);
    }

    uint8_t throttle_active = Motor_AxisIsActive(y_axis);
    uint8_t steering_active = Motor_AxisIsActive(x_axis);

    if (!throttle_active && !steering_active) {
        Motor_JoystickStopInternal();
        return 1u;
    }

    if (throttle_active) {
        Motor_ApplyCarSteering(y_axis, x_axis, applied_left_pwm, applied_right_pwm);
    } else {
        Motor_ApplyTankTurnAxis(x_axis, applied_left_pwm, applied_right_pwm);
    }

    joystick_active = 1u;
    joystick_last_update_tick = timer200ms;

    return 1u;
}

static int16_t Motor_ClampSignedAxis(int32_t value) {
    if (value > MOTOR_JOYSTICK_AXIS_SCALE) {
        return (int16_t)MOTOR_JOYSTICK_AXIS_SCALE;
    }
    if (value < -MOTOR_JOYSTICK_AXIS_SCALE) {
        return (int16_t)(-MOTOR_JOYSTICK_AXIS_SCALE);
    }
    return (int16_t)value;
}

static uint8_t Motor_AxisIsActive(int16_t axis_value) {
    int16_t magnitude = (axis_value >= 0) ? axis_value : (int16_t)(-axis_value);
    unsigned int normalized = Motor_NormalizeAxis(magnitude);
    return (normalized > MOTOR_JOYSTICK_FORWARD_DEADBAND) ? 1u : 0u;
}

static unsigned int Motor_ComputeSignedAxisPwm(int16_t axis_value) {
    int16_t magnitude = (axis_value >= 0) ? axis_value : (int16_t)(-axis_value);
    unsigned int normalized = Motor_NormalizeAxis(magnitude);
    if (normalized <= MOTOR_JOYSTICK_FORWARD_DEADBAND) {
        return 0u;
    }
    return Motor_ComputeBasePwm(normalized);
}

static void Motor_ApplyWheelAxis(int16_t axis_value, unsigned char is_left, unsigned int *applied_pwm) {
    unsigned int pwm = Motor_ComputeSignedAxisPwm(axis_value);
    unsigned int safe_min = is_left ? MOTOR_LEFT_SAFE_MIN_PWM : MOTOR_RIGHT_SAFE_MIN_PWM;
    unsigned int trimmed;

    if (applied_pwm) {
        *applied_pwm = 0u;
    }

    if (pwm == 0u) {
        if (is_left) {
            LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
            LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
        } else {
            RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
            RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
        }
        return;
    }

    if (axis_value > 0) {
        trimmed = apply_trim_value(pwm, is_left ? MOTOR_LEFT_FORWARD_TRIM : MOTOR_RIGHT_FORWARD_TRIM);
        if ((trimmed > 0u) && (trimmed < safe_min)) {
            trimmed = safe_min;
        }
        if (is_left) {
            LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
            LEFT_FORWARD_SPEED = trimmed;
        } else {
            RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
            RIGHT_FORWARD_SPEED = trimmed;
        }
    } else {
        trimmed = apply_trim_value(pwm, is_left ? MOTOR_LEFT_REVERSE_TRIM : MOTOR_RIGHT_REVERSE_TRIM);
        if ((trimmed > 0u) && (trimmed < safe_min)) {
            trimmed = safe_min;
        }
        if (is_left) {
            LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
            LEFT_REVERSE_SPEED = trimmed;
        } else {
            RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
            RIGHT_REVERSE_SPEED = trimmed;
        }
    }

    if (applied_pwm) {
        *applied_pwm = trimmed;
    }
}

static void Motor_ApplyCarSteering(int16_t throttle_axis, int16_t steering_axis,
                                   unsigned int *applied_left_pwm, unsigned int *applied_right_pwm) {
    int16_t throttle_mag = (throttle_axis >= 0) ? throttle_axis : (int16_t)(-throttle_axis);
    unsigned int normalized = Motor_NormalizeAxis(throttle_mag);
    unsigned int base_pwm = Motor_ComputeBasePwm(normalized);

    if (base_pwm == 0u) {
        Motor_JoystickStopInternal();
        return;
    }

    unsigned int left_pwm = base_pwm;
    unsigned int right_pwm = base_pwm;
    int16_t steering_mag_signed = (steering_axis >= 0) ? steering_axis : (int16_t)(-steering_axis);
    unsigned int steering_mag = Motor_NormalizeAxis(steering_mag_signed);

    if (steering_mag > MOTOR_JOYSTICK_FORWARD_DEADBAND) {
        unsigned int inner_scale = Motor_ComputeTurnInnerScale(steering_mag);
        if (steering_axis > 0) {
            right_pwm = Motor_ComputeWheelPwm(base_pwm, inner_scale);
        } else if (steering_axis < 0) {
            left_pwm = Motor_ComputeWheelPwm(base_pwm, inner_scale);
        }
    }

    if (throttle_axis >= 0) {
        Motor_WriteForwardSpeeds(left_pwm, right_pwm, applied_left_pwm, applied_right_pwm);
    } else {
        Motor_WriteReverseSpeeds(left_pwm, right_pwm, applied_left_pwm, applied_right_pwm);
    }
}

static void Motor_ApplyTankTurnAxis(int16_t steering_axis,
                                   unsigned int *applied_left_pwm, unsigned int *applied_right_pwm) {
    int32_t left_sum = -(int32_t)steering_axis;
    int32_t right_sum = (int32_t)steering_axis;
    int16_t left_axis = Motor_ClampSignedAxis(left_sum);
    int16_t right_axis = Motor_ClampSignedAxis(right_sum);

    if (!Motor_AxisIsActive(left_axis) && !Motor_AxisIsActive(right_axis)) {
        Motor_JoystickStopInternal();
        return;
    }

    Motor_ApplyWheelAxis(left_axis, 1u, applied_left_pwm);
    Motor_ApplyWheelAxis(right_axis, 0u, applied_right_pwm);
}

static unsigned int Motor_ComputeTurnInnerScale(unsigned int steering_magnitude) {
    if (steering_magnitude >= MOTOR_JOYSTICK_AXIS_SCALE) {
        return MOTOR_JOYSTICK_TURN_MIN_SCALE;
    }

    unsigned long range = (unsigned long)(MOTOR_JOYSTICK_AXIS_SCALE - MOTOR_JOYSTICK_TURN_MIN_SCALE);
    unsigned long scaled = (range * (unsigned long)steering_magnitude) / MOTOR_JOYSTICK_AXIS_SCALE;
    unsigned long result = (unsigned long)MOTOR_JOYSTICK_AXIS_SCALE - scaled;

    if (result < MOTOR_JOYSTICK_TURN_MIN_SCALE) {
        result = MOTOR_JOYSTICK_TURN_MIN_SCALE;
    }

    return (unsigned int)result;
}

static void Motor_WriteForwardSpeeds(unsigned int left_pwm, unsigned int right_pwm,
                                     unsigned int *applied_left_pwm, unsigned int *applied_right_pwm) {
    if ((left_pwm > 0u) && (left_pwm < MOTOR_LEFT_SAFE_MIN_PWM)) {
        left_pwm = MOTOR_LEFT_SAFE_MIN_PWM;
    }
    if ((right_pwm > 0u) && (right_pwm < MOTOR_RIGHT_SAFE_MIN_PWM)) {
        right_pwm = MOTOR_RIGHT_SAFE_MIN_PWM;
    }

    apply_forward_trim(&left_pwm, &right_pwm);

    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = left_pwm;
    RIGHT_FORWARD_SPEED = right_pwm;

    if (applied_left_pwm) {
        *applied_left_pwm = left_pwm;
    }
    if (applied_right_pwm) {
        *applied_right_pwm = right_pwm;
    }
}

static void Motor_WriteReverseSpeeds(unsigned int left_pwm, unsigned int right_pwm,
                                     unsigned int *applied_left_pwm, unsigned int *applied_right_pwm) {
    if ((left_pwm > 0u) && (left_pwm < MOTOR_LEFT_SAFE_MIN_PWM)) {
        left_pwm = MOTOR_LEFT_SAFE_MIN_PWM;
    }
    if ((right_pwm > 0u) && (right_pwm < MOTOR_RIGHT_SAFE_MIN_PWM)) {
        right_pwm = MOTOR_RIGHT_SAFE_MIN_PWM;
    }

    apply_reverse_trim(&left_pwm, &right_pwm);

    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = left_pwm;
    RIGHT_REVERSE_SPEED = right_pwm;

    if (applied_left_pwm) {
        *applied_left_pwm = left_pwm;
    }
    if (applied_right_pwm) {
        *applied_right_pwm = right_pwm;
    }
}

void Motor_JoystickFailsafeService(void) {
    if (!joystick_active) {
        return;
    }

    unsigned int elapsed = timer200ms - joystick_last_update_tick;
    if (elapsed >= MOTOR_JOYSTICK_FAILSAFE_TICKS) {
        Motor_JoystickStopInternal();
    }
}

//------------------------------------------------------------------------------
// PWM calibration helpers
//------------------------------------------------------------------------------

static unsigned int clamp_to_period(unsigned int value) {
    if (value > PWM1_WHEEL_PERIOD) {
        return PWM1_WHEEL_PERIOD;
    }
    return value;
}

static unsigned int apply_trim_value(unsigned int pwm, unsigned int trim) {
    unsigned long scaled = (unsigned long)pwm * (unsigned long)trim;
    scaled += (MOTOR_PWM_TRIM_SCALE / 2u); // integer rounding
    scaled /= MOTOR_PWM_TRIM_SCALE;
    scaled = clamp_to_period((unsigned int)scaled);
    return (unsigned int)scaled;
}

static void apply_forward_trim(unsigned int *left_pwm, unsigned int *right_pwm) {
    if (left_pwm && *left_pwm) {
        *left_pwm = apply_trim_value(*left_pwm, MOTOR_LEFT_FORWARD_TRIM);
    }
    if (right_pwm && *right_pwm) {
        *right_pwm = apply_trim_value(*right_pwm, MOTOR_RIGHT_FORWARD_TRIM);
    }
}

static void apply_reverse_trim(unsigned int *left_pwm, unsigned int *right_pwm) {
    if (left_pwm && *left_pwm) {
        *left_pwm = apply_trim_value(*left_pwm, MOTOR_LEFT_REVERSE_TRIM);
    }
    if (right_pwm && *right_pwm) {
        *right_pwm = apply_trim_value(*right_pwm, MOTOR_RIGHT_REVERSE_TRIM);
    }
}

static void Motor_JoystickStopInternal(void) {
    if (joystick_active) {
        motorStop();
    }
    joystick_active = 0u;
}

static unsigned int Motor_NormalizeAxis(int16_t value) {
    if (value < 0) {
        return 0u;
    }
    if (value > MOTOR_JOYSTICK_AXIS_SCALE) {
        return MOTOR_JOYSTICK_AXIS_SCALE;
    }
    return (unsigned int)value;
}

static unsigned int Motor_ComputeBasePwm(unsigned int forward_axis) {
    if (forward_axis <= MOTOR_JOYSTICK_FORWARD_DEADBAND) {
        return 0u;
    }

    unsigned int normalized = forward_axis - MOTOR_JOYSTICK_FORWARD_DEADBAND;
    unsigned int span = MOTOR_JOYSTICK_AXIS_SCALE - MOTOR_JOYSTICK_FORWARD_DEADBAND;
    unsigned long pwm_range = (unsigned long)(MOTOR_JOYSTICK_MAX_PWM - MOTOR_JOYSTICK_MIN_PWM);
    unsigned long scaled = pwm_range * (unsigned long)normalized;
    scaled /= span;
    scaled += MOTOR_JOYSTICK_MIN_PWM;
    return clamp_to_period((unsigned int)scaled);
}

static unsigned int Motor_ComputeWheelPwm(unsigned int base_pwm, unsigned int scale_thousandths) {
    if ((base_pwm == 0u) || (scale_thousandths == 0u)) {
        return 0u;
    }
    unsigned long scaled = (unsigned long)base_pwm * (unsigned long)scale_thousandths;
    scaled /= MOTOR_JOYSTICK_AXIS_SCALE;
    return clamp_to_period((unsigned int)scaled);
}

