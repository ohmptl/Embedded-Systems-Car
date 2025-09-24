//------------------------------------------------------------------------------
//
//  Description: This file contains the movement functions
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------


#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "msp430.h"

// Globals
extern unsigned char state;
extern unsigned char event;
extern char display_line[4][11];
extern unsigned int travel_distance;
extern unsigned int right_count_time;
extern unsigned int left_count_time;
extern unsigned int wheel_count_time;

extern unsigned int time_change;
extern unsigned int delay_start;
extern unsigned int cycle_time;
extern unsigned int right_motor_count;
extern unsigned int left_motor_count;
extern unsigned int segment_count;
extern unsigned int backlight_status;

extern unsigned int straight_step;
extern unsigned int circle_step;
extern unsigned int triangle_step;
extern unsigned int figure8_step;

// Globals
extern unsigned char dispEvent;
extern volatile unsigned char display_changed;
extern unsigned char event;
extern char display_line[4][11];

// FORWARD Commands
void motorsForward(void){
    //  Turn ON Motors based on count times
    //  If count_time is 0, that motor stays off for pivot turns
    if(right_count_time > 0) {
        P6OUT  |=  R_FORWARD;
    } else {
        P6OUT  &=  ~R_FORWARD;
    }
    if(left_count_time > 0) {
        P6OUT  |=  L_FORWARD;
    } else {
        P6OUT  &=  ~L_FORWARD;
    }
}

// STOP Commands
void motorStop(void){
    //  Turn OFF Motors
    P6OUT  &= ~R_FORWARD;
    P6OUT  &= ~L_FORWARD;

}

void move(int distance, int turn, int wheel_count){
    if(turn > 50)  turn = 50;
    if(turn < -50) turn = -50;
    
    // Convert distance into internal "segments"
    travel_distance = (unsigned int)distance;

    // Base motor "on-times"
    wheel_count_time = wheel_count;              // cycle length

    // Motor balancing factors (adjust these to compensate for motor differences)
    float right_motor_factor = 1.0f;  // Adjust if right motor is stronger/weaker
    float left_motor_factor = 0.45f;   // Adjust if left motor is stronger/weaker
    
    // Handle pivot turns explicitly (only for exactly ±50)
    if(turn == 50){
        // Maximum right turn - pivot on right wheel
        right_count_time = 0;  // Right motor off
        left_count_time = (unsigned int)(10.0f * left_motor_factor + 0.5f);
    }
    else if(turn == -50){
        // Maximum left turn - pivot on left wheel  
        left_count_time = 0;   // Left motor off
        right_count_time = (unsigned int)(10.0f * right_motor_factor + 0.5f);
    }
    else {
        // Calculate scaling for regular turns (prevent motors from going to 0)
        float left_scale, right_scale;
        if(turn == 0){
            left_scale = 1.0f;
            right_scale = 1.0f;
        } else if(turn < 0){
            // Negative turn -> slow down left motor, but keep it above minimum
            right_scale = 1.0f;
            left_scale = 1.0f - ((float)(-turn) / 50.0f);  // Use 70 instead of 50 to prevent going to 0
            if(left_scale < 0.1f) left_scale = 0.1f;  // Minimum 20% speed
        } else {
            // Positive turn -> slow down right motor, but keep it above minimum  
            left_scale = 1.0f;
            right_scale = 1.0f - ((float)(turn) / 50.0f);  // Use 70 instead of 50 to prevent going to 0
            if(right_scale < 0.1f) right_scale = 0.1f;  // Minimum 20% speed
        }

        // Apply motor calibration AND turn scaling
        right_count_time = (unsigned int)(right_scale * 10.0f * right_motor_factor + 0.5f);
        left_count_time  = (unsigned int)(left_scale  * 10.0f * left_motor_factor + 0.5f);
        
        // Ensure minimum count times (at least 1 to keep motors running)
        if(left_count_time < 1) left_count_time = 1;
        if(right_count_time < 1) right_count_time = 1;
    }

}



// Shape-specific movement functions
void run_straight(void){
    if(segment_count == 0 && straight_step == 0){
        move(10, 0, 10);
        straight_step++;
    }
    run_case();
}

void run_circle(void){
    if(segment_count == 0 && circle_step == 0){
        move(70, 49, 25);
        circle_step++;
    }
    run_case();
}

void run_triangle(void){
    // Only set new move parameters when starting a new segment
    if(segment_count == 0){
        switch(triangle_step){
            case 0:
                move(10, 0, 10);   // Side 1: straight
                break;
            case 1:
                move(11, 50, 10);  // Turn 120 degrees
                break;
            case 2:
                move(10, 0, 10);   // Side 2: straight  
                break;
            case 3:
                move(11, 50, 10);  // Turn 120 degrees
                break;
            case 4:
                move(10, 0, 10);   // Side 3: straight
                break;
            case 5:
                move(11, 50, 10);  // Turn 120 degrees to complete triangle
                break;
            case 6:
                move(10, 0, 10);   // Side 1: straight
                break;
            case 7:
                move(11, 50, 10);  // Turn 120 degrees
                break;
            case 8:
                move(10, 0, 10);   // Side 2: straight  
                break;
            case 9:
                move(11, 50, 10);  // Turn 120 degrees
                break;
            case 10:
                move(10, 0, 10);   // Side 3: straight
                break;
            case 11:
                move(11, 50, 10);  // Turn 120 degrees to complete triangle
                break;
        }
    }
    run_case();
}

void run_figure8(void){
    // Only set new move parameters when starting a new segment
    if(segment_count == 0){
        switch(figure8_step){
            case 0:
                move(38, 49,20);   // First circle (clockwise)
                break;
            case 1:
                move(35, -49,20);  // Second circle (counterclockwise)
                break;
            case 2:
                move(38, 49,20);   // First circle (clockwise)
                break;
            case 3:
                move(35, -49,20);  // Second circle (counterclockwise)
                break;
        }
    }
    run_case();
}

// Movement Cases
void Move_Shape(void){
    switch(state){
    case  WAIT:
        wait_case();
        break;
    case  START:
        start_case();
        break;
    case  RUN:
        // Run actual code to make Shape
        switch(event){
        case STRAIGHT:
            run_straight();
            break;
        case CIRCLE:
            run_circle();
            break;
        case TRIANGLE:
            run_triangle();
            break;
        case FIGURE8C1:
            run_figure8();
            break;
        case NONE:
            motorStop();
            break;
        default:
            break;
        }
        break;
        case  END:
            end_case();
            break;
        default: break;
    }
}

void wait_case(void){
    if(time_change){
        time_change = 0;
        if(delay_start++ >= WAITING2START){
            delay_start = 0;
            state = START;
        }
    }
}


void start_case(void){
    // Reset all step counters
    triangle_step = 0;
    figure8_step = 0;
    straight_step = 0;
    circle_step = 0;
    
    cycle_time = 0;
    right_motor_count = 0;
    left_motor_count = 0;
    segment_count = 0;
    
    state = RUN;
}



void end_case(void){
    motorStop();
    state = WAIT;
    
    // Reset step counters and set event to NONE
    triangle_step = 0;
    figure8_step = 0;
    event = NONE;
    
    // Display completion message
    strcpy(display_line[0], "   NCSU   ");
    strcpy(display_line[1], " WOLFPACK ");
    strcpy(display_line[2], "  ECE306  ");
    strcpy(display_line[3], "  GP I/O  ");
    display_changed = TRUE;
    backlight_status = 1;
}


void run_case(void){
    if(time_change){
        time_change = 0;
        if(segment_count <= travel_distance){
            if(right_motor_count++ >= right_count_time){
                P6OUT &= ~R_FORWARD;
            }
            if(left_motor_count++ >= left_count_time){
                P6OUT &= ~L_FORWARD;
            }
            if(cycle_time >= wheel_count_time){
                cycle_time = 0;
                right_motor_count = 0;
                left_motor_count = 0;
                segment_count++;
                motorsForward();
            }
        }
        else{
            // Current movement segment is complete
            // Increment step counters
            if(event == TRIANGLE){
                triangle_step++;
            } else if(event == FIGURE8C1){
                figure8_step++;
            } else if(event == STRAIGHT){
                straight_step++;
            } else if(event == CIRCLE){
                circle_step++;
            }
            
            // Check if shape has more segments
            if((event == TRIANGLE && triangle_step < 12) || 
               (event == FIGURE8C1 && figure8_step < 4) ||
               (event == STRAIGHT && straight_step < 1) ||
               (event == CIRCLE && circle_step < 1)){
                // Shape has more segments, reset for next segment
                segment_count = 0;
                // Continue in RUN state - shape function will be called again
            } else {
                // Shape is complete
                state = END;
            }
        }
    }
}


