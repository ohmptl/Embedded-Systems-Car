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
#include <string.h>
#include <ctype.h>

#define SERIAL_HEARTBEAT_PERIOD_TICKS      (5u)   // ~1 second @ 0.2s tick
#define SERIAL_BIG_DISPLAY_HOLD_TICKS      (25u)  // ~5 seconds (increased from 5 to match wifi timeout)
#define SERIAL_WIFI_DISPLAY_HOLD_TICKS     (25u)  // ~5 seconds
#define SERIAL_MAX_CMD_LEN                 (32u)
#define SERIAL_MAX_IOT_LINE_LEN            (96u)
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

extern volatile unsigned int timer200ms;

// Forward declarations -------------------------------------------------------
static void Serial_ResetRings(void);
static void Serial_InitUCA0(serial_baud_t speed);
static void Serial_InitUCA1(void);
static void Serial_WritePcChar(char c);
static void Serial_WritePcString(const char *s);
static void Serial_WritePcLine(const char *s);
static void Serial_WriteIotChar(char c);
static void Serial_ProcessUsbChar(char c);
static void Serial_FinalizeCommand(void);
static void Serial_HandleCommand(const char *cmd, unsigned char len);
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
static void Serial_DisplayModeService(void);
static void Serial_ServiceHeartbeat(void);
static void Serial_ParseIotPacket(const char *line);
static void Serial_HandleIotPayload(const char *payload);
static void Serial_ShowBigText(const char *text);
static void Serial_ShowBigCommand(char direction, unsigned int duration);
static void Serial_TeardownBigCommand(void);
static unsigned int Serial_ParseUnsigned(const char *s, unsigned char len);
static void Serial_SendIotString(const char *command);
static void Serial_FormatIpForDisplay(char *line3, char *line4);
static uint8_t Serial_ConvertDurationToTicks(char direction, unsigned int raw_value, unsigned int *out_ticks);
static void Serial_HandleWifiConnected(void);
static void Serial_ServiceServerSetup(void);

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

	Serial_HandleCommand(buffer, command_length);
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
			char direction = (char)toupper((unsigned char)cmd[SERIAL_AUTH_PIN_LEN]);
			const char *duration_str = cmd + SERIAL_AUTH_PIN_LEN + 1u;
			unsigned char expects_duration = 1u;
			unsigned int raw_value = 0u;
			unsigned int duration_ticks = 0u;

			switch (direction) {
				case 'F':
				case 'B':
				case 'L':
				case 'R':
					expects_duration = 1u;
					break;
				case 'S':
					expects_duration = 0u;
					break;
				default:
					Serial_WritePcLine("ERR invalid direction");
					return;
			}

			if (expects_duration) {
				if (*duration_str == '\0') {
					Serial_WritePcLine("ERR missing duration");
					return;
				}
				unsigned char digit_len = (unsigned char)strlen(duration_str);
				raw_value = Serial_ParseUnsigned(duration_str, digit_len);
				if ((raw_value == 0u) || (raw_value > 9999u)) {
					Serial_WritePcLine("ERR invalid duration");
					return;
				}
				if ((direction == 'L') || (direction == 'R')) {
					if (raw_value > SERIAL_MAX_TURN_DEGREES) {
						Serial_WritePcLine("ERR turn angle");
						return;
					}
				}
				if (!Serial_ConvertDurationToTicks(direction, raw_value, &duration_ticks)) {
					Serial_WritePcLine("ERR duration range");
					return;
				}
			} else {
				if (*duration_str != '\0') {
					Serial_WritePcLine("ERR stop syntax");
					return;
				}
			}

			pending_motion_command.direction = direction;
			pending_motion_command.duration  = (uint16_t)duration_ticks;
			motion_command_pending = 1u;

			if (direction == 'S') {
				Serial_WritePcLine("CMD stop");
				Serial_ShowBigCommand(direction, 0u);
			} else {
				Serial_WritePcLine("CMD accepted");
				Serial_ShowBigCommand(direction, raw_value);
			}
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

	if (c == '\r') {
		return;
	}

	if (c == '\n') {
		if (iot_line_length > 0u) {
			iot_line_buffer[iot_line_length] = '\0';
			Serial_ProcessIotLine(iot_line_buffer);
			iot_line_length = 0u;
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
		Serial_ParseIotPacket(line);
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
		Serial_SendIotCommand("AT+CIPSERVER=1,8080");
		server_setup_stage = 0u;
		server_setup_pending = 0u;
		server_configured = 1u;
		return;
	}

	server_setup_pending = 0u;
}

static void Serial_ParseIotPacket(const char *line) {
	const char *colon = strchr(line, ':');
	if (!colon) {
		return;
	}

	const char *payload = colon + 1;
	if (!*payload) {
		return;
	}

	Serial_HandleIotPayload(payload);
}

static void Serial_HandleIotPayload(const char *payload) {
	char clean[64];
	unsigned int idx = 0u;

	while (payload[idx] && payload[idx] != '\r' && payload[idx] != '\n' && idx < (sizeof(clean) - 1u)) {
		clean[idx] = payload[idx];
		idx++;
	}
	clean[idx] = '\0';

	char *start = clean;
	while ((*start == ' ') || (*start == '\t')) {
		start++;
	}
	if (*start == '\0') {
		return;
	}

	char *end = start + strlen(start);
	while ((end > start) && ((end[-1] == ' ') || (end[-1] == '\t'))) {
		*--end = '\0';
	}

	Serial_ShowBigText(start);

	if (*start == '^') {
		const char *cmd_body = start + 1;
		size_t cmd_size = strlen(cmd_body);
		if ((cmd_size > 0u) && (cmd_size < 255u)) {
			Serial_HandleCommand(cmd_body, (unsigned char)cmd_size);
		} else {
			Serial_WritePcLine("ERR remote command");
		}
	} else {
		Serial_WritePcLine("RX message");
		Serial_WritePcLine(start);
	}
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

static void Serial_TeardownBigCommand(void) {
	Serial_ShowWaitingScreen();
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
	dispPrint((char *)"ECE 306", 3);
}

static void Serial_DisplayModeService(void) {
	if (current_display_mode == SERIAL_DISPLAY_WIFI) {
		unsigned int elapsed = timer200ms - display_mode_timestamp;
		if (elapsed >= SERIAL_WIFI_DISPLAY_HOLD_TICKS) {
			Serial_ShowWaitingScreen();
		}
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
