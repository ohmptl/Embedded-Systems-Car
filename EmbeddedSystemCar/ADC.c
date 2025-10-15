//------------------------------------------------------------------------------
//  Name:           ADC.c
//  Description:    Implementation for Analog to Digital Converter
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#include  <string.h>
#include  "msp430.h"
#include  "macros.h"
#include  "ADC.h"
#include  "LCD.h"
#include  "ports.h"
// ------------------------------------------------------------------------------

// Global Variables
extern char display_line[4][11];
char adc_char[10]; // Holds the BCD characters for display (null-terminated)

// ADC results used elsewhere in the app (main.c)
volatile unsigned int ADCLeft;
volatile unsigned int ADCRight;
volatile unsigned int ADCThumb;

void Init_ADC(void){
  //-----------------------------------------------------------------------------
  // V_DETECT_L (0x04)      // Pin 2 A2
  // V_DETECT_R (0x08)      // Pin 3 A3
  // V_THUMB    (0x20)      // Pin 5 A5
  //-----------------------------------------------------------------------------
  // ADCCTL0 Register
  ADCCTL0 = 0;              // Reset
    ADCCTL0 |= ADCSHT_2;      // 16 ADC clocks (LED gated by ISR)
  ADCCTL0 |= ADCMSC;        // MSC
  ADCCTL0 |= ADCON;         // ADC ON
  
  // ADCCTL1 Register
  ADCCTL1 = 0;              // Reset
  ADCCTL1 |= ADCSHS_0;      // 00b = ADCSC bit
  ADCCTL1 |= ADCSHP;        // ADC sample-and-hold SAMPCON signal from sampling timer.
  ADCCTL1 &= ~ADCISSH;      // ADC invert signal sample-and-hold.
  ADCCTL1 |= ADCDIV_0;      // ADC clock divider 000b = Divide by 1
    ADCCTL1 |= ADCSSEL_2;     // ADC clock SMCLK for stability
  ADCCTL1 |= ADCCONSEQ_0;   // ADC conversion sequence 00b = Single-channel single-conversion
  // ADCCTL1 & ADCBUSY identifies a conversion is in process
  
  // ADCCTL2 Register
  ADCCTL2 = 0;              // Reset
    ADCCTL2 &= ~(ADCPDIV_3);  // Clear pre-divider bits (use /1)
  ADCCTL2 |= ADCRES_2;      // ADC resolution 10b = 12 bit (14 clock cycle conversion time)
  ADCCTL2 &= ~ADCDF;        // ADC data read-back format 0b = Binary unsigned.
  ADCCTL2 &= ~ADCSR;        // ADC sampling rate 0b = ADC buffer supports up to 200 ksps

  // ADCMCTL0 Register
    ADCMCTL0 |= ADCSREF_0;    // VREF 000b = {VR+ = AVCC and VR– = AVSS }
    // Start conversion sequence on A2 (Left) first; ISR rotates A2->A3->A5
    ADCMCTL0 |= ADCINCH_2;

  ADCIE |= ADCIE0;          // Enable ADC conv complete interrupt
  ADCCTL0 |= ADCENC;        // ADC enable conversion.
  ADCCTL0 |= ADCSC;         // ADC start conversion.
}


//----------------------------------------------------------------
// Hex to BCD Conversion
// Convert a Hex number to a BCD for display on an LCD or monitor
//----------------------------------------------------------------
void HEXtoBCD(int hex_value) {
    int i, value = 0;
    // char adc_char[4]; duplicate ????

    // Initialize all BCD characters to '0'
    for(i = 0; i < 4; i++) {
        adc_char[i] = '0';
    }

    // Thousands place
    while(hex_value > 999) {
        hex_value -= 1000;
        value += 1;
        adc_char[0] = 0x30 + value;
    }

    // Hundreds place
    value = 0;
    while(hex_value > 99) {
        hex_value -= 100;
        value += 1;
        adc_char[1] = 0x30 + value;
    }

    // Tens place
    value = 0;
    while(hex_value > 9) {
        hex_value -= 10;
        value += 1;
        adc_char[2] = 0x30 + value;
    }

    // Ones place
    adc_char[3] = 0x30 + hex_value;
}
//----------------------------------------------------------------


//----------------------------------------------------------------
// ADC Line insert
// Take the HEX to BCD value in the array adc_char and place it
// in the desired location on the desired line of the display.
// char line => Specifies the line 1 thru 4
// char location => Is the location 0 thru 9
//----------------------------------------------------------------
void adc_line(char line, char location) {
    int i;
    unsigned int real_line;
    // Convert line '1' to index 0, etc.
    real_line = line - 1;

    for(i = 0; i < 4; i++) {
        display_line[real_line][i + location] = adc_char[i];
    }
}
//------------------------------------------------------------
