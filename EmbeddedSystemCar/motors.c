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
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_SLOW;
    RIGHT_FORWARD_SPEED = PWM1_SLOW;
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
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = PWM1_SLOW;
    RIGHT_REVERSE_SPEED = PWM1_SLOW;
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
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = left_pwm;
    RIGHT_FORWARD_SPEED = right_pwm;
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

