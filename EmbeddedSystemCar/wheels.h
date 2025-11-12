//------------------------------------------------------------------------------
//  Name:           wheels.h
//  Description:    Header for wheel/movement control (wheels.c)
//  Author:         Ohm Patel
//  Date:           Nov 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#ifndef WHEELS_H_
#define WHEELS_H_

// Movement state enumeration
typedef enum {
    MOVEMENT_IDLE = 0,
    MOVEMENT_EXECUTING,
    MOVEMENT_COMPLETE,
    MOVEMENT_COOLDOWN
} movement_state_t;

// Function Prototypes
void Wheels_Process(void);
movement_state_t Wheels_GetCurrentState(void);
unsigned char Wheels_IsExecuting(void);

#endif // WHEELS_H_
