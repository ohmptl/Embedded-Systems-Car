//------------------------------------------------------------------------------
//  Name:           motors.h
//  Description:    Header for motor control and movement (motors.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS
//------------------------------------------------------------------------------

#ifndef MOTORS_H_
#define MOTORS_H_

#include <stdint.h>

void Wheels_Process(void);
void motorStop(void);
void safetyCheck(void);
void wait_case(void);
void start_case(void);
void run_case(void);
void end_case(void);

// Project 7 variable speed motor control
void set_motor_speeds(unsigned int left_pwm, unsigned int right_pwm);
void pivot_left_pwm(unsigned int speed);
void pivot_right_pwm(unsigned int speed);
void tank_turn_left_pwm(unsigned int speed);
void tank_turn_right_pwm(unsigned int speed);
uint8_t Motor_ApplyJoystickVector(int16_t x_axis, int16_t y_axis, unsigned char engaged,
                                 unsigned int *applied_left_pwm, unsigned int *applied_right_pwm);
void Motor_JoystickFailsafeService(void);


//---------- PWM ------------//
// PWM Functions
// FORWARD
void PWM1_LEFT_FWD(void);
void PWM1_RIGHT_FWD(void);
void PWM1_BOTH_FWD(void);

// STOP
void PWM1_LEFT_OFF(void);
void PWM1_RIGHT_OFF(void);
void PWM1_BOTH_OFF(void);

// REVERSE
void PWM1_LEFT_REV(void);
void PWM1_RIGHT_REV(void);
void PWM1_BOTH_REV(void);


// PWM Motor Controls
#define PWM_PERIOD          (TB3CCR0)
#define LEFT_FORWARD_SPEED  (TB3CCR1)  // P6.0
#define RIGHT_FORWARD_SPEED (TB3CCR2)  // P6.1
#define LEFT_REVERSE_SPEED  (TB3CCR3)  // P6.2
#define RIGHT_REVERSE_SPEED (TB3CCR4)  // P6.3
#define LCD_BACKLITE_DIMING (TB3CCR5)  // P6.4

// PWM1 Values
// REMEMBER
    // Reverse or Forward must be 0 when the other is not 0.
    // DESIRED ON AMOUNT must be a value less than WHEEL_PERIOD
    // If WHEEL_PERIOD is 50,005 then
    //DESIRED ON AMOUNT of 20,000 would be ON NEAR 0%
    // NOTE: With SMCLK = 8MHz, PWM1_WHEEL_PERIOD = 50,005 -> ~160Hz PWM.
    //       Reduce the period (and rescale duty targets) if a higher, cooler PWM is required.
#define PWM1_WHEEL_PERIOD    (50005)
#define PWM1_WHEEL_OFF       (0)
#define PWM1_SLOW            (10000)
#define PWM1_FAST            (50000)
#define PWM1_PERCENT_100     (50000)
#define PWM1_PERCENT_80      (45000)
//---------------------------//

// Motor straight-line calibration scaling (thousandths == 100% duty)
// 1) Run the car forward on a flat surface at multiple speeds.
// 2) If the car drifts right, increase right trim or decrease left trim.
// 3) Adjust in small steps (5-10 counts) until the car tracks straight.
// 4) Mirror the forward values to the reverse trims if backward motion also drifts.
#define MOTOR_PWM_TRIM_SCALE        (1000u)
#define MOTOR_LEFT_FORWARD_TRIM     (880u)   // 90% of requested PWM to counter right drift
#define MOTOR_RIGHT_FORWARD_TRIM    (1000u)  // 100% of requested PWM
#define MOTOR_LEFT_REVERSE_TRIM     (950u)
#define MOTOR_RIGHT_REVERSE_TRIM    (1000u)

// Forward PWM safety window for proportional joystick control
// Tune MIN values just high enough that the wheels never stall when commanded on.
#define MOTOR_JOYSTICK_MIN_PWM          (11000u)
#define MOTOR_JOYSTICK_MAX_PWM          (45000u)
#define MOTOR_LEFT_SAFE_MIN_PWM         (8000u)
#define MOTOR_RIGHT_SAFE_MIN_PWM        (8000u)
#define MOTOR_JOYSTICK_AXIS_SCALE       (1000)
#define MOTOR_JOYSTICK_FORWARD_DEADBAND (40)      // 4% vertical deadband for idle
#define MOTOR_JOYSTICK_FAILSAFE_TICKS   (5u)      // auto stop after 1s without updates
#define MOTOR_JOYSTICK_TURN_MIN_SCALE   (0u)      // allow inner wheel to stop completely


#endif // MOTORS_H_
