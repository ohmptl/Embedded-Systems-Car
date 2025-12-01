//------------------------------------------------------------------------------
//
//  Description: This file contains the Main Routine - "While" Operating System
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#include  "msp430.h"
#include  <string.h>
#include  "LCD.h"
#include  "macros.h"
#include  "ports.h"
#include "functions.h"
#include  "states.h"
#include  "motors.h"
#include  "display.h"
#include  "serial.h"
#include  "wheels.h"
#include  "IR.h"

// Global Variables
extern char display_line[4][11];
extern char *display[4];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;
extern volatile unsigned int timer200ms;
extern volatile unsigned char one_time;
extern unsigned int IR;
extern unsigned int IRChange;
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
extern volatile unsigned int ADCThumb;

unsigned int backlight = OFF;             // backlight on off flag
unsigned char state;

// Project 7 button events (still used)
volatile unsigned char sw1_press_event;         // SW1 press detected
volatile unsigned char sw2_press_event;         // SW2 press detected (for calibration confirmation)

void main(void){
    // WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;
    bootSequence();

    backlight = ON;
    IR = ON;
    state = IDLE;
    motorStop();

    Serial_Project9_Init();
    IRLine_Init();

//------------------------------------------------------------------------------
// Begining of the "While" Operating System
//------------------------------------------------------------------------------
    while(ALWAYS) {                      
        update();

        Serial_Project9_Service();
        if (sw1_press_event) {
            sw1_press_event = 0;
            Serial_RequestWifiStatus();
        }

        if (sw2_press_event) {
            sw2_press_event = 0;
            Serial_RequestIpAddress();
        }
    }
//------------------------------------------------------------------------------
}

void update(void){
    Wheels_Process();  // Process movement commands
    Display_Process();
    backlight_update();
    IRLine_Service();
    IR_Update();
    P3OUT ^= TEST_PROBE;            // Change State of TEST_PROBE OFF
}
