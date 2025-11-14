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
#include  "timers.h"
#include  "display.h"

extern volatile unsigned char display_changed;      // change tracker
extern volatile unsigned char update_display;       // update flag (set by TB0)
extern unsigned int backlight;                      // backlight on/off flag
extern char display_line[4][11];

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

void dispPrint(char *line, char lineToUpdate) {
    char tempLine[11]; // Temporary line buffer
    int lineIndex = -1; // Index of the line to be updated

    // Determine which line to update based on the input character (1-4)
    switch (lineToUpdate) {
        case 1:
            lineIndex = 0;
            break;
        case 2:
            lineIndex = 1;
            break;
        case 3:
            lineIndex = 2;
            break;
        case 4:
            lineIndex = 3;
            break;
        default:
            // Handle invalid input
            strcpy(display_line[0], " Invalid  ");
            display_changed = TRUE;
            return;
    }

    // If the provided line is not NULL, update the corresponding display line
    if (line != NULL) {
        int spaces = (10 - strlen(line)) >> 1; // Calculate spaces for centering
        int i;
        // Initialize tempLine with spaces
        for (i = 0; i < 10; i++) {
            tempLine[i] = ' ';
        }
        tempLine[10] = '\0'; // Null-terminate

        // Copy the line string into the center of tempLine
        strncpy(tempLine + spaces, line, strlen(line));

        // Ensure the string is null-terminated
        tempLine[10] = '\0'; // Null-terminate explicitly

        // Copy the temporary line to the corresponding display line
        strcpy(display_line[lineIndex], tempLine);

        // Indicate that the display has changed
        display_changed = TRUE;
    } else {
        // Handle null parameters by displaying an error message on the selected line
        strcpy(display_line[lineIndex], " NULL Line ");
        display_changed = TRUE;
    }
}
