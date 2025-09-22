//------------------------------------------------------------------------------
//
//  Description: This file contains wheel control functions
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20
//
//------------------------------------------------------------------------------

#include "ports.h"
#include "msp430.h"
#include "macros.h"

void forward(void){

//    Select and Turn Off Backlight
//    blacklight(0);
//  Turn ON Motors
    P6OUT  |=  R_FORWARD;
    P6OUT  |=  L_FORWARD;

}

void stop(void){
//    Select and Turn On Backlight
//    backlight(1);

//  Turn OFF Motors
    P6OUT  &= ~R_FORWARD;
    P6OUT  &= ~R_REVERSE;
    P6OUT  &= ~L_FORWARD;
    P6OUT  &= ~L_REVERSE;
}

