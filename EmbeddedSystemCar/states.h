//------------------------------------------------------------------------------
//  Name:           states.h
//  Description:    Movement states and events used by motors/switch logic
//  Author:         Ohm Patel
//  Date:           Oct 2025
//------------------------------------------------------------------------------

#ifndef STATES_H_
#define STATES_H_

// // High-level system state
// #define WAIT                  ('W')
// #define RUN                   ('R')
// #define START                 ('S')
// #define END                   ('E')

// // Motion events
// #define NONE                  ('N')
// #define GOFORWARD1            ('F')
// #define GOREVERSE             ('R')
// #define GOFORWARD2            ('Q')
// #define GOCW                  ('C')
// #define GOCCW                 ('W')

// // Rotation directions (distinct from FORWARD/REVERSE motor direction)
// #define CW                    (2)
// #define CCW                   (3)

// States
#define IDLE                    ('I')
#define WAIT                    ('W')
#define FWD                     ('F')
#define BLACKLINE               ('B')
#define WAIT2                   ('w')
#define TURNL                   ('T')
#define LINE1                   ('L')
#define LINE2                   ('l')
#define DONE                    ('D')


#endif // STATES_H_
