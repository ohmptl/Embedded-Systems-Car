//------------------------------------------------------------------------------
//
//  Description: Timer_B0 timebase and timer helper functions
//
//  Implements required timers for HW06 using TB0 in continuous mode:
//   - CCR0: Backlight blink and display service every 200ms (2.5Hz blink)
//   - CCR1: Debounce ticker for SW1 when enabled
//   - CCR2: Debounce ticker for SW2 when enabled
//  Replaces precompiled timersB0.obj.
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS12.x
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include "ports.h"
#include  "LCD.h"
#include  "macros.h"
#include  "ports.h"

//------------------------------------------------------------------------------
// Public globals consumed across the project
//------------------------------------------------------------------------------

extern volatile unsigned char update_display;        // Display service flag
extern volatile unsigned int  update_display_count;  // optional counter

// These are defined here (timersB0.c owns them)
volatile unsigned char one_time = 0;              // Legacy (not required in HW06)
volatile unsigned int  Time_Sequence = 0;         // Legacy (not required in HW06)

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
  // Stubs for other timers (not used by this project configuration)
  Init_Timer_B1();
  Init_Timer_B2();
  Init_Timer_B3();
}

void Init_Timer_B0(void) {
  // Stop and configure Timer_B0 for continuous mode sourced from SMCLK
  TB0CTL = TBSSEL__SMCLK | MC__STOP | TBCLR;   // SMCLK, stop, clear
  TB0CTL |= ID__8;                             // Divide SMCLK by 8
  TB0EX0  = TBIDEX_7;                          // Extra divide by 8 => 125kHz timer clock

  // CCR0: 200ms backlight blink + display service
  TB0CCTL0 = CCIE;                             // Enable CCR0 interrupt
  TB0CCR0  = TB0R + (unsigned int)CCR0_DELTA_COUNTS;

  // CCR1/CCR2: disabled until a debounce begins
  TB0CCTL1 = 0;
  TB0CCTL2 = 0;

  // Start timer in continuous mode
  TB0CTL = (TB0CTL & ~(MC_3)) | MC__CONTINUOUS;
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
  // Legacy helper; retained for compatibility
  while (msec5--) { __delay_cycles(40000); }
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

//------------------------------------------------------------------------------
// ISR: TIMER0_B0_VECTOR (CCR0)
// Source: TB0CCR0 match in continuous mode
// Function: Every 200ms toggle LCD_BACKLITE and set update_display true.
// Globals used/changed:
//   - update_display (set true every 200ms)
//   - update_display_count (optional counter)
//   - P6OUT bit LCD_BACKLITE (toggle)
// Behavior during debounce:
//   - This interrupt is disabled when any debounce is active (TB0CCTL0 CCIE cleared)
//------------------------------------------------------------------------------
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void) {
  // Schedule next 200ms event
  TB0CCR0 += (unsigned int)CCR0_DELTA_COUNTS;

  // Toggle backlight output and request a display service
  P6OUT ^= LCD_BACKLITE;
  update_display_count++;
  update_display = TRUE;
}

// Timer0_B1 ISR handles CCR1/CCR2 and overflow. Used for switch debounce tickers.
#pragma vector = TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void) {
  switch (__even_in_range(TB0IV, TB0IV_TBIFG)) {
    case TB0IV_NONE:
      break; // No interrupt

    case TB0IV_TBCCR1:
      // Debounce tick for SW1 every DEBOUNCE_TICK_MS when active
      TB0CCR1 += (unsigned int)CCRx_DEBOUNCE_DELTA;
      if (debounce_active1) {
        if (debounce_count1 < 0xFFFF) debounce_count1++; // count ticks
        if (debounce_count1 >= DEBOUNCE_THRESHOLD_TICKS) {
          debounce_active1 = 0;
          TB0CCTL1 &= ~CCIE;           // disable CCR1 interrupt
          P4IFG &= ~SW1;               // clear any lingering flag
          P4IE  |= SW1;                // re-enable SW1 interrupt
        }
      } else {
        TB0CCTL1 &= ~CCIE;             // safety: disable if not active
      }
      // If both debounces are complete, re-enable backlight blink CCR0
      if (!debounce_active1 && !debounce_active2) {
        TB0CCR0 = TB0R + (unsigned int)CCR0_DELTA_COUNTS;
        TB0CCTL0 |= CCIE;
      }
      break;

    case TB0IV_TBCCR2:
      // Debounce tick for SW2 every DEBOUNCE_TICK_MS when active
      TB0CCR2 += (unsigned int)CCRx_DEBOUNCE_DELTA;
      if (debounce_active2) {
        if (debounce_count2 < 0xFFFF) debounce_count2++; // count ticks
        if (debounce_count2 >= DEBOUNCE_THRESHOLD_TICKS) {
          debounce_active2 = 0;
          TB0CCTL2 &= ~CCIE;           // disable CCR2 interrupt
          P2IFG &= ~SW2;               // clear any lingering flag
          P2IE  |= SW2;                // re-enable SW2 interrupt
        }
      } else {
        TB0CCTL2 &= ~CCIE;             // safety
      }

      // If both debounces are complete, re-enable backlight blink CCR0
      if (!debounce_active1 && !debounce_active2) {
        // Restart CCR0 timing and enable interrupt
        TB0CCR0 = TB0R + (unsigned int)CCR0_DELTA_COUNTS;
        TB0CCTL0 |= CCIE;
      }
      break;

    case TB0IV_TBIFG:
      TB0CTL &= ~TBIFG;                // Overflow
      break;
    default: break;
  }
}
