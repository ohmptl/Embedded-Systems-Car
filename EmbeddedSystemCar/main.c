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

// Project 7 calibration and timing ----------------
volatile unsigned int time_ticks_200ms;         // 0.2s increments
volatile unsigned char timer_running;           // 1: counting; 0: stopped
volatile unsigned char sw1_press_event;         // SW1 press detected
volatile unsigned char sw2_press_event;         // SW2 press detected (for calibration confirmation)

unsigned int adc_white_left;
unsigned int adc_white_right;
unsigned int adc_black_left;
unsigned int adc_black_right;

unsigned int thresh_black_left;
unsigned int thresh_black_right;

unsigned int lap_count;
unsigned int last_lap_tick;
unsigned char lap_detected_flag;                // debounce for lap detection
unsigned char last_line_position;               // tracks last known line position (LINE_LEFT, LINE_RIGHT, LINE_CENTER)

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

        Project7();

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

void Project7(void){
    // Timer display: update line 4 every 200ms (format: "0000.0s")
    if (update_display) {
        unsigned int ticks = time_ticks_200ms;
        unsigned int whole_sec = ticks / 5;  // 5 ticks = 1s
        unsigned int tenth = (ticks % 5) * 2; // convert 0-4 to 0,2,4,6,8
        display_line[3][0] = ' ';
        display_line[3][1] = (whole_sec / 100) % 10 + '0';
        display_line[3][2] = (whole_sec / 10) % 10 + '0';
        display_line[3][3] = whole_sec % 10 + '0';
        display_line[3][4] = '.';
        display_line[3][5] = tenth + '0';
        display_line[3][6] = 's';
        display_line[3][7] = ' ';
        display_line[3][8] = ' ';
        display_line[3][9] = ' ';
    }

    switch (state) {
    case IDLE:
        strcpy(display_line[0], "   IDLE   ");
        strcpy(display_line[1], " SW1:Start");
        strcpy(display_line[2], " SW2: IR  ");
        PWM1_BOTH_OFF();
        display_changed = TRUE;
        timer_running = 0;
        if (sw1_press_event) {
            sw1_press_event = 0;
            state = CAL_WHITE;
            Time_Sequence = 0;
            IR = ON;  // Turn IR on for calibration
        }
        break;

    case CAL_WHITE:
        strcpy(display_line[0], " CAL WHT  ");
        strcpy(display_line[1], " SW2:Next ");
        // ADC values displayed on lines 2&3 by ADC ISR
        PWM1_BOTH_OFF();
        IR = ON;
        display_changed = TRUE;
        
        // Wait for SW2 press to confirm white calibration
        if (sw2_press_event) {
            sw2_press_event = 0;
            adc_white_left = ADCLeft;
            adc_white_right = ADCRight;
            state = CAL_BLACK;
            Time_Sequence = 0;
        }
        break;

    case CAL_BLACK:
        strcpy(display_line[0], " CAL BLK  ");
        strcpy(display_line[1], " SW2:Next ");
        // ADC values displayed on lines 2&3 by ADC ISR
        IR = ON;
        PWM1_BOTH_OFF();
        display_changed = TRUE;
        
        // Wait for SW2 press to confirm black calibration
        if (sw2_press_event) {
            sw2_press_event = 0;
            adc_black_left = ADCLeft;
            adc_black_right = ADCRight;
            // Compute thresholds: midpoint between white and black
            thresh_black_left = (adc_black_left + adc_white_left) / 2;
            thresh_black_right = (adc_black_right + adc_white_right) / 2;
            state = WAIT;
            Time_Sequence = 0;
        }
        break;

    case WAIT:
        strcpy(display_line[0], "   WAIT   ");
        strcpy(display_line[1], "Ready 3s..");
        PWM1_BOTH_OFF();
        IR = ON;
        display_changed = TRUE;
        
        // Wait 3 seconds before starting
        if (Time_Sequence >= 15) {  // 15 * 0.2s = 3 seconds
            state = INTERCEPT;
            Time_Sequence = 0;
            time_ticks_200ms = 0;
            timer_running = 1;
            lap_count = 0;
            last_lap_tick = 0;
            lap_detected_flag = 0;
        }
        break;

    case INTERCEPT:
        strcpy(display_line[0], "INTERCEPT ");
        strcpy(display_line[1], "          ");
        // Display ADC on line 2 & 3 (handled by ADC ISR)
        IR = ON;
        set_motor_speeds(BASE_SPEED_PWM, BASE_SPEED_PWM);
        display_changed = TRUE;
        // Detect black line (both sensors below threshold)
        if (ADCLeft < thresh_black_left && ADCRight < thresh_black_right) {
            state = TURNING;
            Time_Sequence = 0;
            PWM1_BOTH_OFF();
        }
        break;

    case TURNING:
        strcpy(display_line[0], " TURNING  ");
        strcpy(display_line[1], "          ");
        IR = ON;
        display_changed = TRUE;
        // Pivot left until both sensors see black line again
        pivot_left_pwm(TURN_SPEED_PWM);
        if ((ADCLeft < thresh_black_left) && (ADCRight < thresh_black_right)) {
            state = CIRCLING;
            Time_Sequence = 0;
            lap_count = 0;
            last_lap_tick = 0;
            lap_detected_flag = 0;
            last_line_position = LINE_RIGHT;  // For clockwise: line should be on RIGHT
        }
        break;

    case CIRCLING: {
        strcpy(display_line[0], " CIRCLING ");
        // Display lap count on line 1
        display_line[1][0] = ' ';
        display_line[1][1] = 'L';
        display_line[1][2] = 'a';
        display_line[1][3] = 'p';
        display_line[1][4] = ':';
        display_line[1][5] = ' ';
        display_line[1][6] = lap_count + '0';
        display_line[1][7] = '/';
        display_line[1][8] = '2';
        display_line[1][9] = ' ';
        IR = ON;
        display_changed = TRUE;

        // Line following logic for CLOCKWISE circle
        // Goal: Keep black line under RIGHT sensor (outside of circle)
        // LEFT sensor should mostly see white (inside of circle)
        
        unsigned int left_pwm = BASE_SPEED_PWM;
        unsigned int right_pwm = BASE_SPEED_PWM;
        
        unsigned char left_on_black = (ADCLeft < thresh_black_left);
        unsigned char right_on_black = (ADCRight < thresh_black_right);
        
        // Strategy: RIGHT sensor tracks the line
        // NO BIAS - pure feedback control based on sensor readings
        
        if (left_on_black && right_on_black) {
            // Both on black: could be lap marker or crossing wide part of line
            // Go straight at base speed - NO BIAS
            left_pwm = BASE_SPEED_PWM;
            right_pwm = BASE_SPEED_PWM;
            last_line_position = LINE_BOTH;
            
        } else if (!left_on_black && right_on_black) {
            // IDEAL: Left on white, right on black
            // Perfect tracking! Go straight at base speed - NO BIAS
            left_pwm = BASE_SPEED_PWM;
            right_pwm = BASE_SPEED_PWM;
            last_line_position = LINE_RIGHT;
            
        } else if (left_on_black && !right_on_black) {
            // Left on black, right on white: we've drifted too far LEFT onto the line
            // Need to turn RIGHT aggressively (slow down right motor)
            left_pwm = BASE_SPEED_PWM + MINOR_CORRECTION_PWM;
            right_pwm = BASE_SPEED_PWM - MINOR_CORRECTION_PWM;
            last_line_position = LINE_LEFT;
            
        } else {
            // Both on white: line is COMPLETELY LOST!
            // Enter recovery mode - stop and pivot to find line
            state = RECOVERY;
            Time_Sequence = 0;
            PWM1_BOTH_OFF();
        }
        
        // Cap PWM to safe limits
        if (left_pwm > PWM_MAX) left_pwm = PWM_MAX;
        if (right_pwm > PWM_MAX) right_pwm = PWM_MAX;
        if (left_pwm < PWM_MIN) left_pwm = PWM_MIN;
        if (right_pwm < PWM_MIN) right_pwm = PWM_MIN;
        
        set_motor_speeds(left_pwm, right_pwm);

        // Lap detection: both sensors see black AND enough time has passed
        if (left_on_black && right_on_black && !lap_detected_flag) {
            unsigned int delta_ticks = time_ticks_200ms - last_lap_tick;
            if (delta_ticks >= MIN_LAP_TICKS) {
                lap_count++;
                last_lap_tick = time_ticks_200ms;
                lap_detected_flag = 1;
                if (lap_count >= 2) {
                    state = EXIT_CENTER;
                    Time_Sequence = 0;
                    PWM1_BOTH_OFF();
                }
            }
        }
        // Reset lap detection flag after clearing the lap marker
        if (!left_on_black && !right_on_black) {
            // Both sensors back on white - debounce period
            if (lap_detected_flag) {
                static unsigned int lap_clear_time = 0;
                if (lap_clear_time == 0) {
                    lap_clear_time = time_ticks_200ms;
                } else if ((time_ticks_200ms - lap_clear_time) >= LAP_DEBOUNCE_TICKS) {
                    lap_detected_flag = 0;
                    lap_clear_time = 0;
                }
            }
        }
        
        break;
    }

    case RECOVERY:
        strcpy(display_line[0], " RECOVERY ");
        strcpy(display_line[1], " FIND LINE");
        IR = ON;
        display_changed = TRUE;
        
        if (Time_Sequence < RECOVERY_STOP_TICKS) {
            // Brief stop to stabilize
            PWM1_BOTH_OFF();
        } else if (Time_Sequence < (RECOVERY_STOP_TICKS + RECOVERY_PIVOT_TICKS)) {
            // Pivot to find the line based on where we last saw it
            if (last_line_position == LINE_LEFT) {
                // Line was on left, pivot RIGHT to find it
                pivot_right_pwm(RECOVERY_TURN_PWM);
            } else {
                // Line was on right (most common), pivot LEFT to find it
                pivot_left_pwm(RECOVERY_TURN_PWM);
            }
            
            // Check if we found the line during pivot
            unsigned char left_on_black = (ADCLeft < thresh_black_left);
            unsigned char right_on_black = (ADCRight < thresh_black_right);
            if (left_on_black || right_on_black) {
                // Found the line! Return to circling
                state = CIRCLING;
                Time_Sequence = 0;
            }
        } else {
            // Recovery timeout - return to circling and try again
            state = CIRCLING;
            Time_Sequence = 0;
        }
        break;

    case EXIT_CENTER:
        strcpy(display_line[0], "EXIT CTR  ");
        strcpy(display_line[1], "          ");
        IR = ON;
        display_changed = TRUE;
        if (Time_Sequence < EXIT_PIVOT_TICKS) {
            pivot_left_pwm(TURN_SPEED_PWM);
        } else if (Time_Sequence < (EXIT_PIVOT_TICKS + EXIT_DRIVE_TICKS)) {
            set_motor_speeds(BASE_SPEED_PWM, BASE_SPEED_PWM);
        } else {
            state = STOPPED;
            PWM1_BOTH_OFF();
        }
        break;

    case STOPPED:
        strcpy(display_line[0], " STOPPED  ");
        strcpy(display_line[1], "          ");
        PWM1_BOTH_OFF();
        IR = OFF;
        timer_running = 0;
        display_changed = TRUE;
        break;

    default:
        break;
    }
}


