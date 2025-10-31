//------------------------------------------------------------------------------
//  Name:           serial.c
//  Description:    serial file
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include "ports.h"
#include "msp430.h"
#include "macros.h"
#include "serial.h"
#include "display.h"
#include <string.h>


volatile unsigned int iot_rx_wr;
volatile unsigned int iot_rx_rd;
char IOT_Data[11][11];
char IOT_Ring_Rx[11];
volatile int ip_address[11];
char ip_mac[11];
volatile unsigned int direct_iot;
volatile unsigned int iot_index;
volatile int ip_address_found;
volatile unsigned int iot_tx;
volatile unsigned int boot_state;
volatile unsigned int IOT_parse;
volatile unsigned int usb_rx_ring_wr = 0;
volatile unsigned int usb_rx_ring_rd;
volatile unsigned int usb_tx_ring_wr;
volatile unsigned int usb_tx_ring_rd;
volatile char USB_Char_Rx[SMALL_RING_SIZE];
volatile char USB_Char_Tx[11];
volatile char IOT_Char_Rx[11];
volatile char IOT_Char_Tx[11];
volatile int Still_Put_In_Processor = 1;
volatile unsigned int test_Value;
unsigned int nextline;
int unsigned line = 0;
int character = 0;
char pb_index;  // Index for process_buffer
volatile char NCSU[] = "NCSU  #1  ";
volatile unsigned int ncsu_index = 0;
// RX ring for UCA1 (USB back-channel). Use a modest ring to avoid overflow on bursts.
volatile char USB_Ring_Rx[SMALL_RING_SIZE];
char iot_TX_buf[11];


extern volatile unsigned int Time_Sequence;

#define SERIAL_MESSAGE_LEN          (10u)
#define SERIAL_TRANSMIT_HOLD_TICKS  (5u)

typedef enum {
    SERIAL_STATE_WAITING = 0,
    SERIAL_STATE_RECEIVED,
    SERIAL_STATE_TRANSMIT
} serial_display_state_t;

static serial_display_state_t serial_display_state = SERIAL_STATE_WAITING;
static unsigned int serial_state_timestamp = 0;
static unsigned char serial_baud_mode = 1;
static unsigned char serial_command_available = 0;
static unsigned char serial_payload_len = 0;
static char serial_payload[SERIAL_MESSAGE_LEN];
static char serial_display_buf[SERIAL_MESSAGE_LEN + 1];
static char usb_rx_message_buf[SERIAL_MESSAGE_LEN];
static unsigned char usb_rx_message_len = 0;

extern char display_line[4][11];
extern volatile unsigned char display_changed;

// UART Initialization: eUSCI_A0 (115200 baud)
void Init_Serial_UCA0(char speed){
    //-----------------------------------------------------------------------------
    //                                               TX error (%) RX error (%)
    //  BRCLK   Baudrate UCOS16  UCBRx  UCFx    UCSx    neg   pos  neg  pos
    //  8000000    4800     1     104     2     0xD6   -0.08 0.04 -0.10 0.14
    //  8000000    9600     1      52     1     0x49   -0.08 0.04 -0.10 0.14
    //  8000000   19200     1      26     0     0xB6   -0.08 0.16 -0.28 0.20
    //  8000000   57600     1       8    10     0xF7   -0.32 0.32 -1.00 0.36
    //  8000000  115200     1       4     5     0x55   -0.80 0.64 -1.12 1.76
    //  8000000  460800     0      17     0     0x4A   -2.72 2.56 -3.76 7.28
    //-----------------------------------------------------------------------------
    // Configure eUSCI_A0 for UART mode
    UCA0CTLW0 = 0;
    UCA0CTLW0 |=  UCSWRST ;              // Put eUSCI in reset
    UCA0CTLW0 |=  UCSSEL__SMCLK;         // Set SMCLK as fBRCLK
    UCA0CTLW0 &= ~UCMSB;                 // MSB, LSB select
    UCA0CTLW0 &= ~UCSPB;                 // UCSPB = 0(1 stop bit) OR 1(2 stop bits)
    UCA0CTLW0 &= ~UCPEN;                 // No Parity
    UCA0CTLW0 &= ~UCSYNC;
    UCA0CTLW0 &= ~UC7BIT;
    UCA0CTLW0 |=  UCMODE_0;
    //    BRCLK   Baudrate UCOS16  UCBRx  UCFx    UCSx    neg   pos  neg  pos
    //    8000000  115200     1       4     5     0x55   -0.80 0.64 -1.12 1.76
    //    UCA?MCTLW = UCSx + UCFx + UCOS16

    int i;
    // Init A0 RX ring (IOT_Ring_Rx) and indices
    for(i=0; i<(int)sizeof(IOT_Ring_Rx); i++){
        IOT_Ring_Rx[i] = 0x00;
    }
    iot_rx_wr = BEGINNING;
    iot_rx_rd = BEGINNING;

    switch(speed){
    case 1:
        UCA0BRW = 4 ;                        // 115,200 baud
        UCA0MCTLW = 0x5551;
        break;
    case 2:
        UCA0BRW = 52;                        // 9,600 baud
        UCA0MCTLW = 0x4911;                  // UCBRS=0x49, UCBRF=1, UCOS16=1
        break;
    default:
        break;
    }

    UCA0CTLW0 &= ~UCSWRST ;              // release from reset
    // Do not write a dummy byte here; loopback setups would see it as a NUL.
    UCA0IE |= UCRXIE;                    // Enable RX interrupt

    //-----------------------------------------------------------------------------
}

void Init_Serial_UCA1(char speed){
    //-----------------------------------------------------------------------------
    //                                               TX error (%) RX error (%)
    //  BRCLK   Baudrate UCOS16  UCBRx  UCFx    UCSx    neg   pos  neg  pos
    //  8000000    4800     1     104     2     0xD6   -0.08 0.04 -0.10 0.14
    //  8000000    9600     1      52     1     0x49   -0.08 0.04 -0.10 0.14
    //  8000000   19200     1      26     0     0xB6   -0.08 0.16 -0.28 0.20
    //  8000000   57600     1       8    10     0xF7   -0.32 0.32 -1.00 0.36
    //  8000000  115200     1       4     5     0x55   -0.80 0.64 -1.12 1.76
    //  8000000  460800     0      17     0     0x4A   -2.72 2.56 -3.76 7.28
    //-----------------------------------------------------------------------------
    // Configure eUSCI_A0 for UART mode
    UCA1CTLW0 = 0;
    UCA1CTLW0 |=  UCSWRST ;              // Put eUSCI in reset
    UCA1CTLW0 |=  UCSSEL__SMCLK;         // Set SMCLK as fBRCLK
    UCA1CTLW0 &= ~UCMSB;                 // MSB, LSB select
    UCA1CTLW0 &= ~UCSPB;                 // UCSPB = 0(1 stop bit) OR 1(2 stop bits)
    UCA1CTLW0 &= ~UCPEN;                 // No Parity
    UCA1CTLW0 &= ~UCSYNC;
    UCA1CTLW0 &= ~UC7BIT;
    UCA1CTLW0 |=  UCMODE_0;
    //    BRCLK   Baudrate UCOS16  UCBRx  UCFx    UCSx    neg   pos  neg  pos
    //    8000000  115200     1       4     5     0x55   -0.80 0.64 -1.12 1.76
    //    UCA?MCTLW = UCSx + UCFx + UCOS16
    int i;
    // Init A1 RX ring (USB_Ring_Rx) and indices used by ISR/processor
    for(i=0; i<(int)sizeof(USB_Ring_Rx); i++){
        USB_Ring_Rx[i] = 0x00;
    }
    usb_rx_ring_wr = BEGINNING;
    usb_rx_ring_rd = BEGINNING;

    switch(speed){
    case 1:
        UCA1BRW = 4;                         // 115,200 baud @ 8MHz, UCOS16=1, UCBRF=5, UCBRS=0x55
        UCA1MCTLW = 0x5551;                  // UCBRS=0x55, UCBRF=5, UCOS16=1
        break;
    case 2:
        UCA1BRW = 52;                        // 9,600 baud @ 8MHz, UCOS16=1
        UCA1MCTLW = 0x4911;                  // UCBRS=0x49, UCBRF=1, UCOS16=1
        break;
    default:
        break;
    }

    UCA1CTLW0 &= ~UCSWRST ;              // release from reset
    // Avoid sending a spurious NUL that would pollute loopback RX buffers.
    UCA1IE |= UCRXIE;                    // Enable RX interrupt

    //-----------------------------------------------------------------------------
}

// UART Transmit Function: USCI_A0
char process_buffer[25];   // Buffer for commands/strings
char pb_index;             // Index for buffer

void USCI_A0_transmit(void) {
    pb_index = 0;          // Start at beginning
    UCA0IE |= UCTXIE;      // Enable TX interrupt
}


// Example Process Function for IOT Messages (not used for HW08)
// Keep as a safe no-op to avoid pulling in undefined symbols/macros
void IOT_Process(void) {
    (void)iot_tx; (void)iot_index; (void)IOT_parse; (void)ip_address_found;
    // Intentionally empty
}

//------------------------------------------------------------------------------
// Simple polled sender for UCA1: send a C-string out the back-channel UART
//------------------------------------------------------------------------------
void UCA1_SendString(const char *s) {
    if (!s) return;
    while (*s) {
        while (!(UCA1IFG & UCTXIFG)) { /* wait */ }
        UCA1TXBUF = *s++;
    }
}

static void Serial_DisplayRawLine(unsigned int row, const char *text, unsigned char length) {
    unsigned int i;
    unsigned char changed = 0;

    if (row >= 4u) {
        return;
    }

    for (i = 0; i < 10u; i++) {
        char new_char = (i < length) ? text[i] : ' ';
        if (display_line[row][i] != new_char) {
            display_line[row][i] = new_char;
            changed = 1u;
        }
    }
    display_line[row][10] = '\0';

    if (changed) {
        display_changed = 1;
    }
}

static void Serial_DisplayClearLine(unsigned int row) {
    Serial_DisplayRawLine(row, "", 0u);
}

static const char* Serial_BaudLabel(unsigned char mode) {
    return (mode == 2u) ? "9600" : "115200";
}

static void Serial_ShowBaudOnLine3(void) {
    char line_text[11];
    const char *label = Serial_BaudLabel(serial_baud_mode);
    unsigned int i;
    unsigned int count = (unsigned int)strlen(label);

    if (count > 7u) {
        count = 7u;
    }

    for (i = 0; i < 10u; i++) {
        line_text[i] = ' ';
    }
    line_text[10] = '\0';
    line_text[0] = 'B';
    line_text[1] = 'R';
    line_text[2] = ':';

    for (i = 0; i < count; i++) {
        line_text[3 + i] = label[i];
    }

    Serial_DisplayRawLine(2u, line_text, 10u);
}

static void Serial_FinalizeMessage(unsigned char length) {
    unsigned int i;

    if (length == 0u) {
        usb_rx_message_len = 0u;
        return;
    }

    if (length > SERIAL_MESSAGE_LEN) {
        length = SERIAL_MESSAGE_LEN;
    }

    for (i = 0; i < length; i++) {
        serial_payload[i] = usb_rx_message_buf[i];
        serial_display_buf[i] = usb_rx_message_buf[i];
    }
    for (; i < SERIAL_MESSAGE_LEN; i++) {
        serial_payload[i] = ' ';
        serial_display_buf[i] = ' ';
    }
    serial_display_buf[SERIAL_MESSAGE_LEN] = '\0';

    serial_payload_len = length;
    serial_command_available = 1u;

    dispPrint((char *)"Received", 1);
    Serial_DisplayClearLine(1u);
    Serial_DisplayRawLine(3u, serial_display_buf, SERIAL_MESSAGE_LEN);

    serial_display_state = SERIAL_STATE_RECEIVED;
    serial_state_timestamp = Time_Sequence;
    usb_rx_message_len = 0u;
}

void Serial_Project8_Init(void) {
    unsigned int i;

    serial_baud_mode = 1u;
    serial_display_state = SERIAL_STATE_WAITING;
    serial_state_timestamp = Time_Sequence;
    serial_command_available = 0u;
    serial_payload_len = 0u;
    usb_rx_message_len = 0u;

    for (i = 0; i < SERIAL_MESSAGE_LEN; i++) {
        serial_payload[i] = ' ';
        serial_display_buf[i] = ' ';
        usb_rx_message_buf[i] = 0;
    }
    serial_display_buf[SERIAL_MESSAGE_LEN] = '\0';

    Init_Serial_UCA0(serial_baud_mode);
    Init_Serial_UCA1(serial_baud_mode);

    dispPrint((char *)"Waiting", 1);
    Serial_DisplayClearLine(1u);
    Serial_ShowBaudOnLine3();
    Serial_DisplayClearLine(3u);
}

void Serial_Process_USB_RX(void) {
    while (UCA1IFG & UCRXIFG) {
        char c = UCA1RXBUF;
        USB_Ring_Rx[usb_rx_ring_wr++] = c;
        if (usb_rx_ring_wr >= sizeof(USB_Ring_Rx)) {
            usb_rx_ring_wr = BEGINNING;
        }
    }

    while (usb_rx_ring_rd != usb_rx_ring_wr) {
        char c = USB_Ring_Rx[usb_rx_ring_rd++];
        if (usb_rx_ring_rd >= sizeof(USB_Ring_Rx)) {
            usb_rx_ring_rd = BEGINNING;
        }

        if (c == '\0') {
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (usb_rx_message_len > 0u) {
                Serial_FinalizeMessage(usb_rx_message_len);
            }
            usb_rx_message_len = 0u;
            continue;
        }

        if (usb_rx_message_len < SERIAL_MESSAGE_LEN) {
            usb_rx_message_buf[usb_rx_message_len++] = c;
            if (usb_rx_message_len >= SERIAL_MESSAGE_LEN) {
                Serial_FinalizeMessage(usb_rx_message_len);
            }
        }
    }

    if (serial_display_state == SERIAL_STATE_TRANSMIT) {
        unsigned int elapsed = Time_Sequence - serial_state_timestamp;
        if (elapsed >= SERIAL_TRANSMIT_HOLD_TICKS) {
            dispPrint((char *)"Waiting", 1);
            serial_display_state = SERIAL_STATE_WAITING;
            serial_state_timestamp = Time_Sequence;
        }
    }
}

void Serial_Project8_HandleTransmitRequest(void) {
    unsigned int i;

    if (!serial_command_available || (serial_payload_len == 0u)) {
        Serial_DisplayRawLine(1u, "No Cmd", 6u);
        Serial_DisplayClearLine(3u);
        if (serial_display_state != SERIAL_STATE_WAITING) {
            dispPrint((char *)"Waiting", 1);
        }
        serial_display_state = SERIAL_STATE_WAITING;
        serial_state_timestamp = Time_Sequence;
        return;
    }

    dispPrint((char *)"Transmit", 1);
    Serial_DisplayRawLine(1u, serial_display_buf, SERIAL_MESSAGE_LEN);
    Serial_DisplayClearLine(3u);

    for (i = 0; i < serial_payload_len; i++) {
        while (!(UCA1IFG & UCTXIFG)) {
            /* wait */
        }
        UCA1TXBUF = serial_payload[i];
    }
    while (!(UCA1IFG & UCTXIFG)) {
        /* wait */
    }
    UCA1TXBUF = '\r';
    while (!(UCA1IFG & UCTXIFG)) {
        /* wait */
    }
    UCA1TXBUF = '\n';

    serial_command_available = 0u;
    serial_payload_len = 0u;
    serial_display_state = SERIAL_STATE_TRANSMIT;
    serial_state_timestamp = Time_Sequence;
}

void Serial_Project8_ToggleBaud(void) {
    serial_baud_mode = (serial_baud_mode == 1u) ? 2u : 1u;

    Init_Serial_UCA0(serial_baud_mode);
    Init_Serial_UCA1(serial_baud_mode);

    usb_rx_message_len = 0u;

    Serial_ShowBaudOnLine3();

    if (serial_display_state != SERIAL_STATE_RECEIVED) {
        dispPrint((char *)"Waiting", 1);
        serial_display_state = SERIAL_STATE_WAITING;
        serial_state_timestamp = Time_Sequence;
    }
}

void Serial_Process_IOT_RX(void) {
    while (iot_rx_rd != iot_rx_wr) {
        (void)IOT_Ring_Rx[iot_rx_rd++];
        if (iot_rx_rd >= sizeof(IOT_Ring_Rx)) {
            iot_rx_rd = BEGINNING;
        }
    }
}

