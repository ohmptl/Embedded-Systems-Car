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
#include "wheels.h"
#include <string.h>

// External globals
extern volatile unsigned int Time_Sequence;
extern char display_line[4][11];
extern volatile unsigned char display_changed;

static movement_state_t current_movement_state = MOVEMENT_IDLE;
static unsigned int movement_start_time = 0;
static unsigned int movement_duration_ticks = 0;
static char current_direction = 0;

static serial_motion_command_t queued_motion_command;
static unsigned char queued_command_valid = 0u;
static unsigned int cooldown_start_time = 0u;

// Movement execution parameters
#define DIRECTION_CHANGE_COOLDOWN_TICKS   (2u)  // 0.4s pause between opposing commands
#define MIN_MOVEMENT_TICKS                (1u)  // Guarantee at least one scheduler tick

// Forward declarations
static void Wheels_HandleIncomingCommand(const serial_motion_command_t *cmd);
static void Wheels_EnterCooldown(void);
static void Wheels_ExecuteMovement(char direction, unsigned int duration_ticks);
static void Wheels_UpdateDisplay(char direction);
static void Wheels_StopMovement(void);

//------------------------------------------------------------------------------
//  Function: Wheels_Process
//  Description: Main wheel control processing - call this in main loop
//------------------------------------------------------------------------------
void Wheels_Process(void) {
    serial_motion_command_t cmd;

    if (Serial_DequeueMotionCommand(&cmd)) {
        Wheels_HandleIncomingCommand(&cmd);
    }

    if (current_movement_state == MOVEMENT_EXECUTING) {
        unsigned int elapsed = Time_Sequence - movement_start_time;
        if (elapsed >= movement_duration_ticks) {
            Wheels_StopMovement();
            if (queued_command_valid) {
                Wheels_EnterCooldown();
            }
        }
    }

    if (current_movement_state == MOVEMENT_COOLDOWN) {
        unsigned int elapsed = Time_Sequence - cooldown_start_time;
        if (elapsed >= DIRECTION_CHANGE_COOLDOWN_TICKS) {
            current_movement_state = MOVEMENT_IDLE;
            if (queued_command_valid) {
                serial_motion_command_t next = queued_motion_command;
                queued_command_valid = 0u;
                Wheels_ExecuteMovement(next.direction, next.duration);
            }
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
//  Description: Execute a movement command using scheduler ticks for timing
//------------------------------------------------------------------------------
static void Wheels_ExecuteMovement(char direction, unsigned int duration_ticks) {
    unsigned int ticks = duration_ticks;

    if (ticks < MIN_MOVEMENT_TICKS) {
        ticks = MIN_MOVEMENT_TICKS;
    }

    current_movement_state = MOVEMENT_EXECUTING;
    movement_start_time = Time_Sequence;
    movement_duration_ticks = ticks;
    current_direction = direction;
    queued_command_valid = 0u;  // any queued command is now the active one
    
    // Update display to show current command
    Wheels_UpdateDisplay(direction);
    
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
static void Wheels_UpdateDisplay(char direction) {
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
    movement_duration_ticks = 0u;
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

//------------------------------------------------------------------------------
//  Function: Wheels_HandleIncomingCommand
//  Description: Process a newly received command with safety interlocks
//------------------------------------------------------------------------------
static void Wheels_HandleIncomingCommand(const serial_motion_command_t *cmd) {
    if (!cmd) {
        return;
    }

    char direction = cmd->direction;
    unsigned int duration_ticks = (unsigned int)cmd->duration;

    if (direction == 'S') {
        // Immediate stop request cancels any queued motion
        unsigned char was_executing = (current_movement_state == MOVEMENT_EXECUTING);
        queued_command_valid = 0u;
        Wheels_StopMovement();
        if (was_executing) {
            Wheels_EnterCooldown();
        }
        return;
    }

    if (duration_ticks == 0u) {
        // Ignore zero-length moves for safety
        return;
    }

    if (current_movement_state == MOVEMENT_EXECUTING) {
        if (direction == current_direction) {
            // Same direction: refresh timer with new duration
            Wheels_ExecuteMovement(direction, duration_ticks);
        } else {
            // Opposing direction: stop first, then queue for later
            queued_motion_command = *cmd;
            queued_command_valid = 1u;
            Wheels_StopMovement();
            Wheels_EnterCooldown();
        }
        return;
    }

    if (current_movement_state == MOVEMENT_COOLDOWN) {
        // Remember the most recent request; it will run after cooldown
        queued_motion_command = *cmd;
        queued_command_valid = 1u;
        return;
    }

    // Idle path: execute immediately
    Wheels_ExecuteMovement(direction, duration_ticks);
}

//------------------------------------------------------------------------------
//  Function: Wheels_EnterCooldown
//  Description: Insert a short pause before reversing motor direction
//------------------------------------------------------------------------------
static void Wheels_EnterCooldown(void) {
    cooldown_start_time = Time_Sequence;
    current_movement_state = MOVEMENT_COOLDOWN;
}
