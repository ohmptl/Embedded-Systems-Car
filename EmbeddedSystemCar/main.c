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
#include  "functions.h"
#include  "LCD.h"
#include  "macros.h"
#include  "ports.h"
// -----------------------------------------------------

// Function Prototypes
void main(void);
void Init_Conditions(void);
void Display_Process(void);
void Init_LEDs(void);
void Carlson_StateMachine(void);

// -----------------------------------------------------

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
unsigned int test_value;
char chosen_direction;
char change;

// New Global Variables for Button Switch & Movement ----------------

unsigned int old_Time_Sequence;
unsigned int mytime;

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

unsigned int straight_step;
unsigned int circle_step;
unsigned int circle_step2;
unsigned int triangle_step;
unsigned int figure8_step;

//------------------------------------------------------------------------------

void main(void){
  // WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;

  Init_Ports();                        // Initialize Ports
  Init_Clocks();                       // Initialize Clock System
  Init_Conditions();                   // Initialize Variables and Initial Conditions
  Init_Timers();                       // Initialize Timers
  Init_LCD();                          // Initialize LCD
  // P2OUT &= ~RESET_LCD;

  strcpy(display_line[0], "   NCSU   ");
  strcpy(display_line[1], " WOLFPACK ");
  strcpy(display_line[2], "  ECE306  ");
  strcpy(display_line[3], "  GP I/O  ");
  display_changed = TRUE;


//------------------------------------------------------------------------------
// Begining of the "While" Operating System
//------------------------------------------------------------------------------
    backlight = OFF;
    motorStop();
    // dispEvent = NONE;
    // state = WAIT;
    // event = NONE;

    while(ALWAYS) {                      
        Carlson_StateMachine();         // Run a Time Based State Machine
        Display_Process();              // Update Display based on display_changed
        backlight_update();             // Turn ON Backlight
        P3OUT ^= TEST_PROBE;            // Change State of TEST_PROBE OFF
        if(Time_Sequence != old_Time_Sequence){
            mytime++;
            old_Time_Sequence = Time_Sequence;
            //wheel_period++;
            time_change = 1;
        }

        Switch1_Process();
        Switch2_Process();
        

    }
//------------------------------------------------------------------------------

}








