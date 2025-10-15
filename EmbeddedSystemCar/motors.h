//------------------------------------------------------------------------------
//  Name:           motors.h
//  Description:    Header for motor control and movement (motors.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS
//------------------------------------------------------------------------------

#ifndef MOTORS_H_
#define MOTORS_H_

void Wheels_Process(void);
void motorStop(void);
void safetyCheck(void);
void wait_case(void);
void start_case(void);
void run_case(void);
void end_case(void);


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
#define PWM1_WHEEL_PERIOD    (50005)
#define PWM1_WHEEL_OFF       (0)
#define PWM1_SLOW            (10000)
#define PWM1_FAST            (50000)
#define PWM1_PERCENT_100     (50000)
#define PWM1_PERCENT_80      (45000)
//---------------------------//

#endif // MOTORS_H_
