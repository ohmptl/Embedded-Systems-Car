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
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include "timersB0.h"
#include "switch.h"


// Globals
extern unsigned char dispEvent;
extern volatile unsigned char display_changed;
extern unsigned char event;
extern char display_line[4][11];

extern unsigned int straight_step;
extern unsigned int circle_step;
extern unsigned int circle_step2;
extern unsigned int triangle_step;
extern unsigned int figure8_step;

extern unsigned char state;

// TEST
extern unsigned int backlight;
extern volatile unsigned int Time_Sequence;
extern int activateSM;


// Debounce Vars
char debounce_Status_SW1 = OFF;
char debounce_Status_SW2 = OFF;
unsigned int count_debounce_SW1;
unsigned int count_debounce_SW2;


// ENABLE SWITCHES
// ENABLE SW1
void enable_switch_SW1(void){
    P5OUT |=  SW1;
    P5DIR &= ~SW1;
}
// ENABLE SW2
void enable_switch_SW2(void){
    P5OUT |=  SW2;
    P5DIR &= ~SW2;
}
// ENABLE BOTH
void enable_switches(void){
    enable_switch_SW1();
    enable_switch_SW2();
}



// DISABLE SWITCHES
// DISABLE SW1
void disable_switch_SW1(void){
    P5OUT |=  SW1;
    P5DIR &= ~SW1;
}
// DISABLE SW2
void disable_switch_SW2(void){
    P5OUT |=  SW2;
    P5DIR &= ~SW2;
}
// DISABLE BOTH
void disable_switches(void){
    disable_switch_SW1();
    disable_switch_SW2();
}



#pragma vector=PORT4_VECTOR
__interrupt void switchP4_interrupt(void){
    // Switch 1
    if (P4IFG & SW1) {
        P4IFG &= ~SW1;          // IFG SW1 cleared
        backlight = 1;
        activateSM = TRUE;

        // Debounce
        count_debounce_SW1 = 0;
        debounce_Status_SW1 = ON;
        // Switch2 is disabled within debounceSW2() function
    }
}


#pragma vector=PORT2_VECTOR
__interrupt void switchP2_interrupt(void){
    // Switch 2
    if (P2IFG & SW2) {
        P2IFG &= ~SW2;          // IFG SW2 cleared
        activateSM = TRUE;
        backlight = 0;

        // Debounce
        count_debounce_SW2 = 0;
        debounce_Status_SW2 = ON;
        // Switch2 is disabled within debounceSW2() function
    }
}


void debounce(void){
    debounceSW1();
    debounceSW2();
}

void debounceSW1(void){
    if(debounce_Status_SW1 == ON){
        if(count_debounce_SW1 > DEBOUNCE_TIME){
            debounce_Status_SW1 = OFF;
            enable_switch_SW1();
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "  Press a ");
            strcpy(display_line[2], " Switch to");
            strcpy(display_line[3], "   Begin  ");
            display_changed = TRUE;
        }else{
            backlight = 1;
            // Update Display
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "  Switch1 ");
            display_changed = TRUE;
            disable_switch_SW2();
        }
    }else{
        enable_switch_SW1();
    }
}


void debounceSW2(void){
    if(debounce_Status_SW2 == ON){
        if(count_debounce_SW2 > DEBOUNCE_TIME){
            debounce_Status_SW2 = OFF;
            enable_switch_SW2();
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "  Press a ");
            strcpy(display_line[2], " Switch to");
            strcpy(display_line[3], "   Begin  ");
            display_changed = TRUE;
        }else{
            backlight = 0;
            // Update Display
            strcpy(display_line[0], "          ");
            strcpy(display_line[1], "          ");
            strcpy(display_line[2], "          ");
            strcpy(display_line[3], "  Switch2 ");
            display_changed = TRUE;
            disable_switch_SW2();
        }
    }else{
        enable_switch_SW2();
    }
}






// // Globals ---------------------------------------------------------------------
// extern unsigned char dispEvent;                   // Track display state
// extern volatile unsigned char display_changed;    // Track change y/n
// extern unsigned char event;                       // set event flag for motors.c
// extern char display_line[4][11];                  // 2-D char array for display 

// extern unsigned int straight_step;
// extern unsigned int circle_step;
// extern unsigned int circle_step2;
// extern unsigned int triangle_step;
// extern unsigned int figure8_step;

// extern unsigned char state;

// // TEST
// extern int Switch1_Pressed;
// extern int Switch2_Pressed;
// int okay_to_look_at_switch1=1;
// int count_debounce_SW1;
// int sw1_position=1;
// int okay_to_look_at_switch2=1;
// int count_debounce_SW2;
// int sw2_position=1;
// extern volatile unsigned int debounce_count1;
// extern volatile unsigned int debounce_count2;
// extern unsigned int backlight;
// extern unsigned int secTime;

// // Provide storage for debounce status flags referenced by timer ISR
// // Initialize to OFF so backlight can toggle when idle
// char debounce_Status_SW1 = OFF;
// char debounce_Status_SW2 = OFF;


// // Switch Functions ------------------------------------------------------------

// void Switches_Process(void){    // Error with alr being defined in switch.obj

//     Switch1_Process();
//     Switch2_Process();

// }

// void Switch1_Process(void){
// // Switch Setup-----------------------------------------------------------------
//     if (okay_to_look_at_switch1 && sw1_position){
//         if (!(P4IN & SW1)){
//             sw1_position = PRESSED;
//             okay_to_look_at_switch1 = NOT_OKAY;
//             count_debounce_SW1 = DEBOUNCE_RESTART;
// //------------------------------------------------------------------------------
//             backlight = ON;
//             state = WAIT;
//             event = GOFORWARD1;
//             secTime = 2;
// //------------------------------------------------------------------------------
//         }
//     }
//     if (count_debounce_SW1 <= DEBOUNCE_TIME){
//         count_debounce_SW1++;
//     } else{
//         okay_to_look_at_switch1 = OKAY;
//         if (P4IN & SW1){
//             sw1_position = RELEASED;
//         }
//     }
// //------------------------------------------------------------------------------
// }


// void Switch2_Process(void){
// // Switch Setup-----------------------------------------------------------------
//     if (okay_to_look_at_switch2 && sw2_position){
//         if (!(P2IN & SW2)){
//             sw2_position = PRESSED;
//             okay_to_look_at_switch2 = NOT_OKAY;
//             count_debounce_SW2 = DEBOUNCE_RESTART;
// //------------------------------------------------------------------------------
// // Enter Switch logic here
// // Switch Setup-----------------------------------------------------------------
//         }
//     }
//     if (count_debounce_SW2 <= DEBOUNCE_TIME){
//         count_debounce_SW2++;
//     }else{
//         okay_to_look_at_switch2 = OKAY;
//         if (P2IN & SW2){
//             sw2_position = RELEASED;
//         }
//     }
// //------------------------------------------------------------------------------
// }
