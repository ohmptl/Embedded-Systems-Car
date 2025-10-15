//------------------------------------------------------------------------------
//  Name:           timers.h
//  Description:    Header for timers (timersB0.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS
//------------------------------------------------------------------------------

#ifndef TIMERSB0_H_
#define TIMERSB0_H_

// Init
void Init_Timers(void);
void Init_Timer_B0(void);
void Init_Timer_B1(void);
void Init_Timer_B2(void);
void Init_Timer_B3(void);

// Helpers
void usleep(unsigned int usec);
void usleep10(unsigned int usec10);
void five_msec_sleep(unsigned int msec5);
void measure_delay(void);
void out_control_words(void);

void half_sec_delay(void);

// Debounce globals (defined in timersB0.c)
extern volatile unsigned int debounce_count1;
extern volatile unsigned int debounce_count2;
extern volatile unsigned char debounce_active1;
extern volatile unsigned char debounce_active2;

// Shared timing macros relevant to HW06
// These MUST match the configuration in timersB0.c
#define TB0_SMCLK_HZ            (8000000u)
#define TB0_ID_DIV              (8u)     // ID__8
#define TB0_EX_DIV              (8u)     // TBIDEX_7
#define TB0_TICK_HZ             (TB0_SMCLK_HZ / TB0_ID_DIV / TB0_EX_DIV) // 125kHz

#define BACKLIGHT_INTERVAL_MS   (200u)   // CCR0 interval
#define CCR0_DELTA_COUNTS       ((TB0_TICK_HZ * BACKLIGHT_INTERVAL_MS) / 1000u)

#define DEBOUNCE_TICK_MS        (50u)    // CCR1/CCR2 interval
#define CCRx_DEBOUNCE_DELTA     ((TB0_TICK_HZ * DEBOUNCE_TICK_MS) / 1000u)
//cner
#define TB0CCR0_INTERVAL (25000) // 8,000,000 / 8 / 8 / (1000 / 200msec)
    // Others
#define TB0CCR1_INTERVAL (25000) // 8,000,000 / 8 / 8 / (1000 / 200msec)
#define TB0CCR2_INTERVAL (25000) // 8,000,000 / 8 / 8 / (1000 / 200msec)

    // Timer B3
#define TB3CCR0_INTERVAL (5000)


#define TIMER_B0_0_VECTOR (TIMER0_B0_VECTOR)
#define TIMER_B0_1_OVFL_VECTOR (TIMER0_B1_VECTOR)

#define TIMER_B3_0_VECTOR (TIMER0_B3_VECTOR)


#endif /* TIMERSB0_H_ */




