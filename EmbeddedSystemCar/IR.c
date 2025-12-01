//------------------------------------------------------------------------------
//  Name:           IR.c
//  Description:    Infrared emitter management and line navigation helper
//  Author:         Ohm Patel
//  Date:           Dec 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include "msp430.h"
#include <stdint.h>
#include <string.h>
#include "IR.h"
#include "macros.h"
#include "ports.h"
#include "display.h"
#include "motors.h"
#include "wheels.h"

//------------------------------------------------------------------------------
// Module Globals
//------------------------------------------------------------------------------
unsigned int IR = 0;
unsigned int IRChange = 0;

extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
extern volatile unsigned int timer200ms;

//------------------------------------------------------------------------------
// Constants & Parameters
//------------------------------------------------------------------------------
// Threshold margin to detect line (ADC value > Black - Margin)
#define IR_THRESHOLD_MARGIN           (100u)
#define IR_INTERCEPT_OFFSET           (20u)

// PWM Speeds (Tune these!)
#define IR_SEARCH_SPEED_L             (15000u)
#define IR_SEARCH_SPEED_R             (15000u)
#define IR_TURN_SPEED                 (14000u)
#define IR_FOLLOW_SPEED               (13000u)
#define IR_EXIT_SPEED                 (16000u)
#define IR_MAX_PWM                    (30000u)

// Timing (ticks = seconds * 5)
#define PAUSE_TICKS                   (10u)  // 10 seconds
#define CIRCLE_DELAY_TICKS            (50u)  // 10 seconds before switching to "BL Circle"
#define EXIT_TURN_TICKS               (8u)   // Time to pivot for exit
#define EXIT_DRIVE_TICKS              (25u)  // Time to drive away (5 seconds)

// PID Gains (Tune these!)
#define K_P                           (3000l)
#define K_D                           (1000l)

//------------------------------------------------------------------------------
// State Machine
//------------------------------------------------------------------------------
typedef enum {
    STATE_IDLE = 0,
    STATE_SEARCH,
    STATE_INTERCEPT_WAIT,
    STATE_TURN,
    STATE_TURN_WAIT,
    STATE_FOLLOW,
    STATE_CIRCLE_WAIT,
    STATE_CIRCLE_CONTINUE,
    STATE_EXIT_TURN,
    STATE_EXIT_DRIVE,
    STATE_STOP
} ir_state_t;

static ir_state_t current_state = STATE_IDLE;
static unsigned int state_start_time = 0;

// Calibration
static unsigned int white_left = 0;
static unsigned int white_right = 0;
static unsigned int black_left = 0;
static unsigned int black_right = 0;
static unsigned int thresh_left = 0;
static unsigned int thresh_right = 0;
static unsigned char cal_white = 0;
static unsigned char cal_black = 0;

//------------------------------------------------------------------------------
// Helper Prototypes
//------------------------------------------------------------------------------
static void IR_Control_PID(void);
static void IR_SetStatus(const char *msg);
static unsigned char IR_OnLine(void);
static void IR_EnterState(ir_state_t next_state);

//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void IRLine_Init(void){
    current_state = STATE_IDLE;
    IR = ON;
    IRChange = TRUE;
}

void IR_Update(void){
    if(IR == ON){
        P2OUT |= IR_LED;
    } else {
        P2OUT &= ~IR_LED;
    }
}

uint8_t IRLine_IsActive(void){
    return (current_state != STATE_IDLE);
}

uint8_t IRLine_IsCalibrated(void){
    return (cal_white && cal_black);
}

irline_result_t IRLine_CalibrateWhite(irline_sample_t *sample_out){
    IR = ON;
    IRChange = TRUE;
    // Assume sensors are over white
    white_left = ADCLeft;
    white_right = ADCRight;
    cal_white = 1;
    IR_SetStatus("CAL WHITE");
    if(sample_out){
        sample_out->left = white_left;
        sample_out->right = white_right;
    }
    return IRLINE_RESULT_OK;
}

irline_result_t IRLine_CalibrateBlack(irline_sample_t *sample_out){
    IR = ON;
    IRChange = TRUE;
    // Assume sensors are over black
    black_left = ADCLeft;
    black_right = ADCRight;
    cal_black = 1;
    
    // Calculate thresholds (Black - 100)
    if(black_left > 100) thresh_left = black_left - 100;
    else thresh_left = 0;
    
    if(black_right > 100) thresh_right = black_right - 100;
    else thresh_right = 0;
    
    IR_SetStatus("CAL BLACK");
    if(sample_out){
        sample_out->left = black_left;
        sample_out->right = black_right;
    }
    return IRLINE_RESULT_OK;
}

irline_result_t IRLine_BeginFollowing(void){
    if(!cal_white || !cal_black) return IRLINE_RESULT_NEED_BLACK;
    
    IR = ON;
    IRChange = TRUE;
    IR_EnterState(STATE_SEARCH);
    return IRLINE_RESULT_OK;
}

irline_result_t IRLine_RequestDone(void){
    if(current_state == STATE_IDLE) return IRLINE_RESULT_NOT_RUNNING;
    IR_EnterState(STATE_EXIT_TURN);
    return IRLINE_RESULT_OK;
}

void IRLine_ForceStop(void){
    current_state = STATE_IDLE;
    motorStop();
    IR_SetStatus("IR STOP");
}

void IRLine_Service(void){
    if(current_state == STATE_IDLE) return;

    unsigned int elapsed = timer200ms - state_start_time;

    switch(current_state){
        case STATE_SEARCH:
            // Drive forward until line detected
            set_motor_speeds(IR_SEARCH_SPEED_L, IR_SEARCH_SPEED_R);
            if(IR_OnLine()){
                IR_EnterState(STATE_INTERCEPT_WAIT);
            }
            break;

        case STATE_INTERCEPT_WAIT:
            motorStop();
            if(elapsed >= PAUSE_TICKS){
                IR_EnterState(STATE_TURN);
            }
            break;

        case STATE_TURN:
            // Turn 90 degrees (pivot left)
            pivot_left_pwm(IR_TURN_SPEED);
            // Stop when Right sensor sees line (assuming we approached perpendicular)
            // Or just turn for a fixed time if sensors are unreliable during turn
            // Let's try sensor based:
            if(ADCRight > thresh_right){
                 IR_EnterState(STATE_TURN_WAIT);
            }
            // Timeout safety?
            if(elapsed > 50) { // 10 seconds max turn
                 IR_EnterState(STATE_TURN_WAIT);
            }
            break;

        case STATE_TURN_WAIT:
            motorStop();
            if(elapsed >= PAUSE_TICKS){
                IR_EnterState(STATE_FOLLOW);
            }
            break;

        case STATE_FOLLOW:
            IR_Control_PID();
            // Switch to Circle mode after some time
            if(elapsed >= CIRCLE_DELAY_TICKS){
                IR_SetStatus("BL Circle");
            }
            break;

        case STATE_EXIT_TURN:
            // Turn away from circle (Pivot Right)
            pivot_left_pwm(IR_TURN_SPEED);
            if(elapsed >= EXIT_TURN_TICKS){
                IR_EnterState(STATE_EXIT_DRIVE);
            }
            break;
            
        case STATE_EXIT_DRIVE:
            // Drive straight away
            set_motor_speeds(IR_EXIT_SPEED, IR_EXIT_SPEED);
            if(elapsed >= EXIT_DRIVE_TICKS){
                IR_EnterState(STATE_STOP);
            }
            break;

        case STATE_STOP:
            motorStop();
            break;
    }
}

//------------------------------------------------------------------------------
// Internal Helpers
//------------------------------------------------------------------------------

static void IR_EnterState(ir_state_t next_state){
    current_state = next_state;
    state_start_time = timer200ms;
    
    switch(next_state){
        case STATE_SEARCH:
            IR_SetStatus("BL Start");
            break;
        case STATE_INTERCEPT_WAIT:
            motorStop();
            IR_SetStatus("Intercept");
            break;
        case STATE_TURN:
            IR_SetStatus("BL Turn");
            break;
        case STATE_TURN_WAIT:
            motorStop();
            // Status remains BL Turn or can be blank? Spec says "BL Turn - while your car is turning".
            // "It also must stop for 10 to 20 seconds again after completing the turn".
            // I'll keep "BL Turn" or change to "BL Travel" early?
            // Spec: "BL Travel - while your car is traveling along the black line."
            // I'll leave it as BL Turn during the wait.
            break;
        case STATE_FOLLOW:
            IR_SetStatus("BL Travel");
            break;
        case STATE_CIRCLE_WAIT:
            motorStop();
            IR_SetStatus("BL Circle");
            break;
        case STATE_CIRCLE_CONTINUE:
            IR_SetStatus("BL Circle");
            break;
        case STATE_EXIT_TURN:
            IR_SetStatus("BL Exit");
            break;
        case STATE_EXIT_DRIVE:
            IR_SetStatus("BL Exit");
            break;
        case STATE_STOP:
            motorStop();
            IR_SetStatus("BL Stop");
            break;
        default:
            break;
    }
}

static unsigned char IR_OnLine(void){
    
    return (ADCLeft > black_left-50 || ADCRight > black_right-50);
}

static void IR_Control_PID(void){
    // Normalized Error Calculation
    // Goal: Left on White (0), Right on Black (1000)
    // Error = (Leftness) - (Rightness_Deficit)
    
    long range_L = (long)black_left - (long)white_left;
    long range_R = (long)black_right - (long)white_right;
    
    // Avoid divide by zero
    if(range_L < 100) range_L = 100;
    if(range_R < 100) range_R = 100;
    
    long norm_L = ((long)ADCLeft - (long)white_left) * 1000 / range_L;
    long norm_R = ((long)ADCRight - (long)white_right) * 1000 / range_R;
    
    // Clamp normalized values
    if(norm_L < 0) norm_L = 0;
    if(norm_L > 1000) norm_L = 1000;
    if(norm_R < 0) norm_R = 0;
    if(norm_R > 1000) norm_R = 1000;
    
    // Error Definition:
    // If Left sees Black (norm_L high) -> Positive Error (Turn Left)
    // If Right sees White (norm_R low) -> Negative Error (Turn Right)
    // Ideal: norm_L = 0, norm_R = 1000.
    // Error = norm_L - (1000 - norm_R)
    
    long error = norm_L - (1000 - norm_R);
    
    static long last_error = 0;
    
    // PD controller
    long p_term = (error * K_P) / 1000;
    long d_term = ((error - last_error) * K_D) / 1000;
    long correction = p_term + d_term;
    
    last_error = error;
    
    long left = IR_FOLLOW_SPEED - correction;
    long right = IR_FOLLOW_SPEED + correction;
    
    // Hard Correction Overrides (Latch Logic)
    // Priority: Check if we crossed over to the Left sensor first.
    if(norm_L > 500){
        // Left sensor sees Black -> We drifted Right.
        // Force Hard Left Turn.
        left = 0; 
        right += 3000; 
    }
    else if(norm_R < 500){
        // Right sensor sees White (and Left is White) -> We drifted Left.
        // Force Hard Right Turn.
        right = 0; 
        left += 3000; 
    }
    
    if(left > IR_MAX_PWM) left = IR_MAX_PWM;
    if(left < MOTOR_LEFT_SAFE_MIN_PWM) left = 0;
    
    if(right > IR_MAX_PWM) right = IR_MAX_PWM;
    if(right < MOTOR_RIGHT_SAFE_MIN_PWM) right = 0;
    
    set_motor_speeds((unsigned int)left, (unsigned int)right);
}

static void IR_SetStatus(const char *msg){
    char buffer[11];
    strncpy(buffer, msg, 10);
    buffer[10] = 0;
    dispPrint(buffer, 1); // Line 1 (Top Line)
}



