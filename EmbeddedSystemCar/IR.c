//------------------------------------------------------------------------------
//  Name:           IR.c
//  Description:    Infrared sensor/LED control (template)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include "msp430.h"
#include <string.h>
#include "IR.h"
#include "macros.h"
#include "ports.h"

//------------------------------------------------------------------------------
// Module Globals (optional)
//------------------------------------------------------------------------------
unsigned int IR; // IR status Flag (ON/OFF)
unsigned int IRChange;

//------------------------------------------------------------------------------
// Enable/Disable IR subsystem
//------------------------------------------------------------------------------
void IR_Update(void){
    if(IR == OFF){
        P2OUT  &= ~IR_LED;
    }
    else{
        P2OUT  |=  IR_LED;
    }
}



