//------------------------------------------------------------------------------
//  Name:           serial.c
//  Description:    Serial support for Project 09 (IOT communication)
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include "ports.h"
#include "msp430.h"
#include "macros.h"
#include "serial.h"
#include "display.h"
#include "functions.h"
#include "wheels.h"
#include "motors.h"
#include "IR.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define SERIAL_HEARTBEAT_PERIOD_TICKS      (5u)   // ~1 second @ 0.2s tick
#define SERIAL_BIG_DISPLAY_HOLD_TICKS      (25u)  // ~5 seconds (increased from 5 to match wifi timeout)
#define SERIAL_WIFI_DISPLAY_HOLD_TICKS     (25u)  // ~5 seconds
#define SERIAL_MAX_CMD_LEN                 (32u)
#define SERIAL_MAX_IOT_LINE_LEN            (96u)
#define SERIAL_MAX_HTTP_PAYLOAD            (512u)
#define SERIAL_HTTP_HEADER_MAX             (256u)
#define SERIAL_AUTH_PIN                    "2005"
#define SERIAL_AUTH_PIN_LEN                ((unsigned char)(sizeof(SERIAL_AUTH_PIN) - 1u))
#define SERIAL_TURN_DEGREES_PER_TICK       (18u)  // Approx. 90 deg/s at 0.2s per tick
#define SERIAL_MAX_TURN_DEGREES            (360u) // Prevent spinning beyond a single rotation
#define SERIAL_SERVER_SETUP_DELAY_TICKS    (5u)   // ~1 second delay between staged commands

typedef enum {
	SERIAL_BAUD_FAST = 1u,
	SERIAL_BAUD_SLOW = 2u
} serial_baud_t;

volatile unsigned int usb_rx_ring_wr = 0;
volatile unsigned int usb_rx_ring_rd = 0;
volatile unsigned int iot_rx_wr = 0;
volatile unsigned int iot_rx_rd = 0;

volatile char USB_Ring_Rx[SMALL_RING_SIZE];
volatile char IOT_Ring_Rx[LARGE_RING_SIZE];

static serial_baud_t current_iot_baud = SERIAL_BAUD_FAST;
static unsigned char pc_link_open = 0;

static char command_buffer[SERIAL_MAX_CMD_LEN];
static unsigned char command_length = 0;
static unsigned char command_active = 0;

static unsigned int last_heartbeat_stamp = 0;

static char wifi_ssid[11];
static unsigned char wifi_ssid_valid = 0;
static char wifi_ip[16];
static unsigned char wifi_ip_valid = 0;

static char iot_line_buffer[SERIAL_MAX_IOT_LINE_LEN];
static unsigned char iot_line_length = 0;

static unsigned char big_display_active = 0;
static unsigned int big_display_timestamp = 0;

static serial_motion_command_t pending_motion_command;
static volatile unsigned char motion_command_pending = 0;

typedef enum {
	SERIAL_DISPLAY_WIFI = 0,
	SERIAL_DISPLAY_WAITING,
	SERIAL_DISPLAY_BIG
} serial_display_mode_t;

static serial_display_mode_t current_display_mode = SERIAL_DISPLAY_WIFI;
static unsigned int display_mode_timestamp = 0u;

static unsigned char server_configured = 0u;
static unsigned char server_setup_pending = 0u;
static unsigned char server_setup_stage = 0u;
static unsigned int server_setup_timestamp = 0u;

static unsigned char ipd_active = 0u;
static unsigned char ipd_link_id = 0u;
static unsigned int ipd_expected_length = 0u;
static unsigned int ipd_bytes_captured = 0u;
static unsigned char ipd_overflow = 0u;
static char ipd_payload_buffer[SERIAL_MAX_HTTP_PAYLOAD + 1u];

extern volatile unsigned int timer200ms;
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;

// Forward declarations -------------------------------------------------------
static void Serial_ResetRings(void);
static void Serial_InitUCA0(serial_baud_t speed);
static void Serial_InitUCA1(void);
static void Serial_WritePcChar(char c);
static void Serial_WritePcString(const char *s);
static void Serial_WritePcLine(const char *s);
static void Serial_WritePcSamplePair(const char *prefix, unsigned int left_value, unsigned int right_value);
static void Serial_WriteIotChar(char c);
static void Serial_ProcessUsbChar(char c);
static void Serial_FinalizeCommand(void);
static void Serial_ProcessCommandBuffer(char *buffer, unsigned char length);
static void Serial_ProcessCommandString(const char *buffer, unsigned int length);
static void Serial_HandleCommand(const char *cmd, unsigned char len);
static void Serial_HandleAuthorizedCommand(const char *payload, unsigned int length);
static void Serial_SetIotBaud(serial_baud_t mode);
static void Serial_ShowBaudOnDisplay(serial_baud_t mode);
static void Serial_ProcessIotChar(char c);
static void Serial_ProcessIotLine(const char *line);
static void Serial_ParseWifiSsidLine(const char *line);
static void Serial_ParseWifiIpLine(const char *line);
static void Serial_HandleWifiReady(void);
static void Serial_ShowWifiScreen(void);
static void Serial_RefreshWifiScreen(void);
static void Serial_ShowWaitingScreen(void);
static void Serial_UpdateIrAdcLine(void);
static void Serial_FormatFourDigit(char *dest, unsigned int value);
static void Serial_DisplayModeService(void);
static void Serial_ServiceHeartbeat(void);
static void Serial_ShowBigText(const char *text);
static void Serial_ShowBigCommand(char direction, unsigned int duration);
static unsigned int Serial_ParseUnsigned(const char *s, unsigned char len);
static void Serial_SendIotString(const char *command);
static void Serial_FormatIpForDisplay(char *line3, char *line4);
static uint8_t Serial_ConvertDurationToTicks(char direction, unsigned int raw_value, unsigned int *out_ticks);
static void Serial_HandleWifiConnected(void);
static void Serial_ServiceServerSetup(void);
static void Serial_HandleIpdLine(const char *line);
static void Serial_BeginIpdCapture(unsigned char link_id, unsigned int payload_length, const char *initial_payload);
static void Serial_CaptureIpdByte(char c);
static void Serial_FinalizeIpdCapture(void);
static void Serial_DispatchIpdPayload(unsigned char link_id, const char *payload, unsigned int length, unsigned char truncated);
static uint8_t Serial_IsHttpRequest(const char *payload);
static void Serial_HandleHttpRequest(unsigned char link_id, char *payload, unsigned int length);
static void Serial_SendHttpResponse(unsigned char link_id, const char *status_line, const char *content_type, const char *body);
static void Serial_SendHttpJson(unsigned char link_id, const char *status_line, const char *json_body);
static void Serial_SendHttpNoContent(unsigned char link_id);
static void Serial_CloseHttpSocket(unsigned char link_id);
static void Serial_WriteIotBuffer(const char *data, unsigned int length);
static void Serial_HandleJoystickApi(unsigned char link_id, const char *headers, const char *body, unsigned int body_length);
static void Serial_HandleIrApi(unsigned char link_id, const char *headers, const char *body, unsigned int body_length);
static unsigned int Serial_ParseContentLength(const char *headers);
static uint8_t Serial_HasFormContentType(const char *headers);
static uint8_t Serial_ParseFormInt(const char *body, const char *key, int *value_out);
static void Serial_HandleHttpHealth(unsigned char link_id);
static void Serial_Utoa(unsigned int value, char *buffer, unsigned int buffer_len);

//------------------------------------------------------------------------------
//  Public API
//------------------------------------------------------------------------------

void Serial_Project9_Init(void) {
	Serial_ResetRings();

	pc_link_open = 0;
	command_active = 0;
	command_length = 0;
	wifi_ssid_valid = 0;
	wifi_ip_valid = 0;
	iot_line_length = 0;
	big_display_active = 0;
	motion_command_pending = 0;

	current_iot_baud = SERIAL_BAUD_FAST;
	Serial_InitUCA0(current_iot_baud);
	Serial_InitUCA1();

	P3OUT |= IOT_EN_CPU;   // ensure module is released from reset
	last_heartbeat_stamp = timer200ms;
	server_configured = 0u;
	server_setup_pending = 0u;
	server_setup_stage = 0u;
	server_setup_timestamp = timer200ms;
	Serial_ShowWifiScreen();
}

void Serial_Project9_Service(void) {
	while (usb_rx_ring_rd != usb_rx_ring_wr) {
		char c = USB_Ring_Rx[usb_rx_ring_rd++];
		if (usb_rx_ring_rd >= SMALL_RING_SIZE) {
			usb_rx_ring_rd = BEGINNING;
		}
		Serial_ProcessUsbChar(c);
	}

	while (iot_rx_rd != iot_rx_wr) {
		char c = IOT_Ring_Rx[iot_rx_rd++];
		if (iot_rx_rd >= LARGE_RING_SIZE) {
			iot_rx_rd = BEGINNING;
		}
		Serial_ProcessIotChar(c);
	}

	Serial_ServiceHeartbeat();
	Serial_DisplayModeService();
	Serial_ServiceServerSetup();
	Motor_JoystickFailsafeService();
}

void Serial_RequestWifiStatus(void) {
	Serial_SendIotCommand("AT+CWJAP?");
	Serial_ShowWifiScreen();
}

void Serial_RequestIpAddress(void) {
	Serial_SendIotCommand("AT+CIFSR");
	Serial_ShowWifiScreen();
}

void Serial_ResetIotModule(void) {
	P3OUT &= ~IOT_EN_CPU;
	five_msec_sleep(20);         // ~100ms
	P3OUT |= IOT_EN_CPU;
}

void Serial_ShowWifiStatusScreen(void) {
	Serial_ShowWifiScreen();
}

void Serial_SendIotCommand(const char *command) {
	if (!command || !*command) {
		return;
	}
	Serial_SendIotString(command);
	Serial_WritePcString(">> ");
	Serial_WritePcLine(command);
}

uint8_t Serial_DequeueMotionCommand(serial_motion_command_t *out_command) {
	if (!motion_command_pending || !out_command) {
		return 0u;
	}
	out_command->direction = pending_motion_command.direction;
	out_command->duration  = pending_motion_command.duration;
	motion_command_pending = 0u;
	return 1u;
}

uint8_t Serial_HostReady(void) {
	return pc_link_open;
}

//------------------------------------------------------------------------------
//  Private helpers
//------------------------------------------------------------------------------

static void Serial_ResetRings(void) {
	unsigned int i;
	for (i = 0; i < SMALL_RING_SIZE; i++) {
		USB_Ring_Rx[i] = 0;
	}
	usb_rx_ring_wr = BEGINNING;
	usb_rx_ring_rd = BEGINNING;

	for (i = 0; i < LARGE_RING_SIZE; i++) {
		IOT_Ring_Rx[i] = 0;
	}
	iot_rx_wr = BEGINNING;
	iot_rx_rd = BEGINNING;
}

static void Serial_InitUCA0(serial_baud_t speed) {
	UCA0CTLW0 = UCSWRST;
	UCA0CTLW0 |= UCSSEL__SMCLK;
	UCA0CTLW0 &= ~(UCMSB | UCSPB | UCPEN | UCSYNC | UC7BIT);
	UCA0CTLW0 |= UCMODE_0;

	switch (speed) {
		case SERIAL_BAUD_SLOW:
			UCA0BRW = 52u;
			UCA0MCTLW = 0x4911u;
			break;
		case SERIAL_BAUD_FAST:
		default:
			UCA0BRW = 4u;
			UCA0MCTLW = 0x5551u;
			break;
	}

	UCA0CTLW0 &= ~UCSWRST;
	UCA0IE |= UCRXIE;
}

static void Serial_InitUCA1(void) {
	UCA1CTLW0 = UCSWRST;
	UCA1CTLW0 |= UCSSEL__SMCLK;
	UCA1CTLW0 &= ~(UCMSB | UCSPB | UCPEN | UCSYNC | UC7BIT);
	UCA1CTLW0 |= UCMODE_0;

	UCA1BRW = 4u;
	UCA1MCTLW = 0x5551u;

	UCA1CTLW0 &= ~UCSWRST;
	UCA1IE |= UCRXIE;
}

static void Serial_WritePcChar(char c) {
	if (!pc_link_open) {
		return;
	}
	while (!(UCA1IFG & UCTXIFG)) {
		;
	}
	UCA1TXBUF = c;
}

static void Serial_WritePcString(const char *s) {
	if (!pc_link_open || !s) {
		return;
	}
	while (*s) {
		Serial_WritePcChar(*s++);
	}
}

static void Serial_WritePcLine(const char *s) {
	Serial_WritePcString(s);
	Serial_WritePcChar('\r');
	Serial_WritePcChar('\n');
}

static void Serial_WritePcSamplePair(const char *prefix, unsigned int left_value, unsigned int right_value) {
	char left_str[8];
	char right_str[8];

	Serial_Utoa(left_value, left_str, sizeof(left_str));
	Serial_Utoa(right_value, right_str, sizeof(right_str));

	Serial_WritePcString(prefix);
	Serial_WritePcString(" L:");
	Serial_WritePcString(left_str);
	Serial_WritePcString(" R:");
	Serial_WritePcString(right_str);
	Serial_WritePcChar('\r');
	Serial_WritePcChar('\n');
}

static uint8_t Serial_CommandIsWhitespace(char c) {
	return (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t');
}

static void Serial_ProcessCommandBuffer(char *buffer, unsigned char length) {
	if (!buffer) {
		Serial_WritePcLine("ERR empty command");
		return;
	}

	unsigned char len = length;
	while (len > 0u && Serial_CommandIsWhitespace(buffer[len - 1u])) {
		buffer[len - 1u] = '\0';
		len--;
	}

	unsigned char start = 0u;
	while ((start < len) && Serial_CommandIsWhitespace(buffer[start])) {
		start++;
	}

	if (start > 0u && len > start) {
		memmove(buffer, buffer + start, len - start);
		len -= start;
		buffer[len] = '\0';
	} else if (start > 0u && len == start) {
		len = 0u;
		buffer[0] = '\0';
	}

	if (len == 0u) {
		Serial_WritePcLine("ERR empty command");
		return;
	}

	Serial_HandleCommand(buffer, len);
}

static void Serial_ProcessCommandString(const char *buffer, unsigned int length) {
	if (!buffer || length == 0u) {
		Serial_WritePcLine("ERR empty command");
		return;
	}

	unsigned int copy_len = length;
	if (copy_len > SERIAL_MAX_CMD_LEN) {
		copy_len = SERIAL_MAX_CMD_LEN;
	}

	char local_buf[SERIAL_MAX_CMD_LEN + 1u];
	memcpy(local_buf, buffer, copy_len);
	local_buf[copy_len] = '\0';

	Serial_ProcessCommandBuffer(local_buf, (unsigned char)copy_len);
}

static void Serial_WriteIotChar(char c) {
	while (!(UCA0IFG & UCTXIFG)) {
		;
	}
	UCA0TXBUF = c;
}

static void Serial_ProcessUsbChar(char c) {
	if (!pc_link_open) {
		pc_link_open = 1u;
		Serial_WritePcLine("FRAM link opened");
		last_heartbeat_stamp = timer200ms;
	}

	if (c == '\0') {
		return;
	}

	if (!command_active) {
		if (c == '^') {
			command_active = 1u;
			command_length = 0u;
			return;
		}
		Serial_WriteIotChar(c);
		return;
	}

	if ((c == '\r') || (c == '\n')) {
		Serial_FinalizeCommand();
		command_active = 0u;
		command_length = 0u;
		return;
	}

	if (command_length < SERIAL_MAX_CMD_LEN) {
		command_buffer[command_length++] = c;
	} else {
		command_active = 0u;
		command_length = 0u;
		Serial_WritePcLine("ERR command too long");
	}
}

static void Serial_FinalizeCommand(void) {
	if (command_length == 0u) {
		Serial_WritePcLine("ERR empty command");
		return;
	}

	char buffer[SERIAL_MAX_CMD_LEN + 1u];
	unsigned char i;
	for (i = 0; i < command_length; i++) {
		buffer[i] = command_buffer[i];
	}
	buffer[command_length] = '\0';

	Serial_ProcessCommandBuffer(buffer, command_length);
}

static void Serial_HandleCommand(const char *cmd, unsigned char len) {
	if (!cmd || len == 0u) {
		return;
	}

	if (len == 1u) {
		char key = (char)toupper((unsigned char)cmd[0]);
		switch (key) {
			case '^':
				Serial_WritePcLine("I'm here");
				return;
			case 'F':
				Serial_SetIotBaud(SERIAL_BAUD_FAST);
				Serial_WritePcLine("115200");
				return;
			case 'S':
				Serial_SetIotBaud(SERIAL_BAUD_SLOW);
				Serial_WritePcLine("9600");
				return;
			default:
				break;
		}
	}

	if (len >= (SERIAL_AUTH_PIN_LEN + 1u)) {
		if (strncmp(cmd, SERIAL_AUTH_PIN, SERIAL_AUTH_PIN_LEN) == 0) {
			const char *payload = cmd + SERIAL_AUTH_PIN_LEN;
			unsigned int remaining = len - SERIAL_AUTH_PIN_LEN;
			Serial_HandleAuthorizedCommand(payload, remaining);
			return;
		}
	}

	Serial_WritePcLine("ERR unknown command");
}

static void Serial_SetIotBaud(serial_baud_t mode) {
	if (mode != current_iot_baud) {
		current_iot_baud = mode;
		Serial_InitUCA0(current_iot_baud);
	}
	Serial_ShowBaudOnDisplay(current_iot_baud);
}

static void Serial_HandleAuthorizedCommand(const char *payload, unsigned int length) {
	if (!payload || length == 0u) {
		Serial_WritePcLine("ERR missing opcode");
		return;
	}

	char opcode = (char)toupper((unsigned char)payload[0]);
	const char *args = payload + 1u;
	unsigned int args_len = (length > 0u) ? (length - 1u) : 0u;

	switch (opcode) {
		case 'F':
		case 'B':
		case 'L':
		case 'R': {
			if (IRLine_IsActive()) {
				Serial_WritePcLine("ERR IR active");
				return;
			}
			if (args_len == 0u) {
				Serial_WritePcLine("ERR missing duration");
				return;
			}
			unsigned char digit_len = (unsigned char)args_len;
			unsigned int raw_value = Serial_ParseUnsigned(args, digit_len);
			if ((raw_value == 0u) || (raw_value > 9999u)) {
				Serial_WritePcLine("ERR invalid duration");
				return;
			}
			if ((opcode == 'L' || opcode == 'R') && (raw_value > SERIAL_MAX_TURN_DEGREES)) {
				Serial_WritePcLine("ERR turn angle");
				return;
			}
			unsigned int duration_ticks = 0u;
			if (!Serial_ConvertDurationToTicks(opcode, raw_value, &duration_ticks)) {
				Serial_WritePcLine("ERR duration range");
				return;
			}
			pending_motion_command.direction = opcode;
			pending_motion_command.duration  = (uint16_t)duration_ticks;
			motion_command_pending = 1u;
			Serial_WritePcLine("CMD accepted");
			Serial_ShowBigCommand(opcode, raw_value);
			return;
		}
		case 'S': {
			if (args_len != 0u) {
				Serial_WritePcLine("ERR stop syntax");
				return;
			}
            // Emergency Stop: Kill IR loop if active
            IRLine_ForceStop();
            
			pending_motion_command.direction = 'S';
			pending_motion_command.duration  = 0u;
			motion_command_pending = 1u;
			Serial_WritePcLine("CMD stop");
			Serial_ShowBigCommand('S', 0u);
			return;
		}
		case 'Q': {
			if (args_len != 0u) {
				Serial_WritePcLine("ERR IR syntax");
				return;
			}
			irline_sample_t sample;
			irline_result_t result = IRLine_CalibrateBlack(&sample);
			if (result == IRLINE_RESULT_OK) {
				Serial_WritePcSamplePair("IR BLACK", sample.left, sample.right);
			} else {
				Serial_WritePcLine("ERR IR busy");
			}
			return;
		}
		case 'W': {
			if (args_len != 0u) {
				Serial_WritePcLine("ERR IR syntax");
				return;
			}
			irline_sample_t sample;
			irline_result_t result = IRLine_CalibrateWhite(&sample);
			if (result == IRLINE_RESULT_OK) {
				Serial_WritePcSamplePair("IR WHITE", sample.left, sample.right);
			} else {
				Serial_WritePcLine("ERR IR busy");
			}
			return;
		}
		case 'I': {
			if (args_len != 0u) {
				Serial_WritePcLine("ERR IR syntax");
				return;
			}
			irline_result_t result = IRLine_BeginFollowing();
			switch (result) {
				case IRLINE_RESULT_OK:
					Serial_WritePcLine("IR follow start");
					break;
				case IRLINE_RESULT_NEED_WHITE:
					Serial_WritePcLine("ERR calibrate white");
					break;
				case IRLINE_RESULT_NEED_BLACK:
					Serial_WritePcLine("ERR calibrate black");
					break;
				default:
					Serial_WritePcLine("ERR IR busy");
					break;
			}
			return;
		}
		case 'D': {
			if (args_len != 0u) {
				Serial_WritePcLine("ERR IR syntax");
				return;
			}
			irline_result_t result = IRLine_RequestDone();
			switch (result) {
				case IRLINE_RESULT_OK:
					Serial_WritePcLine("IR exit 3s");
					break;
				case IRLINE_RESULT_NOT_RUNNING:
					Serial_WritePcLine("ERR IR idle");
					break;
				case IRLINE_RESULT_ALREADY_RUNNING:
					Serial_WritePcLine("IR exit pending");
					break;
				default:
					Serial_WritePcLine("ERR IR busy");
					break;
			}
			return;
		}
		default:
			Serial_WritePcLine("ERR invalid direction");
			return;
	}
}

static void Serial_ShowBaudOnDisplay(serial_baud_t mode) {
	(void)mode;
	if (current_display_mode == SERIAL_DISPLAY_WIFI && !big_display_active) {
		Serial_ShowWifiScreen();
	}
}

static void Serial_ProcessIotChar(char c) {
	if (pc_link_open) {
		Serial_WritePcChar(c);
	}

	if (ipd_active) {
		Serial_CaptureIpdByte(c);
		return;
	}

	if (c == '\r') {
		if (ipd_active) {
			Serial_CaptureIpdByte(c);
		}
		return;
	}

	if (c == '\n') {
		if (iot_line_length > 0u) {
			iot_line_buffer[iot_line_length] = '\0';
			uint8_t was_ipd_header = (strncmp(iot_line_buffer, "+IPD,", 5) == 0) ? 1u : 0u;
			Serial_ProcessIotLine(iot_line_buffer);
			iot_line_length = 0u;
			if (was_ipd_header && ipd_active && (ipd_bytes_captured < ipd_expected_length)) {
				Serial_CaptureIpdByte('\n');
			}
		}
		return;
	}

	if (iot_line_length < (SERIAL_MAX_IOT_LINE_LEN - 1u)) {
		iot_line_buffer[iot_line_length++] = c;
	} else {
		iot_line_length = 0u;   // overflow; reset buffer
	}
}

static void Serial_ProcessIotLine(const char *line) {
	if (!line || !*line) {
		return;
	}

	if (strncmp(line, "+IPD,", 5) == 0) {
		Serial_HandleIpdLine(line);
		return;
	}

	if ((strcmp(line, "WIFI CONNECTED") == 0) || (strcmp(line, "WIFI GOT IP") == 0)) {
		Serial_HandleWifiConnected();
		return;
	}

	if (strncmp(line, "+CWJAP:", 7) == 0) {
		Serial_ParseWifiSsidLine(line);
		return;
	}

	if (strncmp(line, "+CIFSR:", 7) == 0) {
		Serial_ParseWifiIpLine(line);
		return;
	}

	if (strcmp(line, "ready") == 0) {
		Serial_HandleWifiReady();
		return;
	}
}

static void Serial_ParseWifiSsidLine(const char *line) {
	const char *first_quote = strchr(line, '"');
	if (!first_quote) {
		return;
	}
	const char *second_quote = strchr(first_quote + 1, '"');
	if (!second_quote) {
		return;
	}

	size_t length = (size_t)(second_quote - first_quote - 1);
	if (length > 10u) {
		length = 10u;
	}

	memcpy(wifi_ssid, first_quote + 1, length);
	wifi_ssid[length] = '\0';
	wifi_ssid_valid = 1u;
	Serial_RefreshWifiScreen();
}

static void Serial_ParseWifiIpLine(const char *line) {
	if (!strstr(line, "STAIP") && !strstr(line, "APIP")) {
		return;
	}

	const char *first_quote = strchr(line, '"');
	if (!first_quote) {
		return;
	}
	const char *second_quote = strchr(first_quote + 1, '"');
	if (!second_quote) {
		return;
	}

	size_t length = (size_t)(second_quote - first_quote - 1);
	if (length >= sizeof(wifi_ip)) {
		length = sizeof(wifi_ip) - 1u;
	}

	memcpy(wifi_ip, first_quote + 1, length);
	wifi_ip[length] = '\0';
	wifi_ip_valid = 1u;
	Serial_RefreshWifiScreen();
}

static void Serial_HandleWifiReady(void) {
	wifi_ssid_valid = 0u;
	wifi_ip_valid = 0u;
	Serial_RefreshWifiScreen();
	server_configured = 0u;
	server_setup_pending = 0u;
	server_setup_stage = 0u;
	server_setup_timestamp = timer200ms;
}

static void Serial_HandleWifiConnected(void) {
	if (!server_configured) {
		server_setup_stage = 1u;
		server_setup_pending = 1u;
		server_setup_timestamp = timer200ms;
	}
}

static void Serial_ServiceServerSetup(void) {
	if (!server_setup_pending) {
		return;
	}

	unsigned int elapsed = timer200ms - server_setup_timestamp;
	if (elapsed < SERIAL_SERVER_SETUP_DELAY_TICKS) {
		return;
	}

	if (server_setup_stage == 1u) {
		Serial_SendIotCommand("AT+CIPMUX=1");
		server_setup_stage = 2u;
		server_setup_timestamp = timer200ms;
		return;
	}

	if (server_setup_stage == 2u) {
		Serial_SendIotCommand("AT+CIPSERVER=1,7898");
		server_setup_stage = 0u;
		server_setup_pending = 0u;
		server_configured = 1u;
		return;
	}

	server_setup_pending = 0u;
}

static void Serial_HandleIpdLine(const char *line) {
	const char *cursor = line + 5; // Skip "+IPD,"
	char *endptr = NULL;
	unsigned long link_id = strtoul(cursor, &endptr, 10);
	if ((endptr == cursor) || (*endptr != ',')) {
		return;
	}
	cursor = endptr + 1;
	unsigned long payload_length = strtoul(cursor, &endptr, 10);
	if (*endptr != ':') {
		return;
	}
	cursor = endptr + 1;
	Serial_BeginIpdCapture((unsigned char)link_id, (unsigned int)payload_length, cursor);
}

static void Serial_BeginIpdCapture(unsigned char link_id, unsigned int payload_length, const char *initial_payload) {
	ipd_active = 1u;
	ipd_link_id = link_id;
	ipd_expected_length = payload_length;
	ipd_bytes_captured = 0u;
	ipd_overflow = 0u;

	if (payload_length == 0u) {
		Serial_FinalizeIpdCapture();
		return;
	}

	while (*initial_payload != '\0') {
		Serial_CaptureIpdByte(*initial_payload++);
		if (!ipd_active) {
			return;
		}
	}
}

static void Serial_CaptureIpdByte(char c) {
	if (!ipd_active) {
		return;
	}

	if (ipd_bytes_captured < ipd_expected_length) {
		if (ipd_bytes_captured < SERIAL_MAX_HTTP_PAYLOAD) {
			ipd_payload_buffer[ipd_bytes_captured] = c;
		} else {
			ipd_overflow = 1u;
		}
		ipd_bytes_captured++;
	}

	if (ipd_bytes_captured >= ipd_expected_length) {
		Serial_FinalizeIpdCapture();
	}
}

static void Serial_FinalizeIpdCapture(void) {
	unsigned int safe_length = ipd_bytes_captured;
	if (safe_length >= SERIAL_MAX_HTTP_PAYLOAD) {
		safe_length = SERIAL_MAX_HTTP_PAYLOAD - 1u;
	}
	ipd_payload_buffer[safe_length] = '\0';
	Serial_DispatchIpdPayload(ipd_link_id, ipd_payload_buffer, ipd_bytes_captured, ipd_overflow);
	ipd_active = 0u;
	ipd_expected_length = 0u;
	ipd_bytes_captured = 0u;
	ipd_overflow = 0u;
}

static void Serial_DispatchIpdPayload(unsigned char link_id, const char *payload, unsigned int length, unsigned char truncated) {
	const char *start = payload;
	while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
		start++;
	}

	if (truncated) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 413 Payload Too Large", "{\"error\":\"payload too large\"}");
		return;
	}

	if (Serial_IsHttpRequest(start)) {
		Serial_HandleHttpRequest(link_id, ipd_payload_buffer, length);
		return;
	}

	Serial_ShowBigText(start);

	if (*start == '^') {
		const char *cmd_body = start + 1;
		size_t cmd_size = strlen(cmd_body);
		if (cmd_size > 0u) {
			Serial_ProcessCommandString(cmd_body, (unsigned int)cmd_size);
		} else {
			Serial_WritePcLine("ERR remote command");
		}
	} else {
		Serial_WritePcLine("RX message");
		Serial_WritePcLine(start);
	}
}

static uint8_t Serial_IsHttpRequest(const char *payload) {
	if (!payload) {
		return 0u;
	}
	while (*payload == ' ' || *payload == '\t' || *payload == '\r' || *payload == '\n') {
		payload++;
	}
	if (strncmp(payload, "GET ", 4) == 0) {
		return 1u;
	}
	if (strncmp(payload, "POST ", 5) == 0) {
		return 1u;
	}
	if (strncmp(payload, "OPTIONS ", 8) == 0) {
		return 1u;
	}
	return 0u;
}

static void Serial_HandleHttpRequest(unsigned char link_id, char *payload, unsigned int length) {
	if (!payload || (length == 0u)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"empty request\"}");
		return;
	}

	char *line_end = strstr(payload, "\r\n");
	if (!line_end) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"malformed request\"}");
		return;
	}

	char method[8];
	char path[64];
	const char *space1 = strchr(payload, ' ');
	if (!space1 || (space1 >= line_end)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"missing path\"}");
		return;
	}
	const char *space2 = strchr(space1 + 1, ' ');
	if (!space2) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"missing version\"}");
		return;
	}

	size_t method_len = (size_t)(space1 - payload);
	if (method_len >= sizeof(method)) {
		method_len = sizeof(method) - 1u;
	}
	memcpy(method, payload, method_len);
	method[method_len] = '\0';

	size_t path_len = (size_t)(space2 - (space1 + 1));
	if (path_len >= sizeof(path)) {
		path_len = sizeof(path) - 1u;
	}
	memcpy(path, space1 + 1, path_len);
	path[path_len] = '\0';
	char *query = strchr(path, '?');
	if (query) {
		*query = '\0';
	}

	const char *headers_start = line_end + 2;
	const char *body_marker = strstr(headers_start, "\r\n\r\n");
	const char *body = body_marker ? (body_marker + 4) : (payload + length);
	unsigned int header_len = body_marker ? (unsigned int)(body_marker - headers_start) : 0u;
	unsigned int body_len = (unsigned int)((payload + length) - body);
	if ((payload + length) < body) {
		body_len = 0u;
	}

	char header_block[SERIAL_HTTP_HEADER_MAX];
	if (header_len >= (SERIAL_HTTP_HEADER_MAX - 1u)) {
		header_len = SERIAL_HTTP_HEADER_MAX - 1u;
	}
	memcpy(header_block, headers_start, header_len);
	header_block[header_len] = '\0';

	if (strcmp(method, "OPTIONS") == 0) {
		Serial_SendHttpNoContent(link_id);
		return;
	}

	if ((strcmp(method, "GET") == 0) && (strcmp(path, "/health") == 0)) {
		Serial_HandleHttpHealth(link_id);
		return;
	}

	if ((strcmp(method, "GET") == 0) && (strcmp(path, "/") == 0)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK",
			"{\"message\":\"Joystick API ready\",\"endpoints\":[\"/api/joystick\",\"/health\"]}");
		return;
	}

	if ((strcmp(method, "POST") == 0) && (strncmp(path, "/api/joystick", 13) == 0)) {
		unsigned int declared_length = Serial_ParseContentLength(header_block);
		if (declared_length > body_len) {
			Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"body truncated\"}");
			return;
		}
		Serial_HandleJoystickApi(link_id, header_block, body, declared_length);
		return;
	}

	if ((strcmp(method, "POST") == 0) && (strncmp(path, "/api/ir", 7) == 0)) {
		unsigned int declared_length = Serial_ParseContentLength(header_block);
		if (declared_length > body_len) {
			Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"body truncated\"}");
			return;
		}
		Serial_HandleIrApi(link_id, header_block, body, declared_length);
		return;
	}

	Serial_SendHttpJson(link_id, "HTTP/1.1 404 Not Found", "{\"error\":\"unknown path\"}");
}

static void Serial_SendHttpResponse(unsigned char link_id, const char *status_line, const char *content_type, const char *body) {
	if (!status_line) {
		status_line = "HTTP/1.1 200 OK";
	}
	if (!content_type) {
		content_type = "text/plain; charset=utf-8";
	}
	if (!body) {
		body = "";
	}

	unsigned int body_len = (unsigned int)strlen(body);
	char header[256];
	int header_len = snprintf(header, sizeof(header),
		"%s\r\n"
		"Content-Type: %s\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n"
		"Access-Control-Allow-Methods: POST, OPTIONS, GET\r\n"
		"Connection: close\r\n"
		"Content-Length: %d\r\n"
		"\r\n",
		status_line, content_type, (int)body_len);
	if (header_len <= 0) {
		return;
	}

	unsigned int total_len = (unsigned int)header_len + body_len;
	char cmd[32];
	int cmd_len = snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", (int)link_id, (int)total_len);
	if (cmd_len <= 0) {
		return;
	}

	Serial_SendIotString(cmd);
	five_msec_sleep(1);
	Serial_WriteIotBuffer(header, (unsigned int)header_len);
	Serial_WriteIotBuffer(body, body_len);
	five_msec_sleep(1);
	Serial_CloseHttpSocket(link_id);
}

static void Serial_SendHttpJson(unsigned char link_id, const char *status_line, const char *json_body) {
	Serial_SendHttpResponse(link_id, status_line, "application/json; charset=utf-8", json_body);
}

static void Serial_SendHttpNoContent(unsigned char link_id) {
	char header[192];
	int header_len = snprintf(header, sizeof(header),
		"HTTP/1.1 204 No Content\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n"
		"Access-Control-Allow-Methods: POST, OPTIONS, GET\r\n"
		"Connection: close\r\n"
		"Content-Length: 0\r\n"
		"\r\n");
	if (header_len <= 0) {
		return;
	}
	char cmd[32];
	int cmd_len = snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", (int)link_id, (int)header_len);
	if (cmd_len <= 0) {
		return;
	}
	Serial_SendIotString(cmd);
	five_msec_sleep(1);
	Serial_WriteIotBuffer(header, (unsigned int)header_len);
	five_msec_sleep(1);
	Serial_CloseHttpSocket(link_id);
}

static void Serial_WriteIotBuffer(const char *data, unsigned int length) {
	unsigned int i;
	if (!data) {
		return;
	}
	for (i = 0u; i < length; i++) {
		Serial_WriteIotChar(data[i]);
	}
}

static void Serial_CloseHttpSocket(unsigned char link_id) {
	char cmd[20];
	int len = snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d", (int)link_id);
	if (len > 0) {
		Serial_SendIotString(cmd);
	}
}

static void Serial_HandleJoystickApi(unsigned char link_id, const char *headers, const char *body, unsigned int body_length) {
	if (!body || (body_length == 0u)) {
		Motor_ApplyJoystickVector(0, 0, 0u, NULL, NULL);
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"idle\"}");
		return;
	}

	if (!Serial_HasFormContentType(headers)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 415 Unsupported Media Type",
			"{\"error\":\"use application/x-www-form-urlencoded\"}");
		return;
	}

	char form_buffer[160];
	unsigned int copy_len = body_length;
	if (copy_len >= sizeof(form_buffer)) {
		copy_len = sizeof(form_buffer) - 1u;
	}
	memcpy(form_buffer, body, copy_len);
	form_buffer[copy_len] = '\0';

	int x_val = 0;
	int y_val = 0;
	int active_flag = 0;
	if (!Serial_ParseFormInt(form_buffer, "x", &x_val) ||
		!Serial_ParseFormInt(form_buffer, "y", &y_val)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"missing axis\"}");
		return;
	}
	if (!Serial_ParseFormInt(form_buffer, "active", &active_flag)) {
		int abs_x = (x_val >= 0) ? x_val : -x_val;
		int abs_y = (y_val >= 0) ? y_val : -y_val;
		active_flag = ((abs_y > MOTOR_JOYSTICK_FORWARD_DEADBAND) ||
			(abs_x > MOTOR_JOYSTICK_FORWARD_DEADBAND)) ? 1 : 0;
	}

	if (x_val > MOTOR_JOYSTICK_AXIS_SCALE) {
		x_val = MOTOR_JOYSTICK_AXIS_SCALE;
	}
	if (x_val < -MOTOR_JOYSTICK_AXIS_SCALE) {
		x_val = -MOTOR_JOYSTICK_AXIS_SCALE;
	}
	if (y_val > MOTOR_JOYSTICK_AXIS_SCALE) {
		y_val = MOTOR_JOYSTICK_AXIS_SCALE;
	}
	if (y_val < -MOTOR_JOYSTICK_AXIS_SCALE) {
		y_val = -MOTOR_JOYSTICK_AXIS_SCALE;
	}

	unsigned int applied_left = 0u;
	unsigned int applied_right = 0u;
	Motor_ApplyJoystickVector((int16_t)x_val, (int16_t)y_val, (active_flag != 0), &applied_left, &applied_right);

	char response[192];
	char left_str[8];
	char right_str[8];
	Serial_Utoa(applied_left, left_str, sizeof(left_str));
	Serial_Utoa(applied_right, right_str, sizeof(right_str));
	int len = snprintf(response, sizeof(response),
		"{\"left\":%s,\"right\":%s,\"axis\":{\"x\":%d,\"y\":%d},\"engaged\":%d}",
		left_str,
		right_str,
		x_val,
		y_val,
		(active_flag != 0));
	if (len < 0) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"ok\"}");
		return;
	}
	Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", response);
}

static void Serial_HandleIrApi(unsigned char link_id, const char *headers, const char *body, unsigned int body_length) {
	if (!body || (body_length == 0u)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"missing body\"}");
		return;
	}

	if (!Serial_HasFormContentType(headers)) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 415 Unsupported Media Type",
			"{\"error\":\"use application/x-www-form-urlencoded\"}");
		return;
	}

	char form_buffer[64];
	unsigned int copy_len = body_length;
	if (copy_len >= sizeof(form_buffer)) {
		copy_len = sizeof(form_buffer) - 1u;
	}
	memcpy(form_buffer, body, copy_len);
	form_buffer[copy_len] = '\0';

	const char *cmd_start = strstr(form_buffer, "cmd=");
	if (!cmd_start) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"missing cmd\"}");
		return;
	}
	cmd_start += 4; // Skip "cmd="

	char cmd_val[32];
	unsigned int i = 0;
	while(cmd_start[i] && cmd_start[i] != '&' && i < sizeof(cmd_val)-1) {
		cmd_val[i] = cmd_start[i];
		i++;
	}
	cmd_val[i] = '\0';

	if (strcmp(cmd_val, "stop") == 0) {
		IRLine_ForceStop();
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"stopped\"}");
	} else if (strcmp(cmd_val, "cal_white") == 0) {
		IRLine_CalibrateWhite(NULL);
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"calibrated_white\"}");
	} else if (strcmp(cmd_val, "cal_black") == 0) {
		IRLine_CalibrateBlack(NULL);
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"calibrated_black\"}");
	} else if (strcmp(cmd_val, "start") == 0) {
		irline_result_t res = IRLine_BeginFollowing();
		if(res == IRLINE_RESULT_OK)
			 Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"started\"}");
		else
			 Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"calibration_needed\"}");
	} else if (strcmp(cmd_val, "exit") == 0) {
		IRLine_RequestDone();
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"exiting\"}");
	} else {
		Serial_SendHttpJson(link_id, "HTTP/1.1 400 Bad Request", "{\"error\":\"unknown_cmd\"}");
	}
}

static unsigned int Serial_ParseContentLength(const char *headers) {
	if (!headers) {
		return 0u;
	}
	const char *marker = strstr(headers, "Content-Length:");
	if (!marker) {
		return 0u;
	}
	marker += strlen("Content-Length:");
	while (*marker == ' ') {
		marker++;
	}
	unsigned int value = 0u;
	while ((*marker >= '0') && (*marker <= '9')) {
		value = (value * 10u) + (unsigned int)(*marker - '0');
		marker++;
	}
	return value;
}

static uint8_t Serial_HasFormContentType(const char *headers) {
	if (!headers) {
		return 0u;
	}
	const char *cursor = headers;
	const char target[] = "application/x-www-form-urlencoded";
	const unsigned int target_len = (unsigned int)(sizeof(target) - 1u);
	unsigned int idx;
	while ((cursor = strstr(cursor, "Content-Type")) != NULL) {
		const char *colon = strchr(cursor, ':');
		if (!colon) {
			break;
		}
		colon++;
		while (*colon == ' ') {
			colon++;
		}
		unsigned int match = 1u;
		idx = 0u;
		while (idx < target_len) {
			char lhs = (char)tolower((unsigned char)colon[idx]);
			char rhs = target[idx];
			if (lhs != rhs) {
				match = 0u;
				break;
			}
			idx++;
		}
		if (match) {
			return 1u;
		}
		cursor = colon;
	}
	return 0u;
}

static uint8_t Serial_ParseFormInt(const char *body, const char *key, int *value_out) {
	if (!body || !key) {
		return 0u;
	}
	size_t key_len = strlen(key);
	const char *cursor = body;
	while (cursor && *cursor) {
		if ((strncmp(cursor, key, key_len) == 0) && (cursor[key_len] == '=')) {
			cursor += key_len + 1u;
			int sign = 1;
			if (*cursor == '-') {
				sign = -1;
				cursor++;
			}
			int value = 0;
			uint8_t has_digit = 0u;
			while ((*cursor >= '0') && (*cursor <= '9')) {
				has_digit = 1u;
				value = (value * 10) + (int)(*cursor - '0');
				cursor++;
			}
			if (!has_digit) {
				return 0u;
			}
			if (value_out) {
				*value_out = value * sign;
			}
			return 1u;
		}
		cursor = strchr(cursor, '&');
		if (cursor) {
			cursor++;
		}
	}
	return 0u;
}

static void Serial_Utoa(unsigned int value, char *buffer, unsigned int buffer_len) {
	char digits[6];
	unsigned int count = 0u;
	unsigned int out_idx = 0u;

	if (!buffer || (buffer_len == 0u)) {
		return;
	}

	do {
		digits[count++] = (char)('0' + (value % 10u));
		value /= 10u;
	} while ((value > 0u) && (count < (unsigned int)sizeof(digits)));

	while ((count > 0u) && (out_idx < (buffer_len - 1u))) {
		buffer[out_idx++] = digits[--count];
	}
	buffer[out_idx] = '\0';
}

static void Serial_HandleHttpHealth(unsigned char link_id) {
	const char *ssid = wifi_ssid_valid ? wifi_ssid : "unknown";
	const char *ip = wifi_ip_valid ? wifi_ip : "0.0.0.0";
	char payload[192];
	int len = snprintf(payload, sizeof(payload),
		"{\"wifi\":{\"ssid\":\"%s\",\"ip\":\"%s\"},\"serverConfigured\":%d,\"failsafeTicks\":%d}",
		ssid,
		ip,
		(int)server_configured,
		(int)MOTOR_JOYSTICK_FAILSAFE_TICKS);
	if (len < 0) {
		Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", "{\"status\":\"ok\"}");
		return;
	}
	Serial_SendHttpJson(link_id, "HTTP/1.1 200 OK", payload);
}

static void Serial_ShowBigCommand(char direction, unsigned int duration) {
	if (direction == 'S') {
		Serial_ShowBigText("STOP");
		return;
	}

	char line[11];
	char digits[6];
	unsigned int idx = 0u;
	unsigned int value = duration;

	do {
		digits[idx++] = (char)('0' + (value % 10u));
		value /= 10u;
	} while ((value > 0u) && (idx < sizeof(digits)));

	unsigned int pos = 0u;
	line[pos++] = direction;
	line[pos++] = ' ';
	while (idx > 0u && pos < (sizeof(line) - 1u)) {
		line[pos++] = digits[--idx];
	}
	line[pos] = '\0';

	Serial_ShowBigText(line);
}

static void Serial_ShowBigText(const char *text) {
	if (!text || !*text) {
		return;
	}

	char buffer[11];
	unsigned int i = 0u;
	while ((text[i] != '\0') && (i < (sizeof(buffer) - 1u))) {
		buffer[i] = text[i];
		i++;
	}
	buffer[i] = '\0';

	lcd_BIG_mid();
	dispPrint((char *)buffer, 2);

	big_display_active = 1u;
	big_display_timestamp = timer200ms;
	current_display_mode = SERIAL_DISPLAY_BIG;
}

static void Serial_ShowWifiScreen(void) {
	big_display_active = 0u;
	current_display_mode = SERIAL_DISPLAY_WIFI;
	display_mode_timestamp = timer200ms;

	lcd_4line();

	if (wifi_ssid_valid) {
		dispPrint(wifi_ssid, 1);
	} else {
		dispPrint((char *)"SSID?", 1);
	}

	if (wifi_ip_valid) {
		char line3[11];
		char line4[11];
		Serial_FormatIpForDisplay(line3, line4);
		dispPrint((char *)"IP Addr", 2);
		dispPrint(line3, 3);
		dispPrint(line4, 4);
	} else {
		dispPrint((char *)"IP Addr", 2);
		dispPrint((char *)"None", 3);
		dispPrint((char *)"", 4);
	}
}

static void Serial_RefreshWifiScreen(void) {
	if (current_display_mode == SERIAL_DISPLAY_WIFI && !big_display_active) {
		Serial_ShowWifiScreen();
	}
}

static void Serial_ShowWaitingScreen(void) {
	big_display_active = 0u;
	current_display_mode = SERIAL_DISPLAY_WAITING;

	lcd_BIG_mid();  // Switch to BIG mode for centered "WAITING"
	dispPrint((char *)"Ohm Patel", 1);
	dispPrint((char *)"WAITING", 2);  // Show "WAITING" centered on line 2
	Serial_UpdateIrAdcLine();
}

static void Serial_UpdateIrAdcLine(void) {
	char line[11];
	Serial_FormatFourDigit(line, ADCLeft);
	line[4] = ' ';
	line[5] = ' ';
	Serial_FormatFourDigit(&line[6], ADCRight);
	line[10] = '\0';
	dispPrint(line, 3);
}

static void Serial_FormatFourDigit(char *dest, unsigned int value) {
	unsigned int remaining;
	unsigned int idx;

	if (!dest) {
		return;
	}

	remaining = (value > 9999u) ? 9999u : value;
	for (idx = 0u; idx < 4u; idx++) {
		dest[3u - idx] = (char)('0' + (remaining % 10u));
		remaining /= 10u;
	}
}

static void Serial_DisplayModeService(void) {
	if (current_display_mode == SERIAL_DISPLAY_WIFI) {
		unsigned int elapsed = timer200ms - display_mode_timestamp;
		if (elapsed >= SERIAL_WIFI_DISPLAY_HOLD_TICKS) {
			Serial_ShowWaitingScreen();
		}
	}

	if (current_display_mode == SERIAL_DISPLAY_WAITING && !big_display_active) {
		Serial_UpdateIrAdcLine();
	}

	// Restore "WAITING" screen after big display timeout
	// BUT only if no command is currently executing
	if (current_display_mode == SERIAL_DISPLAY_BIG && big_display_active) {
		// Don't timeout if a command is still executing
		if (Wheels_IsExecuting()) {
			// Reset timestamp to keep display active during command execution
			big_display_timestamp = timer200ms;
		} else {
			unsigned int elapsed = timer200ms - big_display_timestamp;
			if (elapsed >= SERIAL_BIG_DISPLAY_HOLD_TICKS) {
				big_display_active = 0u;
				Serial_ShowWaitingScreen();
			}
		}
	}
}

static void Serial_ServiceHeartbeat(void) {
	if (!pc_link_open) {
		return;
	}

	unsigned int elapsed = timer200ms - last_heartbeat_stamp;
	if (elapsed >= SERIAL_HEARTBEAT_PERIOD_TICKS) {
		last_heartbeat_stamp = timer200ms;
		Serial_WritePcLine("Dil Dhadakne Do");
	}
}

static unsigned int Serial_ParseUnsigned(const char *s, unsigned char len) {
	unsigned int value = 0u;
	unsigned char consumed = 0u;

	while (consumed < len && s[consumed] != '\0') {
		char c = s[consumed];
		if ((c < '0') || (c > '9')) {
			break;
		}
		value = (value * 10u) + (unsigned int)(c - '0');
		consumed++;
	}

	if (consumed == 0u) {
		return 0u;
	}

	return value;
}

static uint8_t Serial_ConvertDurationToTicks(char direction, unsigned int raw_value, unsigned int *out_ticks) {
	unsigned int ticks = 0u;

	if (!out_ticks) {
		return 0u;
	}

	switch (direction) {
		case 'F':
		case 'B':
			ticks = raw_value * TICKS_PER_SECOND;
			break;
		case 'L':
		case 'R':
			ticks = (raw_value + (SERIAL_TURN_DEGREES_PER_TICK - 1u)) / SERIAL_TURN_DEGREES_PER_TICK;
			break;
		default:
			return 0u;
	}

	if (ticks == 0u) {
		ticks = 1u;
	}

	if (ticks > 0xFFFFu) {
		return 0u;
	}

	*out_ticks = ticks;
	return 1u;
}

static void Serial_SendIotString(const char *command) {
	const char *ptr = command;
	while (ptr && *ptr) {
		Serial_WriteIotChar(*ptr++);
	}
	Serial_WriteIotChar('\r');
	Serial_WriteIotChar('\n');
}

static void Serial_FormatIpForDisplay(char *line3, char *line4) {
	line3[0] = '\0';
	line4[0] = '\0';

	if (!wifi_ip_valid) {
		return;
	}

	const char *first_dot = strchr(wifi_ip, '.');
	if (!first_dot) {
		strncpy(line3, wifi_ip, 10);
		line3[10] = '\0';
		return;
	}

	const char *second_dot = strchr(first_dot + 1, '.');
	if (!second_dot) {
		strncpy(line3, wifi_ip, 10);
		line3[10] = '\0';
		return;
	}

	size_t len_first = (size_t)(second_dot - wifi_ip);
	if (len_first > 10u) {
		len_first = 10u;
	}
	strncpy(line3, wifi_ip, len_first);
	line3[len_first] = '\0';

	const char *rest = second_dot + 1;
	strncpy(line4, rest, 10);
	line4[10] = '\0';
}
