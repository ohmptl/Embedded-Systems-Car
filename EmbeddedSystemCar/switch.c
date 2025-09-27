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
extern int Switch_Counter1;
extern int Switch_Counter2;
int okay_to_look_at_switch1=1;
int count_debounce_SW1;
int sw1_position=1;
int okay_to_look_at_switch2=1;
int count_debounce_SW2;
int sw2_position=1;
extern volatile unsigned int debounce_count1;
extern volatile unsigned int debounce_count2;
extern unsigned int backlight;

// HW5
extern int port3_4;

// Switch Functions ------------------------------------------------------------

// void Switches_Process(void){    // Error with alr being defined in switch.obj

//     Switch1_Process();
//     Switch2_Process();

// }

void Switch1_Process(void){
// Switch Setup-----------------------------------------------------------------
    if (okay_to_look_at_switch1 && sw1_position){
        if (!(P4IN & SW1)){
            sw1_position = PRESSED;
            okay_to_look_at_switch1 = NOT_OKAY;
            count_debounce_SW1 = DEBOUNCE_RESTART;
//------------------------------------------------------------------------------
            port3_4 = USE_GPIO;
            Init_Ports();                        // Initialize Ports
            Init_Clocks();                       // Initialize Clock System
            Init_Conditions();                   // Initialize Variables and Initial Conditions
            Init_Timers();                       // Initialize Timers
            Init_LCD();                          // Initialize LCD
            backlight = ON;
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], " Port 3.4 ");
            strcpy(display_line[2], "   GPIO   ");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;

            // if(event == NONE){
            //     backlight_status = 1;
            //     switch(dispEvent){
            //     case NONE:
            //         dispEvent  = STRAIGHT;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "- Select -");
            //         strcpy(display_line[2], " Straight ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         break;
            //     case STRAIGHT:
            //         dispEvent = CIRCLE;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "- Select -");
            //         strcpy(display_line[2], "  Circle  ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         break;
            //     case CIRCLE:
            //         dispEvent = TRIANGLE;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "- Select -");
            //         strcpy(display_line[2], " Triangle ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         break;
            //     case TRIANGLE:
            //         dispEvent = FIGURE8C1;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "- Select -");
            //         strcpy(display_line[2], " Figure 8 ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         break;
            //     case FIGURE8C1:
            //         dispEvent = STRAIGHT;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "- Select -");
            //         strcpy(display_line[2], " Straight ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         break;
            //     default:
            //         break;
            //     }
            // }
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
            port3_4 = USE_SMCLK;
            Init_Ports();                        // Initialize Ports
            Init_Clocks();                       // Initialize Clock System
            Init_Conditions();                   // Initialize Variables and Initial Conditions
            Init_Timers();                       // Initialize Timers
            Init_LCD();                          // Initialize LCD
            backlight = ON;
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], " Port 3.4 ");
            strcpy(display_line[2], "   SMCLK  ");
            strcpy(display_line[3], "          ");
            display_changed = TRUE;     
            
            // if(event == NONE){
            //     backlight_status = 0;
            //     state = WAIT;
            //     switch(dispEvent){
            //     case STRAIGHT:
            //         event = STRAIGHT;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "  Running ");
            //         strcpy(display_line[2], " Straight ");
            //         strcpy(display_line[3], "          ");
            //         straight_step = 1;
            //         break;
            //     case CIRCLE:
            //         event = CIRCLE;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "  Running ");
            //         strcpy(display_line[2], "  Circle  ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         dispEvent = NONE;
            //         circle_step = 1;
            //         circle_step2 = 1;
            //         break;
            //     case TRIANGLE:
            //         event = TRIANGLE;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "  Running ");
            //         strcpy(display_line[2], " Triangle ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         dispEvent = NONE;
            //         triangle_step = 1;
            //         break;
            //     case FIGURE8C1:
            //         event = FIGURE8C1;
            //         strcpy(display_line[0], "          ");
            //         strcpy(display_line[1], "  Running ");
            //         strcpy(display_line[2], " Figure 8 ");
            //         strcpy(display_line[3], "          ");
            //         display_changed = TRUE;
            //         dispEvent = NONE;
            //         figure8_step = 1;
            //         break;
            //     case NONE:
            //         motorStop();
            //         break;
            //     default:
            //         break;
            //     }
            // }
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


