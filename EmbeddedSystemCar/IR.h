//------------------------------------------------------------------------------
//  Name:           IR.h
//  Description:    Header for Infrared sensor/LED control (IR.c)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#ifndef IR_H_
#define IR_H_

#include <stdint.h>

//------------------------------------------------------------------------------
// Result/status codes for IR line navigation commands
//------------------------------------------------------------------------------
typedef enum {
	IRLINE_RESULT_OK = 0,
	IRLINE_RESULT_BUSY,
	IRLINE_RESULT_NEED_WHITE,
	IRLINE_RESULT_NEED_BLACK,
	IRLINE_RESULT_NOT_RUNNING,
	IRLINE_RESULT_ALREADY_RUNNING
} irline_result_t;

typedef struct {
	unsigned int left;
	unsigned int right;
} irline_sample_t;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------
void IR_Update(void);
void IRLine_Init(void);
void IRLine_Service(void);
irline_result_t IRLine_CalibrateWhite(irline_sample_t *sample_out);
irline_result_t IRLine_CalibrateBlack(irline_sample_t *sample_out);
irline_result_t IRLine_BeginFollowing(void);
irline_result_t IRLine_RequestDone(void);
void IRLine_ForceStop(void);
uint8_t IRLine_IsActive(void);
uint8_t IRLine_IsCalibrated(void);

#endif // IR_H_
