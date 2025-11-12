//------------------------------------------------------------------------------
//  Name:           serial.h
//  Description:    Header for serial functions (serial.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

typedef struct {
	char direction;       // 'F', 'B', 'L', 'R', etc.
	uint16_t duration;    // Scheduler ticks (0.2s each) computed from incoming command
} serial_motion_command_t;

void Serial_Project9_Init(void);
void Serial_Project9_Service(void);

// Helpers for local button-triggered queries / maintenance
void Serial_RequestWifiStatus(void);
void Serial_RequestIpAddress(void);
void Serial_ResetIotModule(void);
void Serial_SendIotCommand(const char *command);
void Serial_ShowWifiStatusScreen(void);

// Command retrieval for higher-level control
uint8_t Serial_DequeueMotionCommand(serial_motion_command_t *out_command);
uint8_t Serial_HostReady(void);

#define BEGINNING            (0)
#define SMALL_RING_SIZE      (128)
#define LARGE_RING_SIZE      (256)

#endif // SERIAL_H_

