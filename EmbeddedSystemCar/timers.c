//------------------------------------------------------------------------------
//
//  Description: This file contains the Timer_B0 Initialization and ISR
//
//  Ohm Patel
//  Oct 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "timers.h"
#include  "ports.h"
#include  "LCD.h"
#include  "macros.h"
#include  "motors.h"

//------------------------------------------------------------------------------
// Public globals consumed across the project
//------------------------------------------------------------------------------

extern volatile unsigned char update_display;        // Display service flag
extern volatile unsigned int  update_display_count;  // optional counter

// These are defined here (timersB0.c owns them)
volatile unsigned char one_time;              // Legacy (not required in HW06)
extern volatile unsigned int  timer200ms;         // Legacy (not required in HW06)

// Switch debouncing counters (not strictly used by current switch.c but provided)
volatile unsigned int debounce_count1 = 0;  // counts debounce ticks for SW1
volatile unsigned int debounce_count2 = 0;  // counts debounce ticks for SW2
volatile unsigned char debounce_active1 = 0; // 1 when SW1 debounce in progress
volatile unsigned char debounce_active2 = 0; // 1 when SW2 debounce in progress

//------------------------------------------------------------------------------
// Local configuration
//------------------------------------------------------------------------------

// Clocking
#define TB0_SMCLK_HZ            (8000000u)
#define TB0_ID_DIV              (8u)     // ID__8
#define TB0_EX_DIV              (8u)     // TBIDEX_7
#define TB0_TICK_HZ             (TB0_SMCLK_HZ / TB0_ID_DIV / TB0_EX_DIV) // 125kHz

// CCR0: 200ms backlight blink + display update
#define BACKLIGHT_INTERVAL_MS   (200u)
#define CCR0_DELTA_COUNTS       ((TB0_TICK_HZ * BACKLIGHT_INTERVAL_MS) / 1000u) // 25,000

// CCR1/CCR2: debounce ticker
#define DEBOUNCE_TICK_MS        (50u)    // debounce tick every 50ms
#define CCRx_DEBOUNCE_DELTA     ((TB0_TICK_HZ * DEBOUNCE_TICK_MS) / 1000u)      // 6,250

// Debounce duration: 800ms - 1200ms suggested; choose 1000ms => 20 ticks of 50ms
#define DEBOUNCE_THRESHOLD_TICKS (20u)

//------------------------------------------------------------------------------
// Init wrappers
//------------------------------------------------------------------------------

void Init_Timers(void) {
  Init_Timer_B0();
  Init_Timer_B1();
  Init_Timer_B2();
  Init_Timer_B3();
}

void Init_Timer_B0(void) {
    TB0CTL = TBSSEL__SMCLK;     // SMCLK source
    TB0CTL |= TBCLR;            // Resets TB0R, clock divider, count direction
    TB0CTL |= MC__CONTINOUS;    // Continuous up
    TB0CTL |= ID__8;            // Divide clock by 2
    TB0EX0 = TBIDEX__8;         // Divide clock by an additional 8

    TB0CCR0 = TB0CCR0_INTERVAL; // CCR0
    TB0CCTL0 |= CCIE;           // CCR0 enable interrupt

    // TB0CCR1 = TB0CCR1_INTERVAL; // CCR1
    // TB0CCTL1 |= CCIE; // CCR1 enable interrupt

    // TB0CCR2 = TB0CCR2_INTERVAL; // CCR2
    // TB0CCTL2 |= CCIE; // CCR2 enable interrupt

    TB0CTL &= ~TBIE;  // Disable Overflow Interrupt
    TB0CTL &= ~TBIFG; // Clear Overflow Interrupt flag
}

void Init_Timer_B1(void) {
  // Not used in this lab configuration
}

void Init_Timer_B2(void) {
  // Not used in this lab configuration
}

void Init_Timer_B3(void) {
    //-----------------------------------------------------------------------------
    // SMCLK source, up count mode, PWM for motors
    // TB3.1 P6.0 L_FORWARD
    // TB3.2 P6.1 R_FORWARD
    // TB3.3 P6.2 L_REVERSE
    // TB3.4 P6.3 R_REVERSE
    // TB3.5 P6.4 LCD_BACKLITE
    //-----------------------------------------------------------------------------
    TB3CTL = TBSSEL__SMCLK; // SMCLK
    TB3CTL |= MC__UP;       // Up Mode
    TB3CTL |= TBCLR;        // Clear TAR

    PWM_PERIOD = PWM1_WHEEL_PERIOD;         // PWM Period [Set this to 50005]

    TB3CCTL1 = OUTMOD_7;               // CCR1 reset/set
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;    // P6.0 Left Forward PWM duty cycle

    TB3CCTL2 = OUTMOD_7;               // CCR2 reset/set
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;   // P6.1 Right Forward PWM duty cycle

    TB3CCTL3 = OUTMOD_7;               // CCR3 reset/set
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;    // P6.2 Left Reverse PWM duty cycle

    TB3CCTL4 = OUTMOD_7;               // CCR4 reset/set
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;   // P6.3 Right Reverse PWM duty cycle

    TB3CCTL5 = OUTMOD_7;               // CCR5 reset/set
    LCD_BACKLITE_DIMING = PWM1_PERCENT_80;  // P6.4 LCD Backlight PWM duty cycle
    //-----------------------------------------------------------------------------
}

//------------------------------------------------------------------------------
// Sleep helpers (rough, CPU busy-wait based on 8MHz MCLK)
//------------------------------------------------------------------------------

void five_msec_sleep(unsigned int msec5) {
  while (msec5--) { __delay_cycles(40000); }
}

void sleep(unsigned int msec) {
  while (msec--) { __delay_cycles(8000); }
}
