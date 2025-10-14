//------------------------------------------------------------------------------
//
//  Description: Timer_B0 timebase and timer helper functions
//
//  Provides a 5ms system tick using TB0 CCR0 (continuous mode + offset),
//  updates Time_Sequence and one_time flags used by the state machine,
//  and exposes basic sleep helpers. Replaces precompiled timersB0.obj.
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS12.x
//------------------------------------------------------------------------------

#include "msp430.h"
#include "functions.h"
#include "macros.h"

//------------------------------------------------------------------------------
// Public globals consumed across the project
//------------------------------------------------------------------------------

// These are defined in LCD.obj (precompiled), so we declare them extern
extern volatile unsigned char update_display;        // Display service flag
extern volatile unsigned int  update_display_count;  // 5ms tick counter for display

// These are defined here (timersB0.c owns them)
volatile unsigned char one_time = 0;              // Pulsed at key time marks
volatile unsigned int  Time_Sequence = 0;         // Increments every 5ms (0..250)

// Switch debouncing counters (not strictly used by current switch.c but provided)
volatile unsigned int debounce_count1 = 0;
volatile unsigned int debounce_count2 = 0;

//------------------------------------------------------------------------------
// Local configuration
//------------------------------------------------------------------------------

#define TB0_TICK_HZ_SMCLK   8000000u
#define TB0_DIVIDER         8u                    // ID__8 -> 1 MHz timer clock
#define TB0_TICK_US         5000u                 // 5 ms tick
#define TB0_COUNTS_PER_TICK (TB0_TICK_HZ_SMCLK / TB0_DIVIDER / (1000000u / TB0_TICK_US))

// Sanity check for counts per tick (at 8MHz /8 and 5ms => 5000 counts)
#if TB0_COUNTS_PER_TICK != 5000
#error "Unexpected TB0_COUNTS_PER_TICK; check clock or dividers"
#endif

//------------------------------------------------------------------------------
// Init wrappers
//------------------------------------------------------------------------------

void Init_Timers(void) {
  Init_Timer_B0();
  // Stubs for other timers (not used by this project configuration)
  Init_Timer_B1();
  Init_Timer_B2();
  Init_Timer_B3();
}

void Init_Timer_B0(void) {
  // Stop and configure Timer_B0 for continuous mode sourced from SMCLK/8
  TB0CTL = TBSSEL__SMCLK | MC__STOP | TBCLR;   // SMCLK, stop, clear
  TB0EX0 = TBIDEX_0;                           // No extra divide beyond ID__8
  TB0CTL |= ID__8;                              // Divide SMCLK by 8 (1MHz)

  // CCR0 will generate periodic interrupts every 5ms by adding offset
  TB0CCTL0 = CCIE;                              // Enable CCR0 interrupt
  TB0CCR0  = TB0R + TB0_COUNTS_PER_TICK;        // First event in 5ms

  // Optional: clear/disable other CCR interrupts for safety
  TB0CCTL1 = 0;
  TB0CCTL2 = 0;

  // Start timer in continuous mode
  TB0CTL = (TB0CTL & ~(MC_3)) | MC__CONTINUOUS; // Continuous mode
}

void Init_Timer_B1(void) {
  // Not used in this lab configuration; left as placeholder to satisfy linker
}

void Init_Timer_B2(void) {
  // Not used in this lab configuration; left as placeholder to satisfy linker
}

void Init_Timer_B3(void) {
  // Not used in this lab configuration; left as placeholder to satisfy linker
}

//------------------------------------------------------------------------------
// Sleep helpers (rough, CPU busy-wait based on 8MHz MCLK)
//------------------------------------------------------------------------------

void usleep(unsigned int usec) {
  // 8 cycles per microsecond at 8MHz
  while (usec--) {
    __delay_cycles(8);
  }
}

void usleep10(unsigned int usec10) {
  // units of 10 us
  while (usec10--) {
    __delay_cycles(80);
  }
}

void five_msec_sleep(unsigned int msec5) {
  // units of 5 ms
  while (msec5--) {
    __delay_cycles(40000); // 8MHz * 0.005s
  }
}

void measure_delay(void) {
  // Placeholder; no-op for now
}

void out_control_words(void) {
  // Placeholder; no-op for now
}

//------------------------------------------------------------------------------
// Timer_B0 Interrupts
//------------------------------------------------------------------------------

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void) {
  // Schedule next 5ms tick
  TB0CCR0 += TB0_COUNTS_PER_TICK;

  // Housekeeping counters
  update_display_count++;
  if (update_display_count >= 5) {    // ~25ms display service
    update_display_count = 0;
    update_display = TRUE;
  }

  // Debounce counters (free-running up to a cap)
  if (debounce_count1 < 0xFFFF) debounce_count1++;
  if (debounce_count2 < 0xFFFF) debounce_count2++;

  // 5ms system time sequencing
  Time_Sequence++;
  if (Time_Sequence >= 250) {         // 250 * 5ms = 1.25s wrap window
    Time_Sequence = 0;
  }
  // Pulse one_time when entering 50,100,150,200,250 steps used by state machine
  if ((Time_Sequence % 50) == 0) {
    one_time = TRUE;
  }
}

// Timer0_B1 ISR handles CCR1/CCR2 and overflow if ever enabled; clear flags
#pragma vector = TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void) {
  switch (__even_in_range(TB0IV, TB0IV_TBIFG)) {
    case TB0IV_NONE:   break; // No interrupt
    case TB0IV_TBCCR1: TB0CCTL1 &= ~CCIFG; break;
    case TB0IV_TBCCR2: TB0CCTL2 &= ~CCIFG; break;
    case TB0IV_TBIFG:  TB0CTL   &= ~TBIFG; break; // Overflow
    default: break;
  }
}
