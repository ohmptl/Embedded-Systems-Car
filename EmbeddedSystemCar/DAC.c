//------------------------------------------------------------------------------
//  Name:           DAC.c
//  Description:    Simple DAC (SAC3) configuration for LT1935 buck-boost
//                  power system control on the MSP430FR2355 car board.
//                  Based on instructor-provided example (DAC buffer mode).
//------------------------------------------------------------------------------

#include "msp430.h"
#include "macros.h"
#include "functions.h"
#include "ports.h"

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------

// 12-bit DAC data register shadow
// Range: 0x000 - 0xFFF, interpreted versus AVCC reference by SAC3
unsigned int DAC_data = 0;

//------------------------------------------------------------------------------
// Simple tuning constants for DAC-based supply voltage
//------------------------------------------------------------------------------
// These are from the instructor notes. Begin around 2V and walk down until
// the measured output is close to 6.0V on the LT1935 output.
// You can swap to other pairs if you want a different final voltage.

const unsigned int DAC_Begin  = 2725u;   // ~2.0V starting value
const unsigned int DAC_Limit  = 850u;    // ~6.08V target
const unsigned int DAC_Adjust = 875u;    // ~6.00V fine-adjust value

//------------------------------------------------------------------------------
// Init_DAC
//   Configures SAC3 as a buffered 12-bit DAC using AVCC as reference.
//   The analog output is routed to the LT1935 FB_DAC node via P3.5
//   (DAC_CNTL_3) and power is enabled by P2.5 (DAC_ENB).
//------------------------------------------------------------------------------

void Init_DAC(void) {
  // Start at instructor-recommended beginning value
  DAC_data = DAC_Begin;

  // Load initial DAC value
  SAC3DAT  = DAC_data;                  // Initial DAC data

  // DAC configuration: AVCC reference, latch on write
  SAC3DAC  = DACSREF_0;                 // Select AVCC as DAC reference
  SAC3DAC |= DACLSEL_0;                 // DAC latch loads when SAC3DAT written

  // Operational amplifier (OA) configuration in buffer mode
  SAC3OA   = NMUXEN;                    // Negative input MUX control
  SAC3OA  |= PMUXEN;                    // Positive input MUX control
  SAC3OA  |= PSEL_1;                    // 12-bit DAC output used as OA+ input
  SAC3OA  |= NSEL_1;                    // Select external pin as OA- input
  SAC3OA  |= OAPM;                      // Low-speed, low-power mode

  SAC3PGA  = MSEL_1;                    // OA in buffer mode

  SAC3OA  |= SACEN;                     // Enable SAC module
  SAC3OA  |= OAEN;                      // Enable OA

  // Route DAC output to the P3.5 analog pin (DAC_CNTL_3)
  P3OUT  &= ~DAC_CNTL_3;                // Ensure output low before analog mode
  P3DIR  &= ~DAC_CNTL_3;                // Input direction when used as analog
  P3SELC |=  DAC_CNTL_3;                // Select analog function for DAC output

  // Enable the DAC core
  SAC3DAC |= DACEN;                     // Enable DAC

  // Enable LT1935 buck-boost power system (port 2.5)
  // NOTE: Port 2 pin direction and function are already configured in
  //       Init_Port2. Here we simply turn the supply on.
  P2OUT |= DAC_ENB;                     // Set DAC_ENB High (power on)
}
