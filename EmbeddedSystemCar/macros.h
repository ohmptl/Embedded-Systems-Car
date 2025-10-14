//------------------------------------------------------------------------------
//
//  Description: This file contains the #defines
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS12.8.1
//
//------------------------------------------------------------------------------

#ifndef MACROS_H_
#define MACROS_H_

#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define RED_LED              (0x01) // RED LED 0
#define GRN_LED              (0x40) // GREEN LED 1
#define TEST_PROBE           (0x01) // 0 TEST PROBE
#define TRUE                 (0x01) //
#define ON                   (0x01) //
#define OFF                  (0x00) //


// STATES ======================================================================
#define WAIT                  ('W')
#define RUN                   ('R')
#define START                 ('S')
#define NONE                  ('N')
#define GOFORWARD1            ('F')
#define GOREVERSE             ('R')
#define GOFORWARD2            ('Q')
#define GOCW                  ('C')
#define GOCCW                 ('W')
#define END                   ('E')

// Switches
#define PRESSED                (0)
#define RELEASED               (1)
#define NOT_OKAY               (0)
#define OKAY                   (1)
#define DEBOUNCE_TIME          (12)
#define DEBOUNCE_RESTART       (0)
#define WAITING2START          (500)

// Directions
#define CW                     (2)
#define CCW                    (3)



// Homework 5
#define USE_GPIO (0x00)
#define USE_SMCLK (0x01)

#endif /* MACROS_H_ */

// TIMERS

#define TB0CCR0_INTERVAL (25000) 
#define TB0CCR1_INTERVAL (2500) 
#define TB0CCR2_INTERVAL (2500)

#define TIMER_B0_0_VECTOR (TIMER0_B0_VECTOR)
#define TIMER_B0_1_OVFL_VECTOR (TIMER0_B1_VECTOR)

#define HALF_SEC (500)
#define COUNTER_RESET (0)

