//------------------------------------------------------------------------------
//
//  Description: This file contains switches
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0 new
//
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"

// Globals ---------------------------------------------------------------------
extern unsigned char dispEvent;                   // Track display state
extern volatile unsigned char display_changed;    // Track change y/n
extern unsigned char event;                       // set event flag for motors.c
extern char display_line[4][11];                  // 2-D char array for display 

extern unsigned int straight_step;
extern unsigned int circle_step;
extern unsigned int circle_step2;
extern unsigned int triangle_step;
extern unsigned int figure8_step;

extern unsigned char state;

// TEST
extern int Switch1_Pressed;
extern int Switch2_Pressed;
int okay_to_look_at_switch1=1;
int count_debounce_SW1;
int sw1_position=1;
int okay_to_look_at_switch2=1;
int count_debounce_SW2;
int sw2_position=1;
extern volatile unsigned int debounce_count1;
extern volatile unsigned int debounce_count2;
extern unsigned int backlight;
extern unsigned int secTime;

// Provide storage for debounce status flags referenced by timer ISR
// Initialize to OFF so backlight can toggle when idle
char debounce_Status_SW1 = OFF;
char debounce_Status_SW2 = OFF;


// Switch Functions ------------------------------------------------------------

void Switches_Process(void){    // Error with alr being defined in switch.obj

    Switch1_Process();
    Switch2_Process();

}

void Switch1_Process(void){
// Switch Setup-----------------------------------------------------------------
    if (okay_to_look_at_switch1 && sw1_position){
        if (!(P4IN & SW1)){
            sw1_position = PRESSED;
            okay_to_look_at_switch1 = NOT_OKAY;
            count_debounce_SW1 = DEBOUNCE_RESTART;
//------------------------------------------------------------------------------
            backlight = ON;
            state = WAIT;
            event = GOFORWARD1;
            secTime = 2;
//------------------------------------------------------------------------------
        }
    }
    if (count_debounce_SW1 <= DEBOUNCE_TIME){
        count_debounce_SW1++;
    } else{
        okay_to_look_at_switch1 = OKAY;
        if (P4IN & SW1){
            sw1_position = RELEASED;
        }
    }
//------------------------------------------------------------------------------
}


void Switch2_Process(void){
// Switch Setup-----------------------------------------------------------------
    if (okay_to_look_at_switch2 && sw2_position){
        if (!(P2IN & SW2)){
            sw2_position = PRESSED;
            okay_to_look_at_switch2 = NOT_OKAY;
            count_debounce_SW2 = DEBOUNCE_RESTART;
//------------------------------------------------------------------------------
// Enter Switch logic here
// Switch Setup-----------------------------------------------------------------
        }
    }
    if (count_debounce_SW2 <= DEBOUNCE_TIME){
        count_debounce_SW2++;
    }else{
        okay_to_look_at_switch2 = OKAY;
        if (P2IN & SW2){
            sw2_position = RELEASED;
        }
    }
//------------------------------------------------------------------------------
}


