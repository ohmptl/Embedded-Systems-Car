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
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "msp430.h"
#include  "motors.h"
#include  "states.h"

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

// Globals
extern unsigned char dispEvent;
extern volatile unsigned char display_changed;
extern int Switch1_Pressed;
extern unsigned int dir;
extern unsigned int cycle_time;
extern unsigned int secTime;

// Travel time configuration (in seconds) - CHANGE THIS TO SET MOVEMENT DURATION
unsigned int travel_time = 3;  // How many seconds to move (EASILY CONFIGURABLE)
unsigned int timer_ticks_per_second = 152; // Assuming 5ms timer (200 * 5ms = 1000ms = 1 second)

// NOTE: The "Poor Man's PWM" balancing still works:
// - right_count_time/left_count_time: Control individual motor balance
// - wheel_count_time: Controls PWM cycle period  
// - travel_time: Controls total movement duration in seconds

void motorStop(void){
    // Turn OFF ALL Motors
    P6OUT &= ~R_FORWARD;
    P6OUT &= ~L_FORWARD;
    P6OUT &= ~R_REVERSE;
    P6OUT &= ~L_REVERSE;
}


// PWM1 // ALWAYS SLOW
//// FORWARD
void PWM1_LEFT_FWD(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_SLOW;
//    PWM1_RIGHT_OFF();
}
void PWM1_RIGHT_FWD(void){
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_SLOW;
//    PWM1_LEFT_OFF();
}
void PWM1_BOTH_FWD(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_SLOW;
    RIGHT_FORWARD_SPEED = PWM1_SLOW;
}

// OFF
void PWM1_LEFT_OFF(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
//    PWM1_RIGHT_OFF();
}
void PWM1_RIGHT_OFF(void){
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
//    PWM1_LEFT_OFF();
}
void PWM1_BOTH_OFF(void){
    LEFT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
}

// REVERSE
void PWM1_LEFT_REV(void){
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = PWM1_SLOW;
//    PWM1_RIGHT_OFF();
}
void PWM1_RIGHT_REV(void){
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_REVERSE_SPEED = PWM1_SLOW;
//    PWM1_LEFT_OFF();
}
void PWM1_BOTH_REV(void){
    LEFT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    RIGHT_FORWARD_SPEED = PWM1_WHEEL_OFF;
    LEFT_REVERSE_SPEED = PWM1_SLOW;
    RIGHT_REVERSE_SPEED = PWM1_SLOW;
}




// void Wheels_Process(void){
//     switch(state){
//         case  WAIT:
//             strcpy(display_line[0], " WAITING  ");
//             strcpy(display_line[1], "   FOR    ");
//             display_line[2][0] = ' ';
//             display_line[2][1] = ' ';
//             display_line[2][2] = ' ';
//             display_line[2][3] = secTime + '0';  // Convert to ASCII
//             display_line[2][4] = ' ';
//             display_line[2][5] = 'S';
//             display_line[2][6] = 'E';
//             display_line[2][7] = 'C';
//             display_line[2][8] = ' ';
//             display_line[2][9] = ' ';
//             strcpy(display_line[3], "          ");
//             display_changed = TRUE;
//             wait_case();
//             break;
//         case  START:
//             strcpy(display_line[0], "          ");
//             strcpy(display_line[1], " STARTING ");
//             strcpy(display_line[2], "          ");
//             strcpy(display_line[3], "          ");
//             display_changed = TRUE;
//             start_case();
//             break;
//         case  RUN:
//             switch(event){
//                 case GOFORWARD1:
//                     travel_distance = 10;
//                     right_count_time = 10;
//                     left_count_time = 7;
//                     wheel_count_time = 10;
//                     dir = FORWARD;
//                     travel_time = 1;
//                     strcpy(display_line[0], "  GOING   ");
//                     strcpy(display_line[1], "  FORWARD ");
//                     display_line[2][0] = ' ';
//                     display_line[2][1] = ' ';
//                     display_line[2][2] = ' ';
//                     display_line[2][3] = travel_time + '0';
//                     display_line[2][4] = ' ';
//                     display_line[2][5] = 'S';
//                     display_line[2][6] = 'E';
//                     display_line[2][7] = 'C';
//                     display_line[2][8] = ' ';
//                     display_line[2][9] = ' ';
//                     strcpy(display_line[3], "          ");
//                     display_changed = TRUE;
//                     run_case();
//                     break;
//                 case GOREVERSE:
//                     travel_distance = 10;
//                     right_count_time = 10;
//                     left_count_time = 7;
//                     wheel_count_time = 10;
//                     dir = REVERSE;
//                     travel_time = 2;
//                     strcpy(display_line[0], "  GOING   ");
//                     strcpy(display_line[1], " REVERSE  ");
//                     display_line[2][0] = ' ';
//                     display_line[2][1] = ' ';
//                     display_line[2][2] = ' ';
//                     display_line[2][3] = travel_time + '0';
//                     display_line[2][4] = ' ';
//                     display_line[2][5] = 'S';
//                     display_line[2][6] = 'E';
//                     display_line[2][7] = 'C';
//                     display_line[2][8] = ' ';
//                     display_line[2][9] = ' ';
//                     strcpy(display_line[3], "          ");
//                     display_changed = TRUE;
//                     run_case();
//                     break;
//                 case GOFORWARD2:
//                     travel_distance = 10;
//                     right_count_time = 10;
//                     left_count_time = 7;
//                     wheel_count_time = 10;
//                     dir = FORWARD;
//                     travel_time = 1;
//                     strcpy(display_line[0], "  GOING   ");
//                     strcpy(display_line[1], "  FORWARD ");
//                     display_line[2][0] = ' ';
//                     display_line[2][1] = ' ';
//                     display_line[2][2] = ' ';
//                     display_line[2][3] = travel_time + '0';
//                     display_line[2][4] = ' ';
//                     display_line[2][5] = 'S';
//                     display_line[2][6] = 'E';
//                     display_line[2][7] = 'C';
//                     display_line[2][8] = ' ';
//                     display_line[2][9] = ' ';
//                     strcpy(display_line[3], "          ");
//                     display_changed = TRUE;
//                     run_case();
//                     break;
//                 case GOCW:
//                     travel_distance = 10;
//                     right_count_time = 10;
//                     left_count_time = 10;
//                     wheel_count_time = 10;
//                     dir = CW;
//                     travel_time = 3;
//                     strcpy(display_line[0], "  GOING   ");
//                     strcpy(display_line[1], "    CW    ");
//                     display_line[2][0] = ' ';
//                     display_line[2][1] = ' ';
//                     display_line[2][2] = ' ';
//                     display_line[2][3] = travel_time + '0';
//                     display_line[2][4] = ' ';
//                     display_line[2][5] = 'S';
//                     display_line[2][6] = 'E';
//                     display_line[2][7] = 'C';
//                     display_line[2][8] = ' ';
//                     display_line[2][9] = ' ';
//                     strcpy(display_line[3], "          ");
//                     display_changed = TRUE;
//                     run_case();
//                     break;
//                 case GOCCW:
//                     travel_distance = 10;
//                     right_count_time = 10;
//                     left_count_time = 10;
//                     wheel_count_time = 10;
//                     dir = CCW;
//                     travel_time = 3;
//                     strcpy(display_line[0], "  GOING   ");
//                     strcpy(display_line[1], "   CCW    ");
//                     display_line[2][0] = ' ';
//                     display_line[2][1] = ' ';
//                     display_line[2][2] = ' ';
//                     display_line[2][3] = travel_time + '0';
//                     display_line[2][4] = ' ';
//                     display_line[2][5] = 'S';
//                     display_line[2][6] = 'E';
//                     display_line[2][7] = 'C';
//                     display_line[2][8] = ' ';
//                     display_line[2][9] = ' ';
//                     strcpy(display_line[3], "          ");
//                     display_changed = TRUE;
//                     run_case();
//                     break;
//                 case NONE:
//                     motorStop();
//                     strcpy(display_line[0], "          ");
//                     strcpy(display_line[1], " STOPPED  ");
//                     strcpy(display_line[2], "          ");
//                     strcpy(display_line[3], "          ");
//                     display_changed = TRUE;
//                     break;
//                 default:
//                     break;
//             }
//             break;
//             case  END:
//                 end_case();
//                 break;
//         default: break;
//     }


// }

// void wait_case(void){
//     if(time_change){
//         time_change = 0;
//         // Use secTime as seconds - convert to timer ticks
//         if(delay_start++ >= (secTime * timer_ticks_per_second)){
//             delay_start = 0;
//             state = START;
//         }
//     }
// }

// void start_case(void){
//     cycle_time = 0;
//     right_motor_count = 0;
//     left_motor_count = 0;
//     segment_count = 0;  // Reset timing counter for new movement
//     state = RUN;
// }

// void run_case(void){
//     if(time_change){
//         time_change = 0;
        
//         // Check if we've been moving for the desired number of seconds
//         if(segment_count >= (travel_time * timer_ticks_per_second)){
//             state = END;  // Time is up!
//         }
//         else {
//             // Increment our time counter
//             segment_count++;
            
//             // Increment cycle time for PWM control
//             cycle_time++;
            
//             // "Poor man's PWM" - Turn off motors when they reach their count time
//             if(right_motor_count++ >= right_count_time){
//                 P6OUT &= ~R_FORWARD;
//                 P6OUT &= ~R_REVERSE;
//             }
//             if(left_motor_count++ >= left_count_time){
//                 P6OUT &= ~L_FORWARD;
//                 P6OUT &= ~L_REVERSE;
//             }
            
//             // Reset PWM cycle and restart motors
//             if(cycle_time >= wheel_count_time){
//                 cycle_time = 0;
//                 right_motor_count = 0;
//                 left_motor_count = 0;
                
//                 // Turn on appropriate motors based on direction
//                 if (dir == FORWARD) {
//                     P6OUT &= ~R_REVERSE;
//                     P6OUT &= ~L_REVERSE;
//                     P6OUT |= R_FORWARD;
//                     P6OUT |= L_FORWARD;
//                 } else if (dir == REVERSE) {
//                     P6OUT &= ~R_FORWARD;
//                     P6OUT &= ~L_FORWARD;
//                     P6OUT |= R_REVERSE;
//                     P6OUT |= L_REVERSE;
//                 } else if (dir == CW) {
//                     P6OUT &= ~R_FORWARD;
//                     P6OUT &= ~L_REVERSE;
//                     P6OUT |= R_REVERSE;
//                     P6OUT |= L_FORWARD;
//                 } else if (dir == CCW) {
//                     P6OUT &= ~R_REVERSE;
//                     P6OUT &= ~L_FORWARD;
//                     P6OUT |= R_FORWARD;
//                     P6OUT |= L_REVERSE;
//                 }
//             }
//         }
//     }
// }



// void end_case(void){
//     motorStop();
//     state = WAIT;
//     switch(event){
//         case GOFORWARD1:
//             event = GOREVERSE;
//             secTime = 1;
//             break;
//         case GOREVERSE:
//             event = GOFORWARD2;
//             secTime = 1;
//             break;
//         case GOFORWARD2:
//             event = GOCW;
//             secTime = 1;
//             break;
//         case GOCW:
//             event = GOCCW;
//             secTime = 2;
//             break;
//         case GOCCW:
//             event = NONE;
//             secTime = 2;
//             break;
//         default: break;
//     }

// }


void safetyCheck(void) {
    
    if ((R_FORWARD && R_REVERSE) || (L_FORWARD && L_REVERSE)) {
    motorStop(); // Emergency stop
    strcpy(display_line[0], "  SAFETY  ");
    strcpy(display_line[1], " CONFLICT ");  
    strcpy(display_line[2], "  MOTORS  ");
    strcpy(display_line[3], "          ");
    display_changed = TRUE;
    return;
    }
}


