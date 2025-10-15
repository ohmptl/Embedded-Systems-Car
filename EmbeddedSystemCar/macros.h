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
#define DEBOUNCE_TIME          (10)  // 10 x 5ms = ~50ms debounce
#define DEBOUNCE_RESTART       (0)
#define WAITING2START          (500)

// Directions
#define CW                     (2)
#define CCW                    (3)



// Homework 5
#define USE_GPIO (0x00)
#define USE_SMCLK (0x01)

// Homework 6 - LCD Blink timing using 5ms TB0 system tick
#define LCD_BLINK_HALF_TICKS   (100) // 100 x 5ms = 500ms (2 Hz blink)

#endif /* MACROS_H_ */
