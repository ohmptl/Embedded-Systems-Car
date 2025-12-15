//------------------------------------------------------------------------------
//  Name:           IR.c
//  Description:    Infrared emitter management and line navigation helper
//  Author:         Ohm Patel
//  Date:           Oct 2025 (updated Nov 2025)
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
// Module Globals (defined here, declared in IR.h)
//------------------------------------------------------------------------------
unsigned int IR = 0;
unsigned int IRChange = 0;

// External sensor samples and timing (owned by ADC/Timers)
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
extern volatile unsigned int timer200ms;

//------------------------------------------------------------------------------
// Line-follow tuning constants
//------------------------------------------------------------------------------
#define IR_LINE_THRESHOLD_MARGIN_MIN      (10u)
#define IR_LINE_THRESHOLD_MARGIN_MAX      (20u)
#define IR_LINE_SEARCH_LEFT_PWM           (21000u)
#define IR_LINE_SEARCH_RIGHT_PWM          (11000u)
#define IR_LINE_ATTACH_SPEED              (16000u)
#define IR_LINE_ALIGN_BASE_PWM            (13000u)
#define IR_LINE_FOLLOW_BASE_PWM           (15000u)
#define IR_LINE_MAX_PWM                   (32000u)
#define IR_LINE_GAIN_NUMERATOR            (14000l)
#define IR_LINE_ATTACH_DEFICIT_GAIN       (80l)
#define IR_LINE_DEFICIT_SLOWDOWN_THRESHOLD (30u)
#define IR_LINE_DEFICIT_SPEED_DROP        (4000u)
#define IR_LINE_TARGET_OFFSET             (50u)
#define IR_LINE_TARGET_MIN_FRACTION       (4u)
#define IR_LINE_ATTACH_PAUSE_TICKS        (2u)
#define IR_LINE_ATTACH_TURN_TICKS         (5u)
#define IR_LINE_ALIGN_TOLERANCE           (25)
#define IR_LINE_ALIGN_STABLE_TICKS        (5u)
#define IR_LINE_SEARCH_TIMEOUT_TICKS      (150u)
#define IR_LINE_EXIT_DURATION_TICKS       (TICKS_PER_SECOND * 3u)
#define IR_LINE_LOST_BIAS_PWM             (7000)

typedef enum {
    IR_LINE_STATE_IDLE = 0,
    IR_LINE_STATE_SEARCH,
    IR_LINE_STATE_ATTACH_PAUSE,
    IR_LINE_STATE_ATTACH_TURN,
    IR_LINE_STATE_ALIGN,
    IR_LINE_STATE_FOLLOW,
    IR_LINE_STATE_EXIT_DRIVE
} ir_line_state_t;

typedef enum {
    IR_SIDE_NONE = 0,
    IR_SIDE_LEFT,
    IR_SIDE_RIGHT
} ir_line_side_t;

typedef struct {
    unsigned int white_left;
    unsigned int white_right;
    unsigned int black_left;
    unsigned int black_right;
    unsigned int threshold_left;
    unsigned int threshold_right;
    unsigned int attach_target_left;
    unsigned int attach_target_right;
    unsigned char white_valid;
    unsigned char black_valid;
} ir_line_calibration_t;

static ir_line_calibration_t ir_calibration = {0};
static ir_line_state_t ir_line_state = IR_LINE_STATE_IDLE;
static unsigned int ir_line_state_stamp = 0u;
static unsigned int ir_line_align_stable = 0u;
static unsigned int ir_line_lost_ticks = 0u;
static ir_line_side_t ir_last_detect_side = IR_SIDE_NONE;

//------------------------------------------------------------------------------
// Forward declarations (internal helpers)
//------------------------------------------------------------------------------
static void IRLine_EnableEmitter(void);
static void IRLine_ShowStatus(const char *status);
static void IRLine_EnterState(ir_line_state_t next);
static void IRLine_SetArcSpeeds(void);
static void IRLine_CommandSpeeds(unsigned int left_pwm, unsigned int right_pwm);
static void IRLine_CommandBalanced(unsigned int base_pwm,
                                   unsigned int left_sample,
                                   unsigned int right_sample,
                                   unsigned char bias_when_lost);
static void IRLine_HandleSearch(void);
static void IRLine_HandleAttachPause(void);
static void IRLine_HandleAttachTurn(void);
static void IRLine_HandleAlign(void);
static void IRLine_HandleFollow(void);
static void IRLine_HandleExitDrive(void);
static uint8_t IRLine_SensorSeesLine(unsigned int sample, unsigned int threshold);
static void IRLine_UpdateThresholds(void);
static unsigned int IRLine_SelectMargin(unsigned int black, unsigned int white);
static unsigned int IRLine_SelectAttachTarget(unsigned int black);
static void IRLine_ResetTracking(void);

//------------------------------------------------------------------------------
// Enable/Disable IR subsystem (emitter LED drive)
//------------------------------------------------------------------------------
void IR_Update(void){
    if(IR == ON){
        P2OUT  |=  IR_LED;
    } else {
        P2OUT  &= ~IR_LED;
    }
}

//------------------------------------------------------------------------------
// Public control API
//------------------------------------------------------------------------------
void IRLine_Init(void){
    IRLine_ResetTracking();
    ir_line_state = IR_LINE_STATE_IDLE;
    ir_line_state_stamp = timer200ms;
}

void IRLine_Service(void){
    if(ir_line_state != IR_LINE_STATE_IDLE){
        IRLine_EnableEmitter();
    }

    switch(ir_line_state){
        case IR_LINE_STATE_IDLE:
            return;
        case IR_LINE_STATE_SEARCH:
            IRLine_HandleSearch();
            break;
        case IR_LINE_STATE_ATTACH_PAUSE:
            IRLine_HandleAttachPause();
            break;
        case IR_LINE_STATE_ATTACH_TURN:
            IRLine_HandleAttachTurn();
            break;
        case IR_LINE_STATE_ALIGN:
            IRLine_HandleAlign();
            break;
        case IR_LINE_STATE_FOLLOW:
            IRLine_HandleFollow();
            break;
        case IR_LINE_STATE_EXIT_DRIVE:
            IRLine_HandleExitDrive();
            break;
        default:
            ir_line_state = IR_LINE_STATE_IDLE;
            motorStop();
            break;
    }
}

irline_result_t IRLine_CalibrateWhite(irline_sample_t *sample_out){
    if(ir_line_state != IR_LINE_STATE_IDLE){
        return IRLINE_RESULT_BUSY;
    }

    IRLine_EnableEmitter();
    ir_calibration.white_left = ADCLeft;
    ir_calibration.white_right = ADCRight;
    ir_calibration.white_valid = TRUE;
    IRLine_UpdateThresholds();
    IRLine_ShowStatus("CAL WHITE");

    if(sample_out){
        sample_out->left = ir_calibration.white_left;
        sample_out->right = ir_calibration.white_right;
    }

    return IRLINE_RESULT_OK;
}

irline_result_t IRLine_CalibrateBlack(irline_sample_t *sample_out){
    if(ir_line_state != IR_LINE_STATE_IDLE){
        return IRLINE_RESULT_BUSY;
    }

    IRLine_EnableEmitter();
    ir_calibration.black_left = ADCLeft;
    ir_calibration.black_right = ADCRight;
    ir_calibration.black_valid = TRUE;
    IRLine_UpdateThresholds();
    IRLine_ShowStatus("CAL BLACK");

    if(sample_out){
        sample_out->left = ir_calibration.black_left;
        sample_out->right = ir_calibration.black_right;
    }

    return IRLINE_RESULT_OK;
}

irline_result_t IRLine_BeginFollowing(void){
    if(ir_line_state != IR_LINE_STATE_IDLE){
        return IRLINE_RESULT_ALREADY_RUNNING;
    }
    if(!ir_calibration.white_valid){
        return IRLINE_RESULT_NEED_WHITE;
    }
    if(!ir_calibration.black_valid){
        return IRLINE_RESULT_NEED_BLACK;
    }

    Wheels_ForceStop();
    IRLine_ResetTracking();
    IRLine_EnableEmitter();
    IRLine_EnterState(IR_LINE_STATE_SEARCH);
    return IRLINE_RESULT_OK;
}

irline_result_t IRLine_RequestDone(void){
    if(ir_line_state == IR_LINE_STATE_IDLE){
        return IRLINE_RESULT_NOT_RUNNING;
    }
    if(ir_line_state == IR_LINE_STATE_EXIT_DRIVE){
        return IRLINE_RESULT_ALREADY_RUNNING;
    }
    IRLine_EnterState(IR_LINE_STATE_EXIT_DRIVE);
    return IRLINE_RESULT_OK;
}

uint8_t IRLine_IsActive(void){
    return (ir_line_state != IR_LINE_STATE_IDLE) ? 1u : 0u;
}

uint8_t IRLine_IsCalibrated(void){
    return (ir_calibration.white_valid && ir_calibration.black_valid) ? 1u : 0u;
}

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------
static void IRLine_EnableEmitter(void){
    if(IR == OFF){
        IR = ON;
        IRChange = TRUE;
    }
}

static void IRLine_ShowStatus(const char *status){
    char buffer[11];
    unsigned int i;

    for(i = 0; i < 10; i++){
        if(status && status[i]){
            buffer[i] = status[i];
        }else{
            buffer[i] = '\0';
            break;
        }
    }
    buffer[10] = '\0';
    dispPrint(buffer,2);
}

static void IRLine_EnterState(ir_line_state_t next){
    ir_line_state = next;
    ir_line_state_stamp = timer200ms;

    switch(next){
        case IR_LINE_STATE_IDLE:
            IRLine_ResetTracking();
            motorStop();
            IRLine_ShowStatus("IR IDLE");
            break;
        case IR_LINE_STATE_SEARCH:
            IRLine_ShowStatus("IR SEARCH");
            IRLine_SetArcSpeeds();
            break;
        case IR_LINE_STATE_ATTACH_PAUSE:
            motorStop();
            IRLine_ShowStatus("IR HOLD");
            break;
        case IR_LINE_STATE_ATTACH_TURN:
            IRLine_ShowStatus("ATTACH");
            pivot_left_pwm(IR_LINE_ATTACH_SPEED);
            break;
        case IR_LINE_STATE_ALIGN:
            ir_line_align_stable = 0u;
            IRLine_ShowStatus("ALIGN");
            break;
        case IR_LINE_STATE_FOLLOW:
            ir_line_lost_ticks = 0u;
            IRLine_ShowStatus("FOLLOW");
            break;
        case IR_LINE_STATE_EXIT_DRIVE:
            IRLine_ShowStatus("EXIT");
            IRLine_CommandSpeeds(IR_LINE_FOLLOW_BASE_PWM, IR_LINE_FOLLOW_BASE_PWM);
            break;
        default:
            break;
    }
}

static void IRLine_SetArcSpeeds(void){
    IRLine_CommandSpeeds(IR_LINE_SEARCH_LEFT_PWM, IR_LINE_SEARCH_RIGHT_PWM);
}

static void IRLine_CommandSpeeds(unsigned int left_pwm, unsigned int right_pwm){
    unsigned int left = left_pwm;
    unsigned int right = right_pwm;

    if(left > IR_LINE_MAX_PWM){ left = IR_LINE_MAX_PWM; }
    if(right > IR_LINE_MAX_PWM){ right = IR_LINE_MAX_PWM; }

    if(left > 0u && left < MOTOR_LEFT_SAFE_MIN_PWM){
        left = MOTOR_LEFT_SAFE_MIN_PWM;
    }
    if(right > 0u && right < MOTOR_RIGHT_SAFE_MIN_PWM){
        right = MOTOR_RIGHT_SAFE_MIN_PWM;
    }

    set_motor_speeds(left, right);
}

static void IRLine_CommandBalanced(unsigned int base_pwm,
                                   unsigned int left_sample,
                                   unsigned int right_sample,
                                   unsigned char bias_when_lost){
    long error = (long)((int)right_sample - (int)left_sample);
    unsigned long sum = (unsigned long)left_sample + (unsigned long)right_sample;
    if(sum == 0u){
        sum = 1u;
    }

    unsigned int adjusted_base = base_pwm;

    unsigned int target_left = ir_calibration.attach_target_left ? ir_calibration.attach_target_left : ir_calibration.black_left;
    unsigned int target_right = ir_calibration.attach_target_right ? ir_calibration.attach_target_right : ir_calibration.black_right;

    unsigned int left_deficit = 0u;
    unsigned int right_deficit = 0u;
    if(target_left > 0u && left_sample < target_left){
        left_deficit = target_left - left_sample;
    }
    if(target_right > 0u && right_sample < target_right){
        right_deficit = target_right - right_sample;
    }

    if((left_deficit > IR_LINE_DEFICIT_SLOWDOWN_THRESHOLD) ||
       (right_deficit > IR_LINE_DEFICIT_SLOWDOWN_THRESHOLD)){
        if(adjusted_base > IR_LINE_DEFICIT_SPEED_DROP){
            adjusted_base -= IR_LINE_DEFICIT_SPEED_DROP;
        }else{
            adjusted_base = (MOTOR_LEFT_SAFE_MIN_PWM > MOTOR_RIGHT_SAFE_MIN_PWM) ? MOTOR_LEFT_SAFE_MIN_PWM : MOTOR_RIGHT_SAFE_MIN_PWM;
        }
    }

    long delta = (error * IR_LINE_GAIN_NUMERATOR) / (long)sum;
    long attach_delta = ((long)right_deficit - (long)left_deficit) * IR_LINE_ATTACH_DEFICIT_GAIN;
    delta += attach_delta;

    if(bias_when_lost &&
       !IRLine_SensorSeesLine(left_sample, ir_calibration.threshold_left) &&
       !IRLine_SensorSeesLine(right_sample, ir_calibration.threshold_right)){
        if(ir_last_detect_side == IR_SIDE_LEFT){
            delta -= IR_LINE_LOST_BIAS_PWM;
        }else if(ir_last_detect_side == IR_SIDE_RIGHT){
            delta += IR_LINE_LOST_BIAS_PWM;
        }
    }

    long left_pwm = (long)adjusted_base - delta;
    long right_pwm = (long)adjusted_base + delta;

    if(left_pwm < (long)MOTOR_LEFT_SAFE_MIN_PWM){
        left_pwm = MOTOR_LEFT_SAFE_MIN_PWM;
    }
    if(right_pwm < (long)MOTOR_RIGHT_SAFE_MIN_PWM){
        right_pwm = MOTOR_RIGHT_SAFE_MIN_PWM;
    }
    if(left_pwm > IR_LINE_MAX_PWM){
        left_pwm = IR_LINE_MAX_PWM;
    }
    if(right_pwm > IR_LINE_MAX_PWM){
        right_pwm = IR_LINE_MAX_PWM;
    }

    set_motor_speeds((unsigned int)left_pwm, (unsigned int)right_pwm);
}

static void IRLine_HandleSearch(void){
    unsigned int left = ADCLeft;
    unsigned int right = ADCRight;
    uint8_t left_line = IRLine_SensorSeesLine(left, ir_calibration.threshold_left);
    uint8_t right_line = IRLine_SensorSeesLine(right, ir_calibration.threshold_right);

    if(left_line || right_line){
        ir_last_detect_side = left_line ? IR_SIDE_LEFT : IR_SIDE_RIGHT;
        ir_line_lost_ticks = 0u;
        IRLine_EnterState(IR_LINE_STATE_ATTACH_PAUSE);
        return;
    }

    if((timer200ms - ir_line_state_stamp) > IR_LINE_SEARCH_TIMEOUT_TICKS){
        IRLine_ShowStatus("NO LINE");
        IRLine_EnterState(IR_LINE_STATE_IDLE);
    }
}

static void IRLine_HandleAttachPause(void){
    motorStop();
    if((timer200ms - ir_line_state_stamp) >= IR_LINE_ATTACH_PAUSE_TICKS){
        IRLine_EnterState(IR_LINE_STATE_ATTACH_TURN);
    }
}

static void IRLine_HandleAttachTurn(void){
    if((timer200ms - ir_line_state_stamp) >= IR_LINE_ATTACH_TURN_TICKS){
        IRLine_EnterState(IR_LINE_STATE_ALIGN);
    }
}

static void IRLine_HandleAlign(void){
    unsigned int left = ADCLeft;
    unsigned int right = ADCRight;
    int error = (int)right - (int)left;

    IRLine_CommandBalanced(IR_LINE_ALIGN_BASE_PWM, left, right, 0u);

    if((error < IR_LINE_ALIGN_TOLERANCE) && (error > -IR_LINE_ALIGN_TOLERANCE)){
        if(ir_line_align_stable < 0xFFFFu){
            ir_line_align_stable++;
        }
    }else{
        ir_line_align_stable = 0u;
    }

    if(ir_line_align_stable >= IR_LINE_ALIGN_STABLE_TICKS){
        IRLine_EnterState(IR_LINE_STATE_FOLLOW);
    }
}

static void IRLine_HandleFollow(void){
    unsigned int left = ADCLeft;
    unsigned int right = ADCRight;
    uint8_t left_line = IRLine_SensorSeesLine(left, ir_calibration.threshold_left);
    uint8_t right_line = IRLine_SensorSeesLine(right, ir_calibration.threshold_right);

    if(left_line && !right_line){
        ir_last_detect_side = IR_SIDE_LEFT;
    }else if(right_line && !left_line){
        ir_last_detect_side = IR_SIDE_RIGHT;
    }else if(left_line && right_line){
        ir_last_detect_side = IR_SIDE_NONE;
    }

    if(left_line || right_line){
        ir_line_lost_ticks = 0u;
    }else if(ir_line_lost_ticks < 0xFFFFu){
        ir_line_lost_ticks++;
    }

    IRLine_CommandBalanced(IR_LINE_FOLLOW_BASE_PWM, left, right, 1u);
}

static void IRLine_HandleExitDrive(void){
    if((timer200ms - ir_line_state_stamp) >= IR_LINE_EXIT_DURATION_TICKS){
        IRLine_EnterState(IR_LINE_STATE_IDLE);
    }
}

static uint8_t IRLine_SensorSeesLine(unsigned int sample, unsigned int threshold){
    if(!ir_calibration.black_valid){
        return 0u;
    }
    return (sample >= threshold) ? 1u : 0u;
}

static void IRLine_UpdateThresholds(void){
    if(!ir_calibration.black_valid){
        return;
    }

    unsigned int left_margin = IRLine_SelectMargin(ir_calibration.black_left, ir_calibration.white_left);
    unsigned int right_margin = IRLine_SelectMargin(ir_calibration.black_right, ir_calibration.white_right);

    if(ir_calibration.black_left > left_margin){
        ir_calibration.threshold_left = ir_calibration.black_left - left_margin;
    }else{
        ir_calibration.threshold_left = ir_calibration.black_left;
    }

    if(ir_calibration.black_right > right_margin){
        ir_calibration.threshold_right = ir_calibration.black_right - right_margin;
    }else{
        ir_calibration.threshold_right = ir_calibration.black_right;
    }

    ir_calibration.attach_target_left = IRLine_SelectAttachTarget(ir_calibration.black_left);
    ir_calibration.attach_target_right = IRLine_SelectAttachTarget(ir_calibration.black_right);
}

static unsigned int IRLine_SelectMargin(unsigned int black, unsigned int white){
    if(!ir_calibration.white_valid){
        return IR_LINE_THRESHOLD_MARGIN_MIN;
    }

    unsigned int margin = IR_LINE_THRESHOLD_MARGIN_MIN;
    if(black > white){
        unsigned int delta = black - white;
        if(delta < margin){
            margin = (delta > 2u) ? (delta - 1u) : 1u;
        }else if(delta > IR_LINE_THRESHOLD_MARGIN_MAX){
            margin = IR_LINE_THRESHOLD_MARGIN_MAX;
        }
    }

    return margin;
}

static void IRLine_ResetTracking(void){
    ir_line_align_stable = 0u;
    ir_line_lost_ticks = 0u;
    ir_last_detect_side = IR_SIDE_NONE;
}

static unsigned int IRLine_SelectAttachTarget(unsigned int black){
    if(black == 0u){
        return 0u;
    }

    unsigned int offset = IR_LINE_TARGET_OFFSET;
    unsigned int fractional_offset = black / IR_LINE_TARGET_MIN_FRACTION;
    if(fractional_offset == 0u){
        fractional_offset = 1u;
    }
    if(offset > fractional_offset){
        offset = fractional_offset;
    }

    if(offset >= black){
        offset = (black > 1u) ? (black - 1u) : 0u;
    }

    if(offset == 0u){
        return black;
    }

    return black - offset;
}



