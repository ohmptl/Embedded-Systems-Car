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

//------------------------------------------------------------------------------
// HW08: Serial and display helpers
//------------------------------------------------------------------------------
static const char STR_NCSU[]    = "NCSU  #1";   // two spaces between U and #
static const char STR_460800[]  = "460,800";
static const char STR_115200[]  = "115,200";

static unsigned char hw8_inited = 0;
static unsigned char splash_done = 0;
static unsigned int  splash_start_ticks = 0;

static unsigned char current_baud = 2;  // 1=115200, 2=460800 per serial.c
static unsigned char pending_tx = 0;
static unsigned int  tx_start_ticks = 0;

static inline const char* baud_str(unsigned char mode) {
    return (mode == 1) ? STR_115200 : STR_460800;
}

static void hw8_show_baud_screen(void) {
    // Line 3: "Baud" centered; Line 4: current baud string centered
    dispPrint("Baud", 3);
    dispPrint((char*)baud_str(current_baud), 4);
}

void main(void){
    // WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    Init_Ports();                        // Initialize Ports
    Init_Clocks();                       // Initialize Clock System
    Init_Conditions();                   // Initialize Variables and Initial Conditions
    Init_Timers();                       // Initialize Timers
    Init_LCD();                          // Initialize LCD
    Init_ADC();                          // Initialize ADC

    // Startup Display (Splash)
    strcpy(display_line[0], "   NCSU   ");
    strcpy(display_line[1], " WOLFPACK ");
    strcpy(display_line[2], "  ECE306  ");
    strcpy(display_line[3], "  GP I/O  ");
    display_changed = TRUE;

    // Initialize serial ports at 460,800 baud (mode 2)
    Init_Serial_UCA0(2);
    Init_Serial_UCA1(2);
    current_baud = 2;


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
    Display_Process();

    // HW08 lifecycle
    if (!hw8_inited) {
        splash_start_ticks = Time_Sequence;
        hw8_inited = 1;
    }

    // After 5 seconds of splash, show baud screen
    if (!splash_done) {
        if ((unsigned int)(Time_Sequence - splash_start_ticks) >= 25) { // 25 * 200ms = 5s
            // Clear lines 1-2; show baud on 3-4
            dispPrint("", 1);
            dispPrint("", 2);
            hw8_show_baud_screen();
            splash_done = 1;
        }
    }

    // Handle SW1/SW2 events from ISR to switch baud
    if (sw1_press_event) {
        sw1_press_event = 0;
        // Set 115,200 on both ports
        Init_Serial_UCA0(1);
        Init_Serial_UCA1(1);
        current_baud = 1;
        // Clear line 1 and 2; update baud on line 4
        dispPrint("", 1);
        dispPrint("", 2);
        hw8_show_baud_screen();
        // Arm a 2-second delay then send string on UCA1
        tx_start_ticks = Time_Sequence;
        pending_tx = 1;
    }
    if (sw2_press_event) {
        sw2_press_event = 0;
        // Set 460,800 on both ports
        Init_Serial_UCA0(2);
        Init_Serial_UCA1(2);
        current_baud = 2;
        // Clear line 1 and 2; update baud on line 4
        dispPrint("", 1);
        dispPrint("", 2);
        hw8_show_baud_screen();
        // Arm a 2-second delay then send string on UCA1
        tx_start_ticks = Time_Sequence;
        pending_tx = 1;
    }

    // Handle delayed TX after baud change
    if (pending_tx) {
        if ((unsigned int)(Time_Sequence - tx_start_ticks) >= 10) { // 10 * 200ms = 2s
            // Send test string and a newline so RX side knows to display the line
            UCA1_SendString(STR_NCSU);
            UCA1_SendString("\r");
            pending_tx = 0;
        }
    }

    // Drain RX ring and display received data on line 1
    Serial_Process_USB_RX();
    // Also show UCA0 (FRAM/IOT) loopback on line 2 for Configuration 1
    Serial_Process_IOT_RX();

    // Existing state machine and modules
    Carlson_StateMachine();
    backlight_update();
    IR_Update();
    P3OUT ^= TEST_PROBE;            // Change State of TEST_PROBE OFF
}