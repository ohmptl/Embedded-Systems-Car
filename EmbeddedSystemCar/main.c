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

unsigned int adc_ambient_left;
unsigned int adc_ambient_right;
unsigned int adc_white_left;
unsigned int adc_white_right;
unsigned int adc_black_left;
unsigned int adc_black_right;

unsigned int thresh_black_left;
unsigned int thresh_black_right;
unsigned int thresh_white_left;
unsigned int thresh_white_right;

unsigned int lap_count;
unsigned int last_lap_tick;

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
        strcpy(display_line[1], " SW1: Cal ");
        strcpy(display_line[2], " SW2: IR  ");
        PWM1_BOTH_OFF();
        display_changed = TRUE;
        timer_running = 0;
        if (sw1_press_event) {
            sw1_press_event = 0;
            state = CAL_AMBIENT;
            Time_Sequence = 0;
            IR = OFF;
        }
        break;

    case CAL_AMBIENT:
        strcpy(display_line[0], " CAL AMB  ");
        strcpy(display_line[1], "IR OFF..  ");
        PWM1_BOTH_OFF();
        IR = OFF;
        display_changed = TRUE;
        if (Time_Sequence >= 10) { // 2s to sample
            adc_ambient_left = ADCLeft;
            adc_ambient_right = ADCRight;
            state = CAL_WHITE;
            Time_Sequence = 0;
        }
        break;

    case CAL_WHITE:
        strcpy(display_line[0], " CAL WHT  ");
        strcpy(display_line[1], "IR ON..   ");
        IR = ON;
        PWM1_BOTH_OFF();
        display_changed = TRUE;
        if (Time_Sequence >= 10) { // 2s on white
            adc_white_left = ADCLeft;
            adc_white_right = ADCRight;
            state = CAL_BLACK;
            Time_Sequence = 0;
        }
        break;

    case CAL_BLACK:
        strcpy(display_line[0], " CAL BLK  ");
        strcpy(display_line[1], "IR ON..   ");
        IR = ON;
        PWM1_BOTH_OFF();
        display_changed = TRUE;
        if (Time_Sequence >= 10) { // 2s on black
            adc_black_left = ADCLeft;
            adc_black_right = ADCRight;
            // Compute thresholds: midpoint
            thresh_black_left = (adc_black_left + adc_white_left) / 2;
            thresh_black_right = (adc_black_right + adc_white_right) / 2;
            state = INTERCEPT;
            Time_Sequence = 0;
            time_ticks_200ms = 0;
            timer_running = 1;
            lap_count = 0;
            last_lap_tick = 0;
        }
        break;

    case INTERCEPT:
        strcpy(display_line[0], "INTERCEPT ");
        strcpy(display_line[1], "          ");
        // Display ADC on line 2 (handled by ADC ISR when not in calibration)
        IR = ON;
        set_motor_speeds(BASE_SPEED_PWM, BASE_SPEED_PWM);
        display_changed = TRUE;
        // Detect black line (both sensors)
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
        // Pivot left until black line detected again
        pivot_left_pwm(TURN_SPEED_PWM);
        if ((ADCLeft < thresh_black_left) && (ADCRight < thresh_black_right)) {
            state = CIRCLING;
            Time_Sequence = 0;
            lap_count = 0;
            last_lap_tick = 0;
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
        display_line[1][7] = ' ';
        display_line[1][8] = ' ';
        display_line[1][9] = ' ';
        IR = ON;
        display_changed = TRUE;

        // Simple steering: if left sees black -> steer right; if right sees black -> steer left
        unsigned int left_pwm = BASE_SPEED_PWM;
        unsigned int right_pwm = BASE_SPEED_PWM;
        if (ADCLeft < thresh_black_left) {
            // left on black => steer right: left faster, right slower
            left_pwm += STEER_DELTA_PWM;
            right_pwm -= STEER_DELTA_PWM;
        } else if (ADCRight < thresh_black_right) {
            // right on black => steer left: right faster, left slower
            right_pwm += STEER_DELTA_PWM;
            left_pwm -= STEER_DELTA_PWM;
        }
        // Cap limits
        if (left_pwm > PWM_MAX) left_pwm = PWM_MAX;
        if (right_pwm > PWM_MAX) right_pwm = PWM_MAX;
        if (left_pwm < PWM_MIN) left_pwm = PWM_MIN;
        if (right_pwm < PWM_MIN) right_pwm = PWM_MIN;
        set_motor_speeds(left_pwm, right_pwm);

        // Detect both sensors black => lap marker
        if ((ADCLeft < thresh_black_left) && (ADCRight < thresh_black_right)) {
            unsigned int delta_ticks = time_ticks_200ms - last_lap_tick;
            if (delta_ticks >= MIN_LAP_TICKS) {
                lap_count++;
                last_lap_tick = time_ticks_200ms;
                if (lap_count >= 2) {
                    state = EXIT_CENTER;
                    Time_Sequence = 0;
                    PWM1_BOTH_OFF();
                }
            }
        }
        break;
    }

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


