//------------------------------------------------------------------------------
//
//  Description: This file contains the display function
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

#include "msp430.h"
#include "macros.h"
#include "functions.h"

extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;

void Display_Process(void){

  if(update_display){
    update_display = 0;

    if(display_changed){
      display_changed = 0;
      Display_Update(0,0,0,0);
      
    }
  }
}
