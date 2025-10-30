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


//extern char NCSU[];
extern volatile unsigned int ncsu_index;
//extern volatile char iot_TX_buf[11];
extern volatile char process_buf[25];
extern char process_buffer[25]; // Size for appropriate Command Length

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
        UCA0BRW = 17;                        // 460,800 baud
        UCA0MCTLW = 0x4A00;
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
        UCA1BRW = 17;                        // 460,800 baud @ 8MHz, UCOS16=0, UCBRS=0x4A
        UCA1MCTLW = 0x4A00;                  // UCBRS=0x4A, UCOS16=0
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

//------------------------------------------------------------------------------
// Drain UCA1 RX ring (filled by ISR) and show latest line on LCD line 1
//------------------------------------------------------------------------------
extern char display_line[4][11];
extern volatile unsigned char display_changed;

void Serial_Process_USB_RX(void) {
    static char linebuf[11];
    static unsigned int idx = 0;

    // Failsafe: also poll hardware RX flag in case interrupts aren't firing
    // This will be a no-op if the ISR is running and has already drained RXBUF.
    while (UCA1IFG & UCRXIFG) {
        char c = UCA1RXBUF; // reading clears RXIFG
        USB_Ring_Rx[usb_rx_ring_wr++] = c;
        if (usb_rx_ring_wr >= sizeof(USB_Ring_Rx)) {
            usb_rx_ring_wr = BEGINNING;
        }
    }

    // Drain any received bytes
    while (usb_rx_ring_rd != usb_rx_ring_wr) {
        char c = USB_Ring_Rx[usb_rx_ring_rd++];
        if (usb_rx_ring_rd >= sizeof(USB_Ring_Rx)) {
            usb_rx_ring_rd = BEGINNING;
        }

        if (c == '\0') {
            continue; // Ignore stray NULs so display strings stay intact
        }

        if (c == '\r' || c == '\n') {
            // Terminate and display the accumulated line
            linebuf[idx] = '\0';
            // Center using Display helper; but here copy directly to display_line[0]
            // Ensure 10-char field padded with spaces
            char temp[11];
            int len = (int)strlen(linebuf);
            if (len > 10) len = 10;
            int spaces = (10 - len) >> 1;
            {
                unsigned int i;
                for (i = 0; i < 10; i++) temp[i] = ' ';
            }
            temp[10] = '\0';
            memcpy(&temp[spaces], linebuf, len);
            strcpy(display_line[0], temp);
            display_changed = 1;
            idx = 0;
        } else {
            if (idx < 10) {
                linebuf[idx++] = c;
                // Live update as characters arrive (helps when no newline is sent)
                char temp[11];
                int len = (int)idx;
                if (len > 10) len = 10;
                int spaces = (10 - len) >> 1;
                {
                    unsigned int i;
                    for (i = 0; i < 10; i++) temp[i] = ' ';
                }
                temp[10] = '\0';
                memcpy(&temp[spaces], linebuf, len);
                strcpy(display_line[0], temp);
                display_changed = 1;
            } else {
                // If overflow, show immediately and reset
                linebuf[10] = '\0';
                strcpy(display_line[0], linebuf);
                display_changed = 1;
                idx = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------
// Drain UCA0 RX ring (filled by ISR) and show latest line on LCD line 2
//------------------------------------------------------------------------------
extern char display_line[4][11];
extern volatile unsigned char display_changed;

void Serial_Process_IOT_RX(void) {
    static char linebuf0[11];
    static unsigned int idx0 = 0;

    // Drain any received bytes from UCA0 ring
    while (iot_rx_rd != iot_rx_wr) {
        char c = IOT_Ring_Rx[iot_rx_rd++];
        if (iot_rx_rd >= sizeof(IOT_Ring_Rx)) {
            iot_rx_rd = BEGINNING;
        }

        if (c == '\0') {
            continue; // Skip NULs that are not part of displayable text
        }

        if (c == '\r' || c == '\n') {
            // Terminate and display the accumulated line on LCD line 2
            linebuf0[idx0] = '\0';
            char temp[11];
            int len = (int)strlen(linebuf0);
            if (len > 10) len = 10;
            int spaces = (10 - len) >> 1;
            {
                unsigned int i;
                for (i = 0; i < 10; i++) temp[i] = ' ';
            }
            temp[10] = '\0';
            memcpy(&temp[spaces], linebuf0, len);
            strcpy(display_line[1], temp); // line 2
            display_changed = 1;
            idx0 = 0;
        } else {
            if (idx0 < 10) {
                linebuf0[idx0++] = c;
                // Live update
                char temp[11];
                int len = (int)idx0;
                if (len > 10) len = 10;
                int spaces = (10 - len) >> 1;
                {
                    unsigned int i;
                    for (i = 0; i < 10; i++) temp[i] = ' ';
                }
                temp[10] = '\0';
                memcpy(&temp[spaces], linebuf0, len);
                strcpy(display_line[1], temp);
                display_changed = 1;
            } else {
                // Overflow: show immediately and reset
                linebuf0[10] = '\0';
                strcpy(display_line[1], linebuf0);
                display_changed = 1;
                idx0 = 0;
            }
        }
    }
}

