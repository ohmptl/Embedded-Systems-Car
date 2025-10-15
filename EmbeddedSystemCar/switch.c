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
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "timersB0.h"
#include  "switch.h"
#include  "motors.h"
#include  "states.h"

// Globals ---------------------------------------------------------------------
extern unsigned char dispEvent;                   // Track display state
extern volatile unsigned char display_changed;    // Track change y/n
extern unsigned char event;                       // set event flag for motors.c
extern char display_line[4][11];                  // 2-D char array for display 

extern unsigned char state;

// TEST
extern int Switch1_Pressed;
extern int Switch2_Pressed;
int okay_to_look_at_switch1=1;
int count_debounce_SW1;
int sw1_position=1;
int okay_to_look_at_switch2=1;
int count_debounce_SW2;
int sw2_position=1;
extern volatile unsigned int debounce_count1;
extern volatile unsigned int debounce_count2;
extern unsigned int backlight;
extern unsigned int secTime;
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;
extern void update_string(char *s, int line);


// // Switch Functions ------------------------------------------------------------ OLD

// void Switches_Process(void){    // Polling fallback (kept for compatibility)

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
// //------------------------------------------------------------------------------ END OLD
// }

//------------------------------------------------------------------------------
// Interrupt-driven switch handling for SW1 (P4.1) and SW2 (P2.3)
// Debouncing uses TimerB0 5ms counters debounce_count1/2.
//------------------------------------------------------------------------------

void enable_switch_SW1(void){
    // Configure P4.1 for interrupt on high-to-low (button press), pull-up enabled
    P4IFG  &= ~SW1;              // Clear any pending flag
    P4IES  |= SW1;               // High-to-low transition
    P4IE   |= SW1;               // Enable interrupt
}

void enable_switch_SW2(void){
    // Configure P2.3 for interrupt on high-to-low (button press), pull-up enabled
    P2IFG  &= ~SW2;              // Clear any pending flag
    P2IES  |= SW2;               // High-to-low transition
    P2IE   |= SW2;               // Enable interrupt
}

void enable_switches(void){
    enable_switch_SW1();
    enable_switch_SW2();
}

void disable_switch_SW1(void){ P4IE &= ~SW1; }
void disable_switch_SW2(void){ P2IE &= ~SW2; }
void disable_switches(void){ disable_switch_SW1(); disable_switch_SW2(); }

// Descriptive headers for ISRs are at the vector declarations

// //------------------------------------------------------------------------------
// // ISR: Port 4 (handles SW1 on P4.1)
// // Triggers on a high-to-low transition (button press). Uses a 5ms-based
// // debounce by requiring debounce_count1 >= DEBOUNCE_TIME before accepting.
// // On valid press, schedules the same actions as the polling path and updates
// // polling guards to avoid duplicate handling.
// //------------------------------------------------------------------------------
// #pragma vector=PORT4_VECTOR
// __interrupt void Port4_ISR(void){
//     if (P4IFG & SW1){
//         P4IFG &= ~SW1;                   // Clear flag early
//         // Begin debounce immediately on press
//         debounce_count1 = 0;             // Reset debounce counter
//         debounce_active1 = 1;            // Mark debounce in progress
//         P4IE &= ~SW1;                    // Disable SW1 interrupt until debounce complete

//         // Start CCR1 debounce ticker (50ms)
//         TB0CCR1  = TB0R + (unsigned int)CCRx_DEBOUNCE_DELTA;
//         TB0CCTL1 |= CCIE;

//         // Disable backlight blink while debouncing (per spec)
//         TB0CCTL0 &= ~CCIE;
//         P6OUT &= ~LCD_BACKLITE;          // Ensure backlight off during debounce

//         // Update display line 4 with "Switch 1"
//         update_string("  Switch 1", 3);
//         display_changed = 1;
//         update_display = 1;
//     }
// }

// //------------------------------------------------------------------------------
// // ISR: Port 2 (handles SW2 on P2.3)
// // Similar debounce scheme to SW1. Define action per homework requirement.
// // Here we demonstrate by toggling the LCD backlight and flagging display update.
// //------------------------------------------------------------------------------
// #pragma vector=PORT2_VECTOR
// __interrupt void Port2_ISR(void){
//     if (P2IFG & SW2){
//         P2IFG &= ~SW2;                   // Clear flag early
//         // Begin debounce immediately on press
//         debounce_count2 = 0;
//         debounce_active2 = 1;
//         P2IE &= ~SW2;                    // Disable SW2 interrupt until debounce complete

//         // Start CCR2 debounce ticker (50ms)
//         TB0CCR2  = TB0R + (unsigned int)CCRx_DEBOUNCE_DELTA;
//         TB0CCTL2 |= CCIE;

//         // Disable backlight blink while debouncing
//         TB0CCTL0 &= ~CCIE;
//         P6OUT &= ~LCD_BACKLITE;

//         // Update display line 4 with "Switch 2"
//         update_string("  Switch 2", 3);
//         display_changed = 1;
//         update_display = 1;
//     }
// }


