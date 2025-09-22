//------------------------------------------------------------------------------
//
//  Description: This file contains the LED initialization
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

#include "msp430.h"
#include "macros.h"

void Init_LEDs(void){     // Turn on both LEDs

  P1OUT &= ~RED_LED;
  P6OUT &= ~GRN_LED;

}
