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


// STATES ======================================================================
#define NONE                  ('N')
#define STRAIGHT              ('L')
#define CIRCLE                ('C')
#define WAIT                  ('W')
#define START                 ('S')
#define RUN                   ('R')
#define END                   ('E')


// #define WHEEL_COUNT_TIME (10)
// #define RIGHT_COUNT_TIME (7)
// #define LEFT_COUNT_TIME (8)
// #define TRAVEL_DISTANCE (2)
// #define WAITING2START (50)

// STATES ======================================================================
#define NONE                  ('N')
#define STRAIGHT              ('L')
#define CIRCLE                ('C')
#define TRIANGLE              ('T')
#define TRIANGLE_CURVE        ('t')
#define FIGURE8C1             ('F')
#define FIGURE8C2             ('f')
#define WAIT                  ('W')
#define START                 ('S')
#define RUN                   ('R')
#define END                   ('E')

// Switches
#define PRESSED                (0)
#define RELEASED               (1)
#define NOT_OKAY               (0)
#define OKAY                   (1)
#define DEBOUNCE_TIME          (12)
#define DEBOUNCE_RESTART       (0)
#define WAITING2START          (500)

#endif /* MACROS_H_ */
