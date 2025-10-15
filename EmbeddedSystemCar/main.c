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

// Global Variables
volatile char slow_input_down;
extern char display_line[4][11];
extern char *display[4];
unsigned char display_mode; //unused
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;
extern volatile unsigned int Time_Sequence;
extern volatile unsigned char one_time;
extern unsigned int IR;
extern unsigned int IRChange;
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
unsigned int test_value;
char chosen_direction;
char change;

// New Global Variables for Button Switch & Movement ----------------
unsigned int Switch1_Pressed;
unsigned int old_Time_Sequence;
unsigned int mytime;
unsigned int dir;

unsigned int right_motor_count;
unsigned int left_motor_count;

unsigned int backlight;             // backlight on off flag
unsigned int time_change;
unsigned char dispEvent;            // switch.c track display state
unsigned char state;
unsigned char event;

unsigned int travel_distance;
unsigned int right_count_time;
unsigned int left_count_time;
unsigned int wheel_count_time;

unsigned int delay_start;
unsigned int segment_count;
unsigned int cycle_time;
unsigned int secTime;

//------------------------------------------------------------------------------

void main(void){
    // WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    Init_Ports();                        // Initialize Ports
    Init_Clocks();                       // Initialize Clock System
    Init_Conditions();                   // Initialize Variables and Initial Conditions
    Init_Timers();                       // Initialize Timers
    Init_LCD();                          // Initialize LCD
    Init_ADC();                          // Initialize ADC

    // Startup Display
    strcpy(display_line[0], "   NCSU   ");
    strcpy(display_line[1], " WOLFPACK ");
    strcpy(display_line[2], "  ECE306  ");
    strcpy(display_line[3], "  GP I/O  ");
    display_changed = TRUE;


//------------------------------------------------------------------------------
// Begining of the "While" Operating System
//------------------------------------------------------------------------------
    backlight = OFF;
    IR = OFF;
    state = IDLE;
    motorStop();

    while(ALWAYS) {                      
        update();

        Project6();

    }
//------------------------------------------------------------------------------

}

void update(void){
    Display_Process();
    Carlson_StateMachine();
    backlight_update();
    IR_Update();
    P3OUT ^= TEST_PROBE;            // Change State of TEST_PROBE OFF
}


void Project6(void){
    switch (state){
        case IDLE:
            if(!IRChange){
                strcpy(display_line[0], "   IDLE   ");
            }

            PWM1_BOTH_OFF(); // motorsOFF();
            display_changed = TRUE;
            update_display = TRUE;
            //        ADC_Update = TRUE; = TRUE;
            // WAIT state called when SW1 is Pressed
            break;

        case WAIT:
            strcpy(display_line[0], "   WAIT   ");
            display_changed = TRUE;
            update_display = TRUE;
            //        ADC_Update = TRUE; = TRUE;
            switch (Time_Sequence)
            {   // Time_Sequence-State_Sequence
            case 10: // Wait for 1 Second
                PWM1_BOTH_OFF(); // motorsOFF();
                state = FWD;
                break;
            default:
                break;
            }
            break;

        case FWD:
            PWM1_BOTH_FWD(); // LRFwdON();
            strcpy(display_line[0], "   FWD    ");
            display_changed = TRUE;
            update_display = TRUE;
            //        ADC_Update = TRUE; = TRUE;
            if ((ADCLeft >= IR_MAGIC_NUM) && (ADCRight >= IR_MAGIC_NUM)){
                state = BLACKLINE; // Black Line Detected
                PWM1_BOTH_OFF(); // motorsOFF();
            }
            break;

        case BLACKLINE:
            PWM1_BOTH_OFF(); // motorsOFF();
            strcpy(display_line[0], " BLACKLINE");
            display_changed = TRUE;
            update_display = TRUE;
            //        ADC_Update = TRUE; = TRUE;
            Time_Sequence = 0; //        State_Sequence = Time_Sequence;
            state = WAIT2;
            break;

        case WAIT2:
            PWM1_BOTH_OFF(); // motorsOFF();
            if(Time_Sequence < 30){
                strcpy(display_line[0], "          ");
                strcpy(display_line[1], "Black Line");
                strcpy(display_line[2], " Detected ");
                strcpy(display_line[3], "          ");
            }
            display_changed = TRUE;
            update_display = TRUE;
            switch (Time_Sequence){ // Time_Sequence-State_Sequence
            case 30: // Wait for 3 Second
                strcpy(display_line[0], "          ");
                strcpy(display_line[1], "          ");
                strcpy(display_line[2], "          ");
                strcpy(display_line[3], "          ");
                display_changed = TRUE;
                update_display = TRUE;
                PWM1_BOTH_FWD();
                if ((ADCLeft < IR_MAGIC_NUM) && (ADCRight < IR_MAGIC_NUM)){
                    Time_Sequence = 0;
                    state = TURNL; // Black Line Detected
                    PWM1_RIGHT_OFF(); // motorsOFF();
                }
                break;
            default:
                break;
            }

            break;

        case TURNL:
            PWM1_LEFT_FWD(); // LeftFwdON();
            strcpy(display_line[0], " TURN LEFT");
            display_changed = TRUE;
            update_display = TRUE;
            //        ADC_Update = TRUE; = TRUE;
            if ((ADCLeft >= IR_MAGIC_NUM) && (ADCRight >= IR_MAGIC_NUM))
            {
                state = LINE1; // Black Line Detected
                PWM1_BOTH_OFF(); // motorsOFF();
                Time_Sequence = 0;
            }
            break;
        case LINE1:
            strcpy(display_line[0], "   LINE1  ");
            display_changed = TRUE;
            update_display = TRUE;
            //        ADC_Update = TRUE; = TRUE;
            if ((ADCLeft >= IR_MAGIC_NUM) && (ADCRight >= IR_MAGIC_NUM)
                    && (Time_Sequence >= 10))
            {
                state = DONE; // Black Line Detected
                Time_Sequence = 0; //  State_Sequence = Time_Sequence;
                PWM1_BOTH_OFF(); // motorsOFF();
            }
        case DONE:
            strcpy(display_line[0], "   DONE   ");
            display_changed = TRUE;
            update_display = TRUE;
            break;

        default:
            break;
    }
}


