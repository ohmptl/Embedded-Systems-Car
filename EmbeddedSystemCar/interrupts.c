//------------------------------------------------------------------------------
//  Name:           interrupts.c
//  Description:    Interrupt code
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

// Header Files
#include  "msp430.h"
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "states.h"
#include  "motors.h"
#include  "display.h"
#include  "timersB0.h"
#include  "switch.h"
#include  "ADC.h"
#include  "IR.h"
#include  <string.h>
#include  <stdio.h>



// External Globals
extern volatile unsigned char update_display;

volatile unsigned int Time_Sequence;
extern char display_line[4][11];
extern volatile unsigned char display_changed;
extern unsigned int count_debounce_SW1;
extern unsigned int count_debounce_SW2;

// Timer display globals (Project 7)
extern volatile unsigned int time_ticks_200ms;    // increments every 0.2s when running
extern volatile unsigned char timer_running;      // 1 when counting, 0 when stopped

// SW1 press event flag (Project 7)
extern volatile unsigned char sw1_press_event;
extern volatile unsigned char sw2_press_event;

// ADC results used by main/state machine
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
extern volatile unsigned int ADCThumb;

extern char state;
extern char adc_char[10];

// IR flags used across modules
extern unsigned int IR;
extern unsigned int IRChange;

// Local ADC channel index for ISR rotation
static volatile unsigned int ADCChannel = 0;

// Timers
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){
    //    ADC_Update = TRUE;
    //-----------------------------------------------------------------------------
    // TimerB0 0 Interrupt handler
    //---------------------------------------------------------------------------
    update_display = TRUE;
    Time_Sequence++;
    // Increment 0.2s timer if enabled (Project 7)
    if (timer_running && time_ticks_200ms < 4999) {
        time_ticks_200ms++;
    }
    TB0CCR0 += TB0CCR0_INTERVAL;
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void){
    //---------------------------------------------------------------------------
    // TimerB0 1-2, Overflow Interrupt Vector (TBIV) handler
    //---------------------------------------------------------------------------
    switch(__even_in_range(TB0IV,14)){
    case  0: break;                    // No interrupt
    case  2:                           // CCR1 Used for SW1 Debounce
        count_debounce_SW1++;
        if (count_debounce_SW1 >= DEBOUNCE_TIME){
            count_debounce_SW1 = 0;

            TB0CCTL1 &= ~CCIE;
            P4IFG &= ~SW1;
            P4IE  |= SW1;

            //            TB0CCTL0 |= CCIE;
        }

        TB0CCR1 += TB0CCR1_INTERVAL;


        break;

    case  4:                           // CCR2 Used for SW2 Debounce
        count_debounce_SW2++;
        if (count_debounce_SW2 >= DEBOUNCE_TIME){
            count_debounce_SW2 = 0;

            TB0CCTL2 &= ~CCIE;
            P2IFG &= ~SW2;
            P2IE  |=  SW2;

            //            TB0CCTL0 |= CCIE;
        }


        TB0CCR2 += TB0CCR2_INTERVAL;

        break;

    case 14:                           // overflow available for greater than 1 second timer
        break;
    default: break;
    }
}











// #pragma vector = TIMER0_B3_VECTOR
// __interrupt void Timer0_B3_ISR(void){
//     //-----------------------------------------------------------------------------
//     // TimerB3 0 Interrupt handler
//     //---------------------------------------------------------------------------
// //    update_display = TRUE;
//     //    State_Sequence = Time_Sequence;
//     //    P6OUT ^= LCD_BACKLITE;
//     TB3CCR0 += TB3CCR0_INTERVAL;
// }


    // Switches
    //-----------------------------------------------------------------------------
    // Port 4 interrupt for switch 1, it is disabled for the duration
    // of the debounce time. Debounce time is set for 1 second
#pragma vector=PORT4_VECTOR
    __interrupt void switch1_interrupt(void) {
        // Switch 1
        if (P4IFG & SW1) {

            P4IE &= ~SW1;
            P4IFG &= ~SW1;

            TB0CCTL1 &= ~CCIFG;             // Clear SW1 debounce interrupt flag
            TB0CCR1 = TB0CCR1_INTERVAL;     // CCR1 add offset
            TB0CCTL1 |= CCIE;               // CCR1 enable interrupt

            // SW1 press: record event for main loop (Project 7)
            sw1_press_event = 1;

        }
        //-----------------------------------------------------------------------------
    }
    //-----------------------------------------------------------------------------
    // Port 2 interrupt for switch 2, it is disabled for the duration
    // of the debounce time. Debounce time is set for 1 second
#pragma vector=PORT2_VECTOR
    __interrupt void switch2_interrupt(void) {
        // Switch 2
        if (P2IFG & SW2) {

            P2IE &= ~SW2;
            P2IFG &= ~SW2;

            TB0CCTL2 &= ~CCIFG;             // Clear SW2 debounce interrupt flag
            TB0CCR2 = TB0CCR2_INTERVAL;     // CCR2 add offset
            TB0CCTL2 |= CCIE;               // CCR2 enable interrupt

            //SW2 FUNCTIONS:
            //        // Implement Later
            //        ADC_Update ^= 1; // Toggles the State of ADC_Update, Makes ADC Values Appear/Disappear on LCD
            //        if(!ADC_Update){
            //            strcpy(display_line[0], "          ");
            //            strcpy(display_line[1], "          ");
            //            strcpy(display_line[2], "          ");
            //            strcpy(display_line[3], "          ");
            //            update_display =TRUE;
            //            backlight_status = OFF;
            //        }
            
            // Project 7: SW2 press event (for calibration confirmation)
            sw2_press_event = 1;
            
            // Legacy IR toggle (still works outside calibration)
            if(IR == OFF){
                IRChange = TRUE;
                IR = ON;
            }
            else{// IR_status = ON
                IRChange = TRUE;
                IR = OFF;
            }


        }
        //-----------------------------------------------------------------------------
    }









    // ADC Interrupt
#pragma vector=ADC_VECTOR
    __interrupt void ADC_ISR(void){
        //    backlight_status = ON;
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG)){
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:   // When a conversion result is written to the ADCMEM0
            // before its previous conversion result was read.
            break;
        case ADCIV_ADCTOVIFG:   // ADC conversion-time overflow
            break;
        case ADCIV_ADCHIIFG:    // Window comparator interrupt flags
            break;
        case ADCIV_ADCLOIFG:    // Window comparator interrupt flag
            break;
        case ADCIV_ADCINIFG:    // Window comparator interrupt flag
            break;
        case ADCIV_ADCIFG:      // ADCMEM0 memory register with the conversion result
            ADCCTL0 &= ~ADCENC;                          // Disable ENC bit.
            switch (ADCChannel++){
            case 0x00:                                   // Channel A2 (Left) Interrupt
                ADCMCTL0 &= ~ADCINCH_2;                  // Disable Last channel A2
                ADCMCTL0 |=  ADCINCH_3;                  // Enable Next channel A3

                ADCLeft = ADCMEM0;                       // Move result into Global Values
                ADCLeft = ADCLeft >> 2;                   // Divide the result by 4

                // Display ADC on line 2 during calibration and normal operation
                if(state != WAIT2 && state != BLACKLINE){
                    HEXtoBCD(ADCLeft);
                    dispPrint(adc_char,2);
                }

                break;

            case 0x01:                                   // Channel A3 (Right) Interrupt
                ADCMCTL0 &= ~ADCINCH_3;                  // Disable Last channel A2
                ADCMCTL0 |=  ADCINCH_5;                  // Enable Next channel A1

                ADCRight = ADCMEM0;                      // Move result into Global Values
                ADCRight = ADCRight >> 2;                 // Divide the result by 4
                
                // Display on line 3 during calibration and normal operation
                if(state != WAIT2 && state != BLACKLINE &&
                   state != CIRCLING){  // Don't overwrite "Lap: X/2" in CIRCLING
                    HEXtoBCD(ADCRight);
                    dispPrint(adc_char,3);
                }

                break;

            case 0x02:                                   // Channel A5 (Thumb) Interrupt
                ADCMCTL0 &= ~ADCINCH_5;                  // Disable Last channel A?
                ADCMCTL0 |= ADCINCH_2;                   // Enable Next [First] channel 2

                ADCThumb = ADCMEM0;                      // Move result into Global Values
                ADCThumb = ADCThumb >> 2;                 // Divide the result by 4
                // Thumb not displayed in Project 7 (line 4 = timer)
                ADCChannel = 0;
                break;

            default:
                break;
            }
            ADCCTL0 |= ADCENC;                          // Enable Conversions
            ADCCTL0 |= ADCSC;
            break;
            default:
                break;
        }
}

