//------------------------------------------------------------------------------
//  Name:           wheels.c
//  Description:    Movement command execution for Project 9
//  Author:         Ohm Patel
//  Date:           Nov 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include "msp430.h"
#include "ports.h"
#include "macros.h"
#include "motors.h"
#include "serial.h"
#include "display.h"
#include <string.h>

// External globals
extern volatile unsigned int Time_Sequence;
extern char display_line[4][11];
extern volatile unsigned char display_changed;

// Movement state machine
typedef enum {
    MOVEMENT_IDLE = 0,
    MOVEMENT_EXECUTING,
    MOVEMENT_COMPLETE
} movement_state_t;

static movement_state_t current_movement_state = MOVEMENT_IDLE;
static unsigned int movement_start_time = 0;
static unsigned int movement_duration = 0;
static char current_direction = 0;

// Movement execution parameters
#define TICKS_PER_SECOND      (5u)   // Time_Sequence increments every 0.2s, so 5 ticks = 1 second

// Forward declarations
static void Wheels_ExecuteMovement(char direction, unsigned int duration_ms);
static void Wheels_UpdateDisplay(char direction, unsigned int duration_ms);
static void Wheels_StopMovement(void);

//------------------------------------------------------------------------------
//  Function: Wheels_Process
//  Description: Main wheel control processing - call this in main loop
//------------------------------------------------------------------------------
void Wheels_Process(void) {
    serial_motion_command_t cmd;
    
    // Check for new commands
    if (current_movement_state == MOVEMENT_IDLE) {
        if (Serial_DequeueMotionCommand(&cmd)) {
            // New command received - start executing
            Wheels_ExecuteMovement(cmd.direction, cmd.duration);
        }
    }
    
    // Check if current movement is complete
    if (current_movement_state == MOVEMENT_EXECUTING) {
        unsigned int elapsed = Time_Sequence - movement_start_time;
        unsigned int required_ticks = (movement_duration * TICKS_PER_SECOND) / 1000u;
        
        if (elapsed >= required_ticks) {
            // Movement complete
            Wheels_StopMovement();
        }
    }
}

//------------------------------------------------------------------------------
//  Function: Wheels_IsExecuting
//  Description: Check if a command is currently executing
//------------------------------------------------------------------------------
unsigned char Wheels_IsExecuting(void) {
    return (current_movement_state == MOVEMENT_EXECUTING) ? 1u : 0u;
}

//------------------------------------------------------------------------------
//  Function: Wheels_ExecuteMovement
//  Description: Execute a movement command
//------------------------------------------------------------------------------
static void Wheels_ExecuteMovement(char direction, unsigned int duration_ms) {
    current_movement_state = MOVEMENT_EXECUTING;
    movement_start_time = Time_Sequence;
    movement_duration = duration_ms;
    current_direction = direction;
    
    // Update display to show current command
    Wheels_UpdateDisplay(direction, duration_ms);
    
    // Execute the movement based on direction
    switch (direction) {
        case 'F':  // Forward
            PWM1_BOTH_FWD();
            break;
            
        case 'B':  // Backward
            PWM1_BOTH_REV();
            break;
            
        case 'R':  // Right turn (CW rotation)
            PWM1_LEFT_FWD();
            PWM1_RIGHT_REV();
            break;
            
        case 'L':  // Left turn (CCW rotation)
            PWM1_RIGHT_FWD();
            PWM1_LEFT_REV();
            break;
            
        default:
            // Invalid direction - stop
            Wheels_StopMovement();
            break;
    }
}

//------------------------------------------------------------------------------
//  Function: Wheels_UpdateDisplay
//  Description: Update display with command being executed
//------------------------------------------------------------------------------
static void Wheels_UpdateDisplay(char direction, unsigned int duration_ms) {
    char display_text[11];
    char dir_str[10];
    
    // Build direction string
    switch (direction) {
        case 'F':
            strcpy(dir_str, "FORWARD");
            break;
        case 'B':
            strcpy(dir_str, "BACKWARD");
            break;
        case 'R':
            strcpy(dir_str, "RIGHT");
            break;
        case 'L':
            strcpy(dir_str, "LEFT");
            break;
        default:
            strcpy(dir_str, "UNKNOWN");
            break;
    }
    
    // Format display text (e.g., "FORWARD" or "LEFT")
    // The big display should show this on line 2
    strcpy(display_text, dir_str);
    
    // Note: Display is already handled by Serial_ShowBigCommand in serial.c
    // which shows the command in BIG mode on line 2
    // This function could be used for additional display updates if needed
}

//------------------------------------------------------------------------------
//  Function: Wheels_StopMovement
//  Description: Stop all movement and return to waiting state
//------------------------------------------------------------------------------
static void Wheels_StopMovement(void) {
    // Stop all motors
    motorStop();
    
    // Update state
    current_movement_state = MOVEMENT_IDLE;
    current_direction = 0;
    
    // The display will automatically return to "WAITING" via Serial_DisplayModeService
    // once the command is no longer executing (Wheels_IsExecuting returns 0)
}

//------------------------------------------------------------------------------
//  Function: Wheels_GetCurrentState
//  Description: Get current movement state (for debugging/monitoring)
//------------------------------------------------------------------------------
movement_state_t Wheels_GetCurrentState(void) {
    return current_movement_state;
}
