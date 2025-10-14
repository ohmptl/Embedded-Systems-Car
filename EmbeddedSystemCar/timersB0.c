//------------------------------------------------------------------------------
//
//  Description: Timer_B0 timebase and timer helper functions
//
//  Ohm Patel
//  Oct 2025
//  Built with Code Composer Version: CCS20.3.0
//------------------------------------------------------------------------------

// Includes
#include "LCD.h"
#include "functions.h"
#include "macros.h"
#include "msp430.h"
#include "ports.h"
#include <string.h>

#include "timersB0.h"


// Globals
volatile unsigned int Time_Sequence;
volatile char one_time;
unsigned int counter_B0;
unsigned int delay_time;
extern unsigned int backlight;
extern int Switch_Counter1;
extern volatile unsigned char update_display;

extern int activateSM;
extern char display_line[4][11];
extern volatile unsigned char display_changed;

// Debounce Vars
extern char debounce_Status_SW1;
extern char debounce_Status_SW2;
extern unsigned int count_debounce_SW1;
extern unsigned int count_debounce_SW2;

void Init_Timers(void) {
    Init_Timer_B0();
}

// ---ORIGINAL-------------------------------------------------------------------
// Timer B0 initialization sets up both B0_0, B0_1-B0_2 and overflow
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
//----------------------------------------------------------------------------

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){
    //-----------------------------------------------------------------------------
    // TimerB0 0 Interrupt handler
    //---------------------------------------------------------------------------
    update_display = TRUE;
    Time_Sequence++;
    count_debounce_SW1++;
    count_debounce_SW2++;
    TB0CCR0 += TB0CCR0_INTERVAL;
    if(debounce_Status_SW1==OFF && debounce_Status_SW2==OFF){
        if(backlight){
            backlight = 0;
        }
        else {
            backlight = 1;
        }
    }
    // Add Offset to TBCCR0
    //---------------------------------------------------------------------------
}




