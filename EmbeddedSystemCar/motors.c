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
    //  Turn ON Motors
    P6OUT  |=  R_FORWARD;
    P6OUT  |=  L_FORWARD;

}

// STOP Commands
void motorStop(void){
    //  Turn OFF Motors
    P6OUT  &= ~R_FORWARD;
    P6OUT  &= ~L_FORWARD;

}

void move(int distance_cm, int turn){
    if(turn > 50)  turn = 50;
    if(turn < -50) turn = -50;

    // Convert distance into internal "segments"
    travel_distance = distance_cm * 0.3;

    // Base motor "on-times"
    wheel_count_time = 10;              // cycle length

    // Calculate scaling from turn ratio
    float left_scale, right_scale;
    if(turn == 0){
        left_scale = 1;
        right_scale = 1;
    } else if(turn < 0){
        // Negative turn -> bias left motor
        left_scale  = 1.0f;
        right_scale = 1.0f - ((float)(-turn) / 50.0f);  
    } else {
        // Positive turn -> bias right motor
        right_scale = 1.0f;
        left_scale  = 1.0f - ((float)(turn) / 50.0f);
    }

    // Apply motor calibration AND turn scaling
    right_count_time = (unsigned int)(right_scale * 10);
    left_count_time  = (unsigned int)(left_scale  * 5);

}



// Movement Cases
void Move_Shape(void){
    switch(state){
    case  WAIT:
        wait_case();
        // Begin
        break;
    case  START:
        start_case();
        break;
    case  RUN:
        // Run actual code to make Shape
        switch(event){
        case STRAIGHT:
            // travel_distance = 20;
            // right_count_time = 10;
            // left_count_time = 6;
            // wheel_count_time = 10;
            move(20, 0);
            run_case();
            break;
        case CIRCLE:
            travel_distance = 160; // 85 Best so Far, Little Under //175
            right_count_time = 1;
            left_count_time = 9;
            wheel_count_time = 10;
            run_case();
            break;
        case TRIANGLE:
            travel_distance = 10;
            right_count_time = 10;
            left_count_time = 6;
            wheel_count_time = 10;
            run_case();
            break;
        case TRIANGLE_CURVE:
            travel_distance = 15;
            right_count_time = 0;
            left_count_time = 10;
            wheel_count_time = 10;
            run_case();
            break; // NEW??
        case FIGURE8C1:
            travel_distance = 105; // 75 Best so Far, Little Under
            right_count_time = 10;
            left_count_time = 0;
            wheel_count_time = 10;
            run_case();
            break;
        case FIGURE8C2:
            travel_distance = 120; // 75 Best so Far, Little Under
            right_count_time = 0;
            left_count_time = 10;
            wheel_count_time = 10;
            run_case();
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
    cycle_time = 0;
    right_motor_count = 0;
    left_motor_count = 0;
    segment_count = 0;
    motorsForward();
    state = RUN;
}



void end_case(void){
    motorStop();
    state = WAIT;
    switch(event){
    case FIGURE8C1:
        if(figure8_step==0){
            event = NONE;
        }else if(figure8_step==1){
            event = FIGURE8C2;
            figure8_step++;

        }else if(figure8_step==2){
            event = NONE;
            figure8_step = 0;
        }
        break;

    case TRIANGLE:
        if(triangle_step==0){
            event = NONE;
        }else if(triangle_step==12){
            event = NONE;
            strcpy(display_line[0], "   NCSU   ");
            strcpy(display_line[1], " WOLFPACK ");
            strcpy(display_line[2], "  ECE306  ");
            strcpy(display_line[3], "  GP I/O  ");
            display_changed = TRUE;
            backlight_status = 1;
            triangle_step = 0;
        }else if(triangle_step % 2){// Even
            event = TRIANGLE_CURVE;
            triangle_step++;
        }else{// Odd
            event = TRIANGLE;
            triangle_step++;
        }
        break;

    case TRIANGLE_CURVE:
        if(triangle_step==0){
            event = NONE;
        }else if(triangle_step==12){
            event = NONE;
            strcpy(display_line[0], "   NCSU   ");
            strcpy(display_line[1], " WOLFPACK ");
            strcpy(display_line[2], "  ECE306  ");
            strcpy(display_line[3], "  GP I/O  ");
            display_changed = TRUE;
            backlight_status = 1;
            triangle_step = 0;
        }else if(triangle_step % 2){// Even
            event = TRIANGLE_CURVE;
            triangle_step++;
        }else{// Odd
            event = TRIANGLE;
            triangle_step++;
        }
        break;

    default:
        event = NONE;
        strcpy(display_line[0], "   NCSU   ");
        strcpy(display_line[1], " WOLFPACK ");
        strcpy(display_line[2], "  ECE306  ");
        strcpy(display_line[3], "  GP I/O  ");
        display_changed = TRUE;
        backlight_status = 1;
        break;
    }
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
            state = END;
        }
    }
}


