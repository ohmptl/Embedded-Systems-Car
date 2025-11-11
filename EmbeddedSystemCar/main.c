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
extern volatile unsigned int ADCThumb;
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
unsigned char left_black_high = TRUE;
unsigned char right_black_high = TRUE;
unsigned int intercept_thresh_left;
unsigned int intercept_thresh_right;
unsigned int intercept_margin_left;
unsigned int intercept_margin_right;
unsigned int intercept_entry_left;
unsigned int intercept_entry_right;
unsigned char intercept_entry_valid = FALSE;

static unsigned int turning_line_ticks = 0;
static unsigned int turning_prev_sequence = 0;

#define INTERCEPT_PHASE_SEEK         (0u)
#define INTERCEPT_PHASE_PAUSE        (1u)
#define INTERCEPT_PHASE_REVERSE      (2u)
#define INTERCEPT_PHASE_PAUSE_AFTER  (3u)
#define INTERCEPT_PHASE_COMPLETE     (4u)

static unsigned char intercept_phase = INTERCEPT_PHASE_SEEK;
static unsigned int intercept_phase_ticks = 0;
static unsigned int intercept_prev_sequence = 0;

// Lap timing (reference-style): exactly two laps based on time
volatile unsigned int lap_ticks_target;  // ticks per lap, mapped from thumbwheel
volatile unsigned int lap_ticks_accum;   // accumulated ticks in current lap
unsigned int laps_completed;             // number of laps completed
unsigned char follow_dir;                       // 'L' for CCW, 'R' for CW (like reference)

void main(void){
    // WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    Init_Ports();                        // Initialize Ports
    Init_Clocks();                       // Initialize Clock System
    Init_Conditions();                   // Initialize Variables and Initial Conditions
    Init_Timers();                       // Initialize Timers
    Init_LCD();                          // Initialize LCD
    // Init_ADC();                          // Initialize ADC

    Serial_Project9_Init();


//------------------------------------------------------------------------------
// Begining of the "While" Operating System
//------------------------------------------------------------------------------
    backlight = OFF;
    IR = OFF;
    state = IDLE;
    motorStop();

    while(ALWAYS) {                      
        update();


    }
//------------------------------------------------------------------------------

}

void update(void){
    Serial_Project9_Service();

    Display_Process();

    if (sw1_press_event) {
        sw1_press_event = 0;
        Serial_RequestWifiStatus();
    }

    if (sw2_press_event) {
        sw2_press_event = 0;
        Serial_RequestIpAddress();
    }

    StateMachine();
    backlight_update();
    IR_Update();
    P3OUT ^= TEST_PROBE;            // Change State of TEST_PROBE OFF
}