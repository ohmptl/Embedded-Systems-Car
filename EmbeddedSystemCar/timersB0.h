









#ifndef TIMERSB0_H_
#define TIMERSB0_H_

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

#endif /* TIMERSB0_H_ */




