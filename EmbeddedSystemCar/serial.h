//------------------------------------------------------------------------------
//  Name:           serial.h
//  Description:    Header for serial functions (serial.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#ifndef SERIAL_H_
#define SERIAL_H_

// UART Initialization Functions
void Init_Serial_UCA0(char speed);
void Init_Serial_UCA1(char speed);
void USCI_A0_transmit(void);
void USCI_A1_transmit(void);
void IOT_Process(void);

// Convenience helpers for HW08
void UCA1_SendString(const char *s);
void Serial_Process_USB_RX(void);
void Serial_Process_IOT_RX(void);

// Shared TX buffer used by A0 TX ISR
extern char process_buffer[25];
extern char pb_index;

#define BEGINNING            (0)
#define SMALL_RING_SIZE (16)
#define LARGE_RING_SIZE (32)
#define CHARACTER       (0)

#endif // SERIAL_H_

