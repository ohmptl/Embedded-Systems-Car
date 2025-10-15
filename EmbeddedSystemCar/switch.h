//------------------------------------------------------------------------------
//
//  Description: This file contains switches
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0 new
//
//------------------------------------------------------------------------------


#ifndef SWITCHES_H_
#define SWITCHES_H_

// Switch state macros
#define PRESSED                (0)
#define RELEASED               (1)
#define NOT_OKAY               (0)
#define OKAY                   (1)

// Debounce timing for polling path in switch.c (10 x 5ms = ~50ms)
#define DEBOUNCE_TIME          (10)
#define DEBOUNCE_RESTART       (0)

void Switch1_Proj5_Process(void);

// Enable Switches
void enable_switch_SW1(void);
void enable_switch_SW2(void);
void enable_switches(void);

// Disable Switches
void disable_switch_SW1(void);
void disable_switch_SW2(void);
void disable_switches(void);

// Debounce
void debounce(void);
void debounceSW1(void);
void debounceSW2(void);

// Debounce constants are defined in macros.h (DEBOUNCE_TIME, DEBOUNCE_RESTART)

#endif /* SWITCHES_H_ */

