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
extern unsigned int mytime;
extern unsigned int right_motor_count;
extern unsigned int left_motor_count;
extern unsigned int segment_count;
extern unsigned int backlight;

extern unsigned int straight_step;
extern unsigned int circle_step;
extern unsigned int triangle_step;
extern unsigned int figure8_step;

// Globals
extern unsigned char dispEvent;
extern volatile unsigned char display_changed;

// // FORWARD Commands
// void motorsForward(void){
//     //  Turn ON Motors based on count times
//     //  If count_time is 0, that motor stays off for pivot turns
//     if(right_count_time > 0) {
//         P6OUT  |=  R_FORWARD;
//     } else {
//         P6OUT  &=  ~R_FORWARD;
//     }
//     if(left_count_time > 0) {
//         P6OUT  |=  L_FORWARD;
//     } else {
//         P6OUT  &=  ~L_FORWARD;
//     }
// }

// STOP Commands
void motorStop(void){
    //  Turn OFF Motors
    P6OUT  &= ~R_FORWARD;
    P6OUT  &= ~L_FORWARD;
}

// void Wheels_Process(void){
//     switch (states) {
//         case IDLE:
//             if(SW1_pressed){
//                 states = PAUSE;
//             }
//             break;
//         case PAUSE:
//             wait_case()
//             states = GO;
//             break;
//         case GO:
//             // RIGHT_FORWARD_SPEED = FAST;
//             // LEFT_FORWARD_SPEED = FAST;
//             // states = RUN;
//             // [Set Timer to start]
//             break;
//         case RUN:
//             if (time_passed) {
//                 states = STOP;
//             }
//             break;
//         case STOP:
//             // RIGHT_FORWARD_SPEED = WHEELS_OFF;
//             // LEFT_FORWARD_SPEED = WHEELS_OFF;
//             // states = IDLE;
//             break;
//     }
// }

// // Movement Cases
// void Move_Shape(void){
//     switch(state){
//     case  WAIT:
//         wait_case();
//         break;
//     case  START:
//         start_case();
//         break;
//     case  RUN:
//         // Run actual code to make Shape
//         switch(event){
//         case STRAIGHT:
//             // travel_distance;
//             // left_count_time;
//             // right_count_time;
//             // wheel_count_time;
//             break;
//         case CIRCLE:
//             // travel_distance;
//             // left_count_time;
//             // right_count_time;
//             // wheel_count_time;
//             break;
//         case TRIANGLE:
//             // travel_distance;
//             // left_count_time;
//             // right_count_time;
//             // wheel_count_time;
//             break;
//         case FIGURE8C1:
//             // travel_distance;
//             // left_count_time;
//             // right_count_time;
//             // wheel_count_time;
//             break;
//         case NONE:
//             motorStop();
//             break;
//         default:
//             break;
//         }
//         break;
//         case  END:
//             end_case();
//             break;
//         default: break;
//     }
// }

// void wait_case(void){
//     if(time_change){
//         time_change = 0;
//         if(delay_start++ >= WAITING2START){
//             delay_start = 0;
//             state = START;
//         }
//     }
// }


// void start_case(void){
//     // Reset all step counters
//     triangle_step = 0;
//     figure8_step = 0;
//     straight_step = 0;
//     circle_step = 0;
    
//     mytime = 0;
//     right_motor_count = 0;
//     left_motor_count = 0;
//     segment_count = 0;
    
//     state = RUN;
// }


// void end_case(void){
//     motorStop();
//     state = WAIT;
    
//     // Reset step counters and set event to NONE
//     triangle_step = 0;
//     figure8_step = 0;
//     event = NONE;
    
//     // Display completion message
//     strcpy(display_line[0], "   NCSU   ");
//     strcpy(display_line[1], " WOLFPACK ");
//     strcpy(display_line[2], "  ECE306  ");
//     strcpy(display_line[3], "  GP I/O  ");
//     display_changed = TRUE;
//     backlight_status = 1;
// }


// void run_case(void){
//     if(time_change){
//         time_change = 0;
//         if(segment_count <= travel_distance){
//             if(right_motor_count++ >= right_count_time){
//                 P6OUT &= ~R_FORWARD;
//             }
//             if(left_motor_count++ >= left_count_time){
//                 P6OUT &= ~L_FORWARD;
//             }
//             if(mytime >= wheel_count_time){
//                 mytime = 0;
//                 right_motor_count = 0;
//                 left_motor_count = 0;
//                 segment_count++;
//                 motorsForward();
//             }
//         }
//         else{
//             // Current movement segment is complete
//             // Increment step counters
//             if(event == TRIANGLE){
//                 triangle_step++;
//             } else if(event == FIGURE8C1){
//                 figure8_step++;
//             } else if(event == STRAIGHT){
//                 straight_step++;
//             } else if(event == CIRCLE){
//                 circle_step++;
//             }
            
//             // Check if shape has more segments
//             if((event == TRIANGLE && triangle_step < 12) || 
//                (event == FIGURE8C1 && figure8_step < 4) ||
//                (event == STRAIGHT && straight_step < 1) ||
//                (event == CIRCLE && circle_step < 1)){
//                 // Shape has more segments, reset for next segment
//                 segment_count = 0;
//                 // Continue in RUN state - shape function
//             } else {
//                 // Shape is complete
//                 state = END;
//             }
//         }
//     }
// }


