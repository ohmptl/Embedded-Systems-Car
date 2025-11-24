//------------------------------------------------------------------------------
//
//  Description: This file contains the function prototypes (central reference)
//
//  Ohm Patel
//  Sept 2025
//  Built with Code Composer Version: CCS12.8.1
//
//------------------------------------------------------------------------------

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdint.h>

//------------------------------------------------------------------------------
// main.c
//------------------------------------------------------------------------------
void main(void);
void update(void);

//------------------------------------------------------------------------------
// init.c
//------------------------------------------------------------------------------
void Init_Conditions(void);

//------------------------------------------------------------------------------
// system.c
//------------------------------------------------------------------------------
void enable_interrupts(void);
 
//------------------------------------------------------------------------------
// timersB0.c
//------------------------------------------------------------------------------
__interrupt void Timer0_B0_ISR(void);
__interrupt void TIMER0_B1_ISR(void);
// Timer init and helpers
void Init_Timers(void);
void Init_Timer_B0(void);
void Init_Timer_B1(void);
void Init_Timer_B2(void);
void Init_Timer_B3(void);

void five_msec_sleep(unsigned int msec);
void sleep(unsigned int sec);

//------------------------------------------------------------------------------
// switch.c
//------------------------------------------------------------------------------
__interrupt void Port4_ISR(void);
__interrupt void Port2_ISR(void);
__interrupt void switch_interrupt(void);

void enable_switch_SW1(void);
void enable_switch_SW2(void);
void enable_switches(void);
void disable_switch_SW1(void);
void disable_switch_SW2(void);
void disable_switches(void);

//------------------------------------------------------------------------------
// ADC.c
//------------------------------------------------------------------------------
void Init_ADC(void);
void HEXtoBCD(int hex_value);
void adc_line(char line, char location);

//------------------------------------------------------------------------------
// clocks.c
//------------------------------------------------------------------------------
void Init_Clocks(void);

//------------------------------------------------------------------------------
// led.c
//------------------------------------------------------------------------------
void Init_LEDs(void);
void IR_LED_control(char selection);
void backlight_update(void);

//------------------------------------------------------------------------------
// display.c
//------------------------------------------------------------------------------
void Display_Process(void);

//------------------------------------------------------------------------------
// LCD.c
//------------------------------------------------------------------------------
void Display_Update(char p_L1,char p_L2,char p_L3,char p_L4);
void Display_Process(void);
void enable_display_update(void);
void update_string(char *string_data, int string);
void Init_LCD(void);
void lcd_clear(void);
void lcd_putc(char c);
void lcd_puts(char *s);

void lcd_power_on(void);
void lcd_write_line1(void);
void lcd_write_line2(void);
//void lcd_draw_time_page(void);
//void lcd_power_off(void);
void lcd_enter_sleep(void);
void lcd_exit_sleep(void);
//void lcd_write(unsigned char c);
//void out_lcd(unsigned char c);

void Write_LCD_Ins(char instruction);
void Write_LCD_Data(char data);
void ClrDisplay(void);
void ClrDisplay_Buffer_0(void);
void ClrDisplay_Buffer_1(void);
void ClrDisplay_Buffer_2(void);
void ClrDisplay_Buffer_3(void);

void SetPostion(char pos);
void DisplayOnOff(char data);
void lcd_BIG_mid(void);
void lcd_BIG_bot(void);
void lcd_120(void);

void lcd_4line(void);
void lcd_out(char *s, char line, char position);
void lcd_rotate(char view);

//void lcd_write(char data, char command);
void lcd_write(unsigned char c);
void lcd_write_line1(void);
void lcd_write_line2(void);
void lcd_write_line3(void);

void lcd_command( char data);
void LCD_test(void);
void LCD_iot_meassage_print(int nema_index);

//------------------------------------------------------------------------------
// stateMachine.c
//------------------------------------------------------------------------------
void StateMachine(void);
void bootSequence(void);

//------------------------------------------------------------------------------
// ports.c
//------------------------------------------------------------------------------
void Init_Ports(void);
void Init_Port1(void);
void Init_Port2(void);
//void Init_Port3(char smclk);
void Init_Port3(void);
void Init_Port4(void);
void Init_Port5(void);
void Init_Port6(void);

//------------------------------------------------------------------------------
// LCD.c (SPI helpers)
//------------------------------------------------------------------------------
void Init_SPI_B1(void);
void SPI_B1_write(char byte);
void spi_rs_data(void);
void spi_rs_command(void);
void spi_LCD_idle(void);
void spi_LCD_active(void);
void SPI_test(void);
void WriteIns(char instruction);
void WriteData(char data);

//------------------------------------------------------------------------------
// IR.c
//------------------------------------------------------------------------------
void IR_Update(void);

//------------------------------------------------------------------------------
// motors.c
//------------------------------------------------------------------------------
void wait_case();
void start_case(void);
void run_case(void);
void end_case(void);

void Move_Shape(void);

void secondTimer(int seconds);

void Wheels_Process(void);

void motorStop(void);
void safetyCheck(void);
void set_motor_speeds(unsigned int left_pwm, unsigned int right_pwm);
void pivot_left_pwm(unsigned int speed);
void pivot_right_pwm(unsigned int speed);
uint8_t Motor_ApplyJoystickVector(int16_t x_axis, int16_t y_axis, unsigned char engaged,
								 unsigned int *applied_left_pwm, unsigned int *applied_right_pwm);
void Motor_JoystickFailsafeService(void);

// PWM1 motor control functions
void PWM1_LEFT_FWD(void);
void PWM1_RIGHT_FWD(void);
void PWM1_BOTH_FWD(void);
void PWM1_LEFT_OFF(void);
void PWM1_RIGHT_OFF(void);
void PWM1_BOTH_OFF(void);
void PWM1_LEFT_REV(void);
void PWM1_RIGHT_REV(void);
void PWM1_BOTH_REV(void);

//------------------------------------------------------------------------------
#endif // FUNCTIONS_H_



