//------------------------------------------------------------------------------
//
//  Description: This file contains the booting sequence implementation
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

#include "msp430.h"
#include "ports.h"
#include "stateMachine.h"

extern volatile unsigned int timer200ms;

void bootSequence(void){
    P1OUT |= RED_LED;
    Init_Ports();         // Initialize Ports
    Init_Clocks();        // Initialize Clock System
    sleep(100);           // Allow time for clocks to stabilize
    Init_Conditions();    // Initialize Variables and Initial Conditions
    Init_Timers();        // Initialize Timers
    sleep(200);           // Allow time for timers to stabilize
    Init_LCD();           // Initialize LCD
    // Init_ADC();          // Initialize ADC
    sleep(200);           // Allow time for LCD to stabilize

    P1OUT &= ~RED_LED;    // Turn OFF Red LED
    P6OUT |= GRN_LED;     // Turn ON Green LED
}
