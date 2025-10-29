//------------------------------------------------------------------------------
//
//  Description: This file contains the Main Routine - "While" Operating System
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS20.3.0
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#include  "msp430.h"
#include  <string.h>
#include  "LCD.h"
#include  "macros.h"
#include  "ports.h"
#include "functions.h"
#include "serial.h"
#include  "states.h"
#include  "motors.h"

// Global Variables
volatile char slow_input_down;
extern char display_line[4][11];
extern char *display[4];
unsigned char display_mode; //unused
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;
extern volatile unsigned int update_display_count;
extern volatile unsigned int Time_Sequence;
extern volatile unsigned char one_time;
extern unsigned int IR;
extern unsigned int IRChange;
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
extern volatile unsigned int ADCThumb;
unsigned int test_value;
char chosen_direction;
char change;


// New Global Variables for Button Switch & Movement ----------------
unsigned int Switch1_Pressed;
unsigned int old_Time_Sequence;
unsigned int mytime;
unsigned int dir;

unsigned int right_motor_count;
unsigned int left_motor_count;

unsigned int backlight;             // backlight on off flag
unsigned int time_change;
unsigned char dispEvent;            // switch.c track display state
unsigned char state;
unsigned char event;

unsigned int travel_distance;
unsigned int right_count_time;
unsigned int left_count_time;
unsigned int wheel_count_time;

unsigned int delay_start;
unsigned int segment_count;
unsigned int cycle_time;
unsigned int secTime;

// Serial HW8 globals
char ncsu_string[] = "NCSU  #1"; // 2 spaces between U and #
char baud_460800[] = "460,800";
char baud_115200[] = "115,200";

// Local state for serialized baud-change workflow
static unsigned char baud_wait_active = FALSE;
static unsigned int baud_wait_start = 0;
static unsigned char response_wait_active = FALSE;
static unsigned int response_wait_start = 0;

static void lcd_fill_line(char *dest, const char *src){
    unsigned int i;
    unsigned int len = (unsigned int)strlen(src);
    if (len > 10u){
        len = 10u;
    }
    for (i = 0; i < 10u; i++){
        dest[i] = (i < len) ? src[i] : ' ';
    }
    dest[10] = '\0';
}

static void lcd_center_line(char *dest, const char *text){
    unsigned int i;
    unsigned int len = (unsigned int)strlen(text);
    if (len > 10u){
        len = 10u;
    }
    unsigned int offset = (10u - len) / 2u;
    for (i = 0; i < 10u; i++){
        dest[i] = ' ';
    }
    for (i = 0; i < len && (offset + i) < 10u; i++){
        dest[offset + i] = text[i];
    }
    dest[10] = '\0';
}

static void serial_update_display(const char *baud_text){
    lcd_fill_line(display_line[0], "");
    lcd_fill_line(display_line[1], "");
    lcd_center_line(display_line[2], "Baud");
    lcd_center_line(display_line[3], baud_text);
    display_changed = TRUE;
}

static void begin_baud_change(unsigned long baud_value, const char *baud_text){
    Serial_SetBaudAll(baud_value);
    Serial_ClearUCA1Rx();
    serial_update_display(baud_text);
    baud_wait_active = TRUE;
    baud_wait_start = Time_Sequence;
    response_wait_active = FALSE;
}

// Project 7 calibration and timing ----------------
volatile unsigned int time_ticks_200ms;         // 0.2s increments
volatile unsigned char timer_running;           // 1: counting; 0: stopped
volatile unsigned char sw1_press_event;         // SW1 press detected
volatile unsigned char sw2_press_event;         // SW2 press detected (for calibration confirmation)

unsigned int adc_white_left;
unsigned int adc_white_right;
unsigned int adc_black_left;
unsigned int adc_black_right;

unsigned int thresh_black_left;
unsigned int thresh_black_right;
unsigned char left_black_high = TRUE;
unsigned char right_black_high = TRUE;
unsigned int intercept_thresh_left;
unsigned int intercept_thresh_right;
unsigned int intercept_margin_left;
unsigned int intercept_margin_right;
unsigned int intercept_entry_left;
unsigned int intercept_entry_right;
unsigned char intercept_entry_valid = FALSE;

static unsigned int turning_line_ticks = 0;
static unsigned int turning_prev_sequence = 0;

#define INTERCEPT_PHASE_SEEK         (0u)
#define INTERCEPT_PHASE_PAUSE        (1u)
#define INTERCEPT_PHASE_REVERSE      (2u)
#define INTERCEPT_PHASE_PAUSE_AFTER  (3u)
#define INTERCEPT_PHASE_COMPLETE     (4u)

static unsigned char intercept_phase = INTERCEPT_PHASE_SEEK;
static unsigned int intercept_phase_ticks = 0;
static unsigned int intercept_prev_sequence = 0;

// Lap timing (reference-style): exactly two laps based on time
volatile unsigned int lap_ticks_target;  // ticks per lap, mapped from thumbwheel
volatile unsigned int lap_ticks_accum;   // accumulated ticks in current lap
unsigned int laps_completed;             // number of laps completed
unsigned char follow_dir;                       // 'L' for CCW, 'R' for CW (like reference)

//------------------------------------------------------------------------------


void main(void){
    // WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    Init_Ports();                        // Initialize Ports
    Init_Clocks();                       // Initialize Clock System
    Init_Conditions();                   // Initialize Variables and Initial Conditions
    Init_Timers();                       // Initialize Timers
    Init_LCD();                          // Initialize LCD
    Init_ADC();                          // Initialize ADC

    // LCD backlight OFF by default
    backlight = OFF;
    IR = OFF;
    state = IDLE;
    motorStop();

    // Initialize serial ports to 115200 baud
    Serial_InitAll(115200u);

    // Splash screen for 5 seconds
    strcpy(display_line[0], "   NCSU   ");
    strcpy(display_line[1], " WOLFPACK ");
    strcpy(display_line[2], "  ECE306  ");
    strcpy(display_line[3], "  GP I/O  ");
    display_changed = TRUE;
    unsigned int splash_ticks = Time_Sequence;
    while ((unsigned int)(Time_Sequence - splash_ticks) < 25u) { // 5 seconds at 0.2s/tick
        Display_Process();
        Serial_Service();
    }

    // Show baud info
    serial_update_display(baud_115200);

    // Main loop
    while (ALWAYS) {
        Serial_Service();

        if (sw1_press_event) {
            sw1_press_event = 0;
            begin_baud_change(115200u, baud_115200);
        }

        if (sw2_press_event) {
            sw2_press_event = 0;
            begin_baud_change(460800u, baud_460800);
        }

        if (baud_wait_active) {
            if ((unsigned int)(Time_Sequence - baud_wait_start) >= 10u) { // 2 seconds
                Serial_SendStringUCA1(ncsu_string);
                baud_wait_active = FALSE;
                response_wait_active = TRUE;
                response_wait_start = Time_Sequence;
            }
        }

        if (Serial_UCA1LineReady()) {
            char rx_line[SERIAL_RX_LINE_LENGTH];
            Serial_CopyUCA1Line(rx_line, sizeof(rx_line));
            lcd_fill_line(display_line[0], rx_line);
            display_changed = TRUE;
            response_wait_active = FALSE;
        }

        if (response_wait_active) {
            if ((unsigned int)(Time_Sequence - response_wait_start) >= 10u) { // 2 seconds without response
                lcd_center_line(display_line[0], "No Rx");
                display_changed = TRUE;
                response_wait_active = FALSE;
            }
        }

        update();
    }
}

void update(void){
    Display_Process();
    Carlson_StateMachine();
    backlight_update();
    IR_Update();
    P3OUT ^= TEST_PROBE;            // Change State of TEST_PROBE OFF
}

// static unsigned char left_sensor_on_black(void){
//     if(left_black_high){
//         return (ADCLeft >= thresh_black_left);
//     }
//     return (ADCLeft <= thresh_black_left);
// }

// static unsigned char right_sensor_on_black(void){
//     if(right_black_high){
//         return (ADCRight >= thresh_black_right);
//     }
//     return (ADCRight <= thresh_black_right);
// }

// static unsigned char left_sensor_crossed_intercept(void){
//     unsigned int current = ADCLeft;
//     unsigned int threshold = intercept_thresh_left;
//     unsigned int margin = intercept_margin_left ? intercept_margin_left : INTERCEPT_MARGIN_MIN;

//     if(!threshold){
//         threshold = thresh_black_left;
//         if(!threshold && adc_white_left){
//             threshold = adc_white_left + margin;
//         }
//     }

//     if(left_black_high){
//         if(threshold && (current >= threshold)){
//             return TRUE;
//         }
//         if(adc_white_left && (current > adc_white_left) && ((current - adc_white_left) >= margin)){
//             return TRUE;
//         }
//         if(intercept_entry_valid && (current > intercept_entry_left) && ((current - intercept_entry_left) >= margin)){
//             return TRUE;
//         }
//         return FALSE;
//     }

//     if(threshold && (current <= threshold)){
//         return TRUE;
//     }
//     if(adc_white_left && (adc_white_left > current) && ((adc_white_left - current) >= margin)){
//         return TRUE;
//     }
//     if(intercept_entry_valid && (intercept_entry_left > current) && ((intercept_entry_left - current) >= margin)){
//         return TRUE;
//     }
//     return FALSE;
// }

// static unsigned char right_sensor_crossed_intercept(void){
//     unsigned int current = ADCRight;
//     unsigned int threshold = intercept_thresh_right;
//     unsigned int margin = intercept_margin_right ? intercept_margin_right : INTERCEPT_MARGIN_MIN;

//     if(!threshold){
//         threshold = thresh_black_right;
//         if(!threshold && adc_white_right){
//             threshold = adc_white_right + margin;
//         }
//     }

//     if(right_black_high){
//         if(threshold && (current >= threshold)){
//             return TRUE;
//         }
//         if(adc_white_right && (current > adc_white_right) && ((current - adc_white_right) >= margin)){
//             return TRUE;
//         }
//         if(intercept_entry_valid && (current > intercept_entry_right) && ((current - intercept_entry_right) >= margin)){
//             return TRUE;
//         }
//         return FALSE;
//     }

//     if(threshold && (current <= threshold)){
//         return TRUE;
//     }
//     if(adc_white_right && (adc_white_right > current) && ((adc_white_right - current) >= margin)){
//         return TRUE;
//     }
//     if(intercept_entry_valid && (intercept_entry_right > current) && ((intercept_entry_right - current) >= margin)){
//         return TRUE;
//     }
//     return FALSE;
// }


// void Project6(void){
//     switch (state){
//         case IDLE:
//             if(!IRChange){
//                 strcpy(display_line[0], "   IDLE   ");
//             }

//             PWM1_BOTH_OFF(); // motorsOFF();
//             display_changed = TRUE;
//             update_display = TRUE;
//             //        ADC_Update = TRUE; = TRUE;
//             // WAIT state called when SW1 is Pressed
//             break;

//         case WAIT:
//             strcpy(display_line[0], "   WAIT   ");
//             display_changed = TRUE;
//             update_display = TRUE;
//             //        ADC_Update = TRUE; = TRUE;
//             switch (Time_Sequence)
//             {   // Time_Sequence-State_Sequence
//             case 10: // Wait for 1 Second
//                 PWM1_BOTH_OFF(); // motorsOFF();
//                 state = FWD;
//                 break;
//             default:
//                 break;
//             }
//             break;

//         case FWD:
//             PWM1_BOTH_FWD(); // LRFwdON();
//             strcpy(display_line[0], "   FWD    ");
//             display_changed = TRUE;
//             update_display = TRUE;
//             //        ADC_Update = TRUE; = TRUE;
//             if ((ADCLeft >= IR_MAGIC_NUM) && (ADCRight >= IR_MAGIC_NUM)){
//                 state = BLACKLINE; // Black Line Detected
//                 PWM1_BOTH_OFF(); // motorsOFF();
//             }
//             break;

//         case BLACKLINE:
//             PWM1_BOTH_OFF(); // motorsOFF();
//             strcpy(display_line[0], " BLACKLINE");
//             display_changed = TRUE;
//             update_display = TRUE;
//             //        ADC_Update = TRUE; = TRUE;
//             Time_Sequence = 0; //        State_Sequence = Time_Sequence;
//             state = WAIT2;
//             break;

//         case WAIT2:
//             PWM1_BOTH_OFF(); // motorsOFF();
//             if(Time_Sequence < 30){
//                 strcpy(display_line[0], "          ");
//                 strcpy(display_line[1], "Black Line");
//                 strcpy(display_line[2], " Detected ");
//                 strcpy(display_line[3], "          ");
//             }
//             display_changed = TRUE;
//             update_display = TRUE;
//             switch (Time_Sequence){ // Time_Sequence-State_Sequence
//             case 30: // Wait for 3 Second
//                 strcpy(display_line[0], "          ");
//                 strcpy(display_line[1], "          ");
//                 strcpy(display_line[2], "          ");
//                 strcpy(display_line[3], "          ");
//                 display_changed = TRUE;
//                 update_display = TRUE;
//                 PWM1_BOTH_FWD();
//                 if ((ADCLeft < IR_MAGIC_NUM) && (ADCRight < IR_MAGIC_NUM)){
//                     Time_Sequence = 0;
//                     state = TURNL; // Black Line Detected
//                     PWM1_RIGHT_OFF(); // motorsOFF();
//                 }
//                 break;
//             default:
//                 break;
//             }

//             break;

//         case TURNL:
//             PWM1_LEFT_FWD(); // LeftFwdON();
//             strcpy(display_line[0], " TURN LEFT");
//             display_changed = TRUE;
//             update_display = TRUE;
//             //        ADC_Update = TRUE; = TRUE;
//             if ((ADCLeft >= IR_MAGIC_NUM) && (ADCRight >= IR_MAGIC_NUM))
//             {
//                 state = LINE1; // Black Line Detected
//                 PWM1_BOTH_OFF(); // motorsOFF();
//                 Time_Sequence = 0;
//             }
//             break;
//         case LINE1:
//             strcpy(display_line[0], "   LINE1  ");
//             display_changed = TRUE;
//             update_display = TRUE;
//             //        ADC_Update = TRUE; = TRUE;
//             if ((ADCLeft >= IR_MAGIC_NUM) && (ADCRight >= IR_MAGIC_NUM)
//                     && (Time_Sequence >= 10))
//             {
//                 state = DONE; // Black Line Detected
//                 Time_Sequence = 0; //  State_Sequence = Time_Sequence;
//                 PWM1_BOTH_OFF(); // motorsOFF();
//             }
//         case DONE:
//             strcpy(display_line[0], "   DONE   ");
//             display_changed = TRUE;
//             update_display = TRUE;
//             break;

//         default:
//             break;
//     }
// }

// void Project7(void){
//     // Timer display: update line 4 every 200ms (format: "0000.0s")
//     if (update_display) {
//         unsigned int ticks = time_ticks_200ms;
//         unsigned int whole_sec = ticks / 5;  // 5 ticks = 1s
//         unsigned int tenth = (ticks % 5) * 2; // convert 0-4 to 0,2,4,6,8
//         display_line[3][0] = ' ';
//         display_line[3][1] = (whole_sec / 100) % 10 + '0';
//         display_line[3][2] = (whole_sec / 10) % 10 + '0';
//         display_line[3][3] = whole_sec % 10 + '0';
//         display_line[3][4] = '.';
//         display_line[3][5] = tenth + '0';
//         display_line[3][6] = 's';
//         display_line[3][7] = ' ';
//         display_line[3][8] = ' ';
//         display_line[3][9] = ' ';
//     }

//     switch (state) {
//     case IDLE:
//         strcpy(display_line[0], "   IDLE   ");
//         strcpy(display_line[1], " SW1:Start");
//         strcpy(display_line[2], " SW2: IR  ");
//         PWM1_BOTH_OFF();
//         display_changed = TRUE;
//         timer_running = 0;
//         if (sw1_press_event) {
//             sw1_press_event = 0;
//             state = CAL_WHITE;
//             Time_Sequence = 0;
//             IR = ON;  // Turn IR on for calibration
//         }
//         break;

//     case CAL_WHITE:
//         strcpy(display_line[0], " CAL WHT  ");
//         strcpy(display_line[1], " SW2:Next ");
//         // ADC values displayed on lines 2&3 by ADC ISR
//         PWM1_BOTH_OFF();
//         IR = ON;
//         display_changed = TRUE;
        
//         // Wait for SW2 press to confirm white calibration
//         if (sw2_press_event) {
//             sw2_press_event = 0;
//             adc_white_left = ADCLeft;
//             adc_white_right = ADCRight;
//             state = CAL_BLACK;
//             Time_Sequence = 0;
//         }
//         break;

//     case CAL_BLACK:
//         strcpy(display_line[0], " CAL BLK  ");
//         strcpy(display_line[1], " SW2:Next ");
//         // ADC values displayed on lines 2&3 by ADC ISR
//         IR = ON;
//         PWM1_BOTH_OFF();
//         display_changed = TRUE;
        
//         // Wait for SW2 press to confirm black calibration
//         if (sw2_press_event) {
//             sw2_press_event = 0;
//             adc_black_left = ADCLeft;
//             adc_black_right = ADCRight;
//             // Compute thresholds: midpoint between white and black
//             thresh_black_left = (adc_black_left + adc_white_left) / 2;
//             thresh_black_right = (adc_black_right + adc_white_right) / 2;
//             left_black_high = (adc_black_left >= adc_white_left);
//             right_black_high = (adc_black_right >= adc_white_right);

//             unsigned int left_delta = (adc_black_left > adc_white_left) ?
//                                       (adc_black_left - adc_white_left) :
//                                       (adc_white_left - adc_black_left);
//             unsigned int right_delta = (adc_black_right > adc_white_right) ?
//                                        (adc_black_right - adc_white_right) :
//                                        (adc_white_right - adc_black_right);

//             unsigned int left_margin = left_delta / INTERCEPT_MARGIN_DIVISOR;
//             unsigned int right_margin = right_delta / INTERCEPT_MARGIN_DIVISOR;
//             if (left_margin < INTERCEPT_MARGIN_MIN) {
//                 left_margin = (left_delta < INTERCEPT_MARGIN_MIN) ? left_delta : INTERCEPT_MARGIN_MIN;
//             }
//             if (right_margin < INTERCEPT_MARGIN_MIN) {
//                 right_margin = (right_delta < INTERCEPT_MARGIN_MIN) ? right_delta : INTERCEPT_MARGIN_MIN;
//             }

//             if (left_margin == 0) {
//                 left_margin = 1;
//             }
//             if (right_margin == 0) {
//                 right_margin = 1;
//             }

//             if (left_margin > left_delta) {
//                 left_margin = left_delta;
//             }
//             if (right_margin > right_delta) {
//                 right_margin = right_delta;
//             }

//             if (left_black_high) {
//                 intercept_thresh_left = adc_white_left + left_margin;
//                 if (intercept_thresh_left > adc_black_left) {
//                     intercept_thresh_left = adc_black_left;
//                 }
//             } else {
//                 intercept_thresh_left = (adc_white_left > left_margin) ?
//                                          (adc_white_left - left_margin) : 0;
//                 if (intercept_thresh_left < adc_black_left) {
//                     intercept_thresh_left = adc_black_left;
//                 }
//             }

//             if (right_black_high) {
//                 intercept_thresh_right = adc_white_right + right_margin;
//                 if (intercept_thresh_right > adc_black_right) {
//                     intercept_thresh_right = adc_black_right;
//                 }
//             } else {
//                 intercept_thresh_right = (adc_white_right > right_margin) ?
//                                           (adc_white_right - right_margin) : 0;
//                 if (intercept_thresh_right < adc_black_right) {
//                     intercept_thresh_right = adc_black_right;
//                 }
//             }
//             intercept_margin_left = left_margin;
//             intercept_margin_right = right_margin;
//             state = WAIT;
//             Time_Sequence = 0;
//         }
//         break;

//     case WAIT:
//         strcpy(display_line[0], "   WAIT   ");
//         strcpy(display_line[1], "Ready 3s..");
//         PWM1_BOTH_OFF();
//         IR = ON;
//         display_changed = TRUE;
        
//         // Wait 3 seconds before starting
//         if (Time_Sequence >= 15) {  // 15 * 0.2s = 3 seconds
//             state = INTERCEPT;
//             Time_Sequence = 0;
//             time_ticks_200ms = 0;
//             timer_running = 1;
//             // Initialize lap timing targets from thumbwheel
//             laps_completed = 0;
//             lap_ticks_accum = 0;
//             intercept_entry_left = ADCLeft;
//             intercept_entry_right = ADCRight;
//             intercept_entry_valid = TRUE;
//             if(!intercept_margin_left){
//                 intercept_margin_left = INTERCEPT_MARGIN_MIN;
//             }
//             if(!intercept_margin_right){
//                 intercept_margin_right = INTERCEPT_MARGIN_MIN;
//             }
//             intercept_phase = INTERCEPT_PHASE_SEEK;
//             intercept_phase_ticks = 0;
//             intercept_prev_sequence = Time_Sequence;
//         }
//         break;

//     case INTERCEPT:
//         strcpy(display_line[0], "INTERCEPT ");
//         strcpy(display_line[1], "          ");
//         // Display ADC on line 2 & 3 (handled by ADC ISR)
//         IR = ON;
//         display_changed = TRUE;
        
//         switch (intercept_phase) {
//         case INTERCEPT_PHASE_SEEK:
//             PWM1_BOTH_FWD();
//             if (left_sensor_crossed_intercept() || right_sensor_crossed_intercept()) {
//                 PWM1_BOTH_OFF();
//                 intercept_phase = INTERCEPT_PHASE_PAUSE;
//                 intercept_phase_ticks = 0;
//                 intercept_prev_sequence = Time_Sequence;
//             } else {
//                 Time_Sequence = 0;  // keep timer small while seeking
//             }
//             break;

//         default:
//             if (Time_Sequence != intercept_prev_sequence) {
//                 intercept_prev_sequence = Time_Sequence;
//                 if (intercept_phase_ticks < 0xFFFF) {
//                     intercept_phase_ticks++;
//                 }
//             }

//             switch (intercept_phase) {
//             case INTERCEPT_PHASE_PAUSE:
//                 PWM1_BOTH_OFF();
//                 if (intercept_phase_ticks >= INTERCEPT_PAUSE_BEFORE_REV) {
//                     intercept_phase = INTERCEPT_PHASE_REVERSE;
//                     intercept_phase_ticks = 0;
//                 }
//                 break;

//             case INTERCEPT_PHASE_REVERSE:
//                 PWM1_BOTH_REV();
//                 if (intercept_phase_ticks >= INTERCEPT_BACKUP_TICKS) {
//                     intercept_phase = INTERCEPT_PHASE_PAUSE_AFTER;
//                     intercept_phase_ticks = 0;
//                     PWM1_BOTH_OFF();
//                 }
//                 break;

//             case INTERCEPT_PHASE_PAUSE_AFTER:
//                 PWM1_BOTH_OFF();
//                 if (intercept_phase_ticks >= INTERCEPT_PAUSE_BEFORE_TURN) {
//                     intercept_phase = INTERCEPT_PHASE_COMPLETE;
//                 }
//                 break;

//             case INTERCEPT_PHASE_COMPLETE:
//                 PWM1_BOTH_OFF();
//                 state = TURNING;
//                 Time_Sequence = 0;
//                 intercept_phase = INTERCEPT_PHASE_SEEK;
//                 intercept_phase_ticks = 0;
//                 intercept_prev_sequence = 0;
//                 intercept_entry_valid = FALSE;
//                 turning_line_ticks = 0;
//                 turning_prev_sequence = 0;
//                 break;

//             default:
//                 intercept_phase = INTERCEPT_PHASE_SEEK;
//                 intercept_phase_ticks = 0;
//                 intercept_prev_sequence = Time_Sequence;
//                 break;
//             }
//             break;
//         }
//         break;

//     case TURNING:
//         strcpy(display_line[0], " TURNING  ");
//         strcpy(display_line[1], "          ");
//         IR = ON;
//         display_changed = TRUE;
//         // Pivot left until both sensors see black line again (for CW following)
//         // Require continuous detection for ALIGN_CONFIRM_SECONDS to avoid briefly touching the line and missing
//         pivot_left_pwm(TURN_SPEED_PWM);
//         {
//             unsigned char align_on_black = right_sensor_crossed_intercept(); // CW alignment uses right sensor

//             if (Time_Sequence != turning_prev_sequence) {
//                 turning_prev_sequence = Time_Sequence;
//                 if (align_on_black) {
//                     if (turning_line_ticks < 0xFFFF) {
//                         turning_line_ticks++;
//                     }
//                 } else {
//                     turning_line_ticks = 0;
//                 }
//             }

//             if (align_on_black && (turning_line_ticks >= (ALIGN_CONFIRM_SECONDS * TICKS_PER_SECOND))) {
//                 state = CIRCLING;
//                 Time_Sequence = 0;
//                 turning_line_ticks = 0;
//                 turning_prev_sequence = 0;
//                 follow_dir = FOLLOW_DIR_CW;  // Set clockwise direction
//                 // Use fixed lap duration (tuned for two circuits)
//                 unsigned int secs = LAP_SECONDS_FIXED;
//                 lap_ticks_target = secs * TICKS_PER_SECOND;
//                 laps_completed = 0;
//                 lap_ticks_accum = 0;
//             }
//         }
//         break;

//     case CIRCLING: {
//         strcpy(display_line[0], " CIRCLING ");

//         // Discrete ON/OFF steering (reference implementation approach)
//     unsigned char left_on_black = left_sensor_on_black();
//     unsigned char right_on_black = right_sensor_on_black();

//         // Basic discrete steering logic from reference project
//         if (left_on_black && right_on_black) {
//             // Both on black - go straight
//             PWM1_BOTH_FWD();
//         } else if (left_on_black && !right_on_black) {
//             // Left on black, right on white
//             // For CW (right focus): turn toward left sensor (tighten)
//             PWM1_LEFT_FWD();
//             if (follow_dir == FOLLOW_DIR_CW) {
//                 PWM1_RIGHT_OFF();  // Sharp turn for CW
//             } else {
//                 PWM1_RIGHT_FWD();  // Gentle for CCW
//             }
//         } else if (!left_on_black && right_on_black) {
//             // Right on black, left on white
//             // For CW (right focus): this is ideal or gentle correction
//             PWM1_RIGHT_FWD();
//             if (follow_dir == FOLLOW_DIR_CCW) {
//                 PWM1_LEFT_OFF();   // Sharp turn for CCW
//             } else {
//                 PWM1_LEFT_FWD();   // Gentle for CW
//             }
//         } else {
//             // Both off - lost line, bias turn based on follow_dir
//             if (follow_dir == FOLLOW_DIR_CCW) {
//                 PWM1_LEFT_FWD();
//                 PWM1_RIGHT_OFF();
//             } else {
//                 PWM1_RIGHT_FWD();
//                 PWM1_LEFT_OFF();
//             }
//         }

//         // Time-based lap counting (reference style)
//         // Accumulate 0.2s clock ticks into current lap
//         static unsigned int last_clock = 0;
//         if (Time_Sequence == 0){
//             last_clock = time_ticks_200ms; // sync on state entry
//         }
//         if (time_ticks_200ms != last_clock){
//             lap_ticks_accum += (time_ticks_200ms - last_clock);
//             last_clock = time_ticks_200ms;
//         }

//         if (lap_ticks_accum >= lap_ticks_target){
//             laps_completed++;
//             lap_ticks_accum = 0;
//             if (laps_completed >= LAPS_TARGET){
//                 state = EXIT_CENTER;  // exactly two laps
//                 Time_Sequence = 0;
//                 PWM1_BOTH_OFF();
//             }
//         }
        
//         break;
//     }

//     case EXIT_CENTER:
//         strcpy(display_line[0], "EXIT CTR  ");
//         strcpy(display_line[1], "          ");
//         IR = ON;
//         display_changed = TRUE;
//         if (Time_Sequence < EXIT_PIVOT_TICKS) {
//             pivot_left_pwm(TURN_SPEED_PWM);
//         } else if (Time_Sequence < (EXIT_PIVOT_TICKS + EXIT_DRIVE_TICKS)) {
//             set_motor_speeds(BASE_SPEED_PWM, BASE_SPEED_PWM);
//         } else {
//             state = STOPPED;
//             PWM1_BOTH_OFF();
//         }
//         break;

//     case STOPPED:
//         strcpy(display_line[0], " STOPPED  ");
//         strcpy(display_line[1], "          ");
//         PWM1_BOTH_OFF();
//         IR = OFF;
//         timer_running = 0;
//         display_changed = TRUE;
//         break;

//     default:
//         break;
//     }
// }


