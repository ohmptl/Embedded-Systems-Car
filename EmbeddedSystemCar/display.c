//------------------------------------------------------------------------------
//
//  Description: This file contains the display function
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "macros.h"
#include  "ports.h"

extern volatile unsigned char display_changed;      //change tracker
extern volatile unsigned char update_display;       //update flag
extern unsigned int backlight;                      // backlight on/off flag

void Display_Process(void){

  if(update_display){
    update_display = 0;

    if(display_changed){
      display_changed = 0;
      Display_Update(0,0,0,0);
      
    }
  }
}


void backlight_update(void){

    if(backlight == 0){
        P6OUT  &= ~LCD_BACKLITE;
        P6DIR  &= ~LCD_BACKLITE;
    }
    else{
        P6OUT  |=  LCD_BACKLITE;
        P6DIR  |=  LCD_BACKLITE;
    }
}
