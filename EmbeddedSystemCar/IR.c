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
// Module Globals (defined here, declared in IR.h)
//------------------------------------------------------------------------------
unsigned int IR = 1; // Default ON so sensors work without toggling
unsigned int IRChange = 0;

//------------------------------------------------------------------------------
// Enable/Disable IR subsystem
//------------------------------------------------------------------------------
void IR_Update(void){
    // Only force LED OFF when IR is disabled; when enabled, ISR gates the LED
    if(IR == OFF){
        P2OUT  &= ~IR_LED;
    }
}



