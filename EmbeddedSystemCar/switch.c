//------------------------------------------------------------------------------
//
//  Description: This file contains switches
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0 new
//
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  "ports.h"
#include  "macros.h"
#include  "switch.h"

// Global Variables
int count_debounce_SW1;
int count_debounce_SW2;

//------------------------------------------------------------------------------
// Interrupt-driven switch handling for SW1 (P4.1) and SW2 (P2.3)
// Debouncing uses TimerB0 5ms counters debounce_count1/2.
//------------------------------------------------------------------------------

void enable_switch_SW1(void){
    // Configure P4.1 for interrupt on high-to-low (button press), pull-up enabled
    P4IFG  &= ~SW1;              // Clear any pending flag
    P4IES  |= SW1;               // High-to-low transition
    P4IE   |= SW1;               // Enable interrupt
}

void enable_switch_SW2(void){
    // Configure P2.3 for interrupt on high-to-low (button press), pull-up enabled
    P2IFG  &= ~SW2;              // Clear any pending flag
    P2IES  |= SW2;               // High-to-low transition
    P2IE   |= SW2;               // Enable interrupt
}

void enable_switches(void){
    enable_switch_SW1();
    enable_switch_SW2();
}

void disable_switch_SW1(void){ P4IE &= ~SW1; }
void disable_switch_SW2(void){ P2IE &= ~SW2; }
void disable_switches(void){ disable_switch_SW1(); disable_switch_SW2(); }
