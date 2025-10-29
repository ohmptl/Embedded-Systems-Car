//------------------------------------------------------------------------------
//  Name:           serial.h
//  Description:    Header for serial functions (serial.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#ifndef SERIAL_H_
#define SERIAL_H_


// RX line length chosen to fit LCD width (10 chars) plus terminator
#define SERIAL_RX_LINE_LENGTH      (11u)

void Serial_InitAll(unsigned long baud);
void Serial_SetBaudAll(unsigned long baud);
unsigned long Serial_GetCurrentBaud(void);

void Serial_SendStringUCA0(const char *str);
void Serial_SendStringUCA1(const char *str);

void Serial_ClearUCA1Rx(void);
unsigned char Serial_UCA1LineReady(void);
void Serial_CopyUCA1Line(char *dest, unsigned int dest_len);
void Serial_Service(void);

// Called from UART ISRs
void Serial_HandleUCA0Rx(char byte_in);
void Serial_HandleUCA1Rx(char byte_in);

#endif // SERIAL_H_

