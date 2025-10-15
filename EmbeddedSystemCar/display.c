//------------------------------------------------------------------------------
//  Name:           display.c
//  Description:    Display update implementaion
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "LCD.h"
#include  "macros.h"
#include  "ports.h"
#include  "timersB0.h"
#include  "display.h"

extern volatile unsigned char display_changed;      // change tracker
extern volatile unsigned char update_display;       // update flag (set by TB0)
extern unsigned int backlight;                      // backlight on/off flag

// For HW06, backlight blinking is handled by TB0 CCR0 every 200ms.

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
