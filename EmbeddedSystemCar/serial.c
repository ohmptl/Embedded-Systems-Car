//------------------------------------------------------------------------------
//  Name:           init.c
//  Description:    Init file
//  Author:         Ohm Patel
//  Date:           Oct 2025
//  IDE:            CCS20.3.0
//------------------------------------------------------------------------------

#include "ports.h"
#include "msp430.h"
#include "macros.h"
#include "serial.h"

// External timer tick used for RX idle detection
extern volatile unsigned int Time_Sequence;

// Baud rate settings for 8MHz SMCLK (see MSP430FR2355 User's Guide table 22-5)
#define BAUD_115200_BRW           (4u)
#define BAUD_115200_MCTLW         (0x5551u)
#define BAUD_460800_BRW           (17u)
#define BAUD_460800_MCTLW         (0x4A00u)

#define SERIAL_IDLE_TICKS         (3u)      // 0.6s of idle (3 * 0.2s)

#define UART_RING_SIZE            (64u)

static volatile char uca0_rx_ring[UART_RING_SIZE];
static volatile unsigned int uca0_rx_head;
static volatile unsigned int uca0_rx_tail;

static volatile char uca1_rx_line[SERIAL_RX_LINE_LENGTH];
static volatile unsigned int uca1_rx_index;
static volatile unsigned int uca1_last_rx_tick;
static volatile unsigned char uca1_line_ready;

static unsigned long serial_current_baud = 115200u;

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------
static void serial_configure_uca0(unsigned long baud) {
  UCA0CTLW0 = UCSWRST;                  // put module in reset
  UCA0CTLW0 |= UCSSEL__SMCLK;           // SMCLK source
  UCA0CTLW0 &= ~(UCMSB | UCSPB | UCPEN | UCSYNC | UC7BIT);
  UCA0CTLW0 |= UCMODE_0;                // UART mode

  if (baud == 460800u) {
    UCA0BRW = BAUD_460800_BRW;
    UCA0MCTLW = BAUD_460800_MCTLW;
  } else {
    UCA0BRW = BAUD_115200_BRW;
    UCA0MCTLW = BAUD_115200_MCTLW;
  }

  UCA0CTLW0 &= ~UCSWRST;               // release for operation
  UCA0IFG &= ~(UCRXIFG | UCTXIFG);
  UCA0IE = UCRXIE;                      // RX interrupt only
  UCA0TXBUF = 0x00;                     // prime the pump (sets TXIFG)
}

static void serial_configure_uca1(unsigned long baud) {
  UCA1CTLW0 = UCSWRST;
  UCA1CTLW0 |= UCSSEL__SMCLK;
  UCA1CTLW0 &= ~(UCMSB | UCSPB | UCPEN | UCSYNC | UC7BIT);
  UCA1CTLW0 |= UCMODE_0;

  if (baud == 460800u) {
    UCA1BRW = BAUD_460800_BRW;
    UCA1MCTLW = BAUD_460800_MCTLW;
  } else {
    UCA1BRW = BAUD_115200_BRW;
    UCA1MCTLW = BAUD_115200_MCTLW;
  }

  UCA1CTLW0 &= ~UCSWRST;
  UCA1IFG &= ~(UCRXIFG | UCTXIFG);
  UCA1IE = UCRXIE;                      // RX interrupt only
  UCA1TXBUF = 0x00;
}

static void serial_reset_buffers(void) {
  unsigned int i;
  uca0_rx_head = 0;
  uca0_rx_tail = 0;
  for (i = 0; i < UART_RING_SIZE; i++) {
    uca0_rx_ring[i] = 0;
  }

  uca1_rx_index = 0;
  uca1_rx_line[0] = '\0';
  uca1_line_ready = 0;
  uca1_last_rx_tick = Time_Sequence;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
void Serial_InitAll(unsigned long baud) {
  serial_current_baud = baud;
  serial_reset_buffers();
  serial_configure_uca0(baud);
  serial_configure_uca1(baud);
}

void Serial_SetBaudAll(unsigned long baud) {
  serial_current_baud = baud;
  serial_configure_uca0(baud);
  serial_configure_uca1(baud);
  Serial_ClearUCA1Rx();
}

unsigned long Serial_GetCurrentBaud(void) {
  return serial_current_baud;
}

void Serial_SendStringUCA0(const char *str) {
  if (!str) {
    return;
  }
  while (*str) {
    while (!(UCA0IFG & UCTXIFG)) {
      ;
    }
    UCA0TXBUF = *str++;
  }
}

void Serial_SendStringUCA1(const char *str) {
  if (!str) {
    return;
  }
  while (*str) {
    while (!(UCA1IFG & UCTXIFG)) {
      ;
    }
    UCA1TXBUF = *str++;
  }
}

void Serial_ClearUCA1Rx(void) {
  __disable_interrupt();
  uca1_rx_index = 0;
  uca1_rx_line[0] = '\0';
  uca1_line_ready = 0;
  uca1_last_rx_tick = Time_Sequence;
  __enable_interrupt();
}

unsigned char Serial_UCA1LineReady(void) {
  return uca1_line_ready;
}

void Serial_CopyUCA1Line(char *dest, unsigned int dest_len) {
  unsigned int i;
  if (!dest || dest_len == 0) {
    return;
  }

  __disable_interrupt();
  for (i = 0; i < dest_len - 1 && i < uca1_rx_index; i++) {
    dest[i] = uca1_rx_line[i];
  }
  if (i < dest_len) {
    dest[i] = '\0';
  }
  uca1_rx_index = 0;
  uca1_rx_line[0] = '\0';
  uca1_line_ready = 0;
  __enable_interrupt();
}

void Serial_Service(void) {
  if (!uca1_line_ready && uca1_rx_index) {
    unsigned int delta = (unsigned int)(Time_Sequence - uca1_last_rx_tick);
    if (delta >= SERIAL_IDLE_TICKS) {
      uca1_line_ready = 1;
    }
  }
}

//------------------------------------------------------------------------------
// ISR hooks
//------------------------------------------------------------------------------
void Serial_HandleUCA0Rx(char byte_in) {
  unsigned int next_head = (uca0_rx_head + 1u) % UART_RING_SIZE;
  uca0_rx_ring[uca0_rx_head] = byte_in;
  uca0_rx_head = next_head;
  if (uca0_rx_head == uca0_rx_tail) {
    uca0_rx_tail = (uca0_rx_tail + 1u) % UART_RING_SIZE; // drop oldest
  }

  // Echo locally for loopback verification
  if (UCA0IFG & UCTXIFG) {
    UCA0TXBUF = byte_in;
  }

  // Forward to PC back channel to monitor IoT traffic
  if (UCA1IFG & UCTXIFG) {
    UCA1TXBUF = byte_in;
  }
}

void Serial_HandleUCA1Rx(char byte_in) {
  if (uca1_rx_index < (SERIAL_RX_LINE_LENGTH - 1u)) {
    uca1_rx_line[uca1_rx_index++] = byte_in;
    uca1_rx_line[uca1_rx_index] = '\0';
  } else {
    // Saturate buffer and keep last characters visible
    unsigned int i;
    for (i = 0; i < SERIAL_RX_LINE_LENGTH - 2u; i++) {
      uca1_rx_line[i] = uca1_rx_line[i + 1u];
    }
    uca1_rx_line[SERIAL_RX_LINE_LENGTH - 2u] = byte_in;
    uca1_rx_line[SERIAL_RX_LINE_LENGTH - 1u] = '\0';
    uca1_rx_index = SERIAL_RX_LINE_LENGTH - 1u;
  }

  uca1_last_rx_tick = Time_Sequence;
  uca1_line_ready = 0;

  // Forward received data to UCA0 and echo locally for PC terminal visibility
  if (UCA0IFG & UCTXIFG) {
    UCA0TXBUF = byte_in;
  }
  if (UCA1IFG & UCTXIFG) {
    UCA1TXBUF = byte_in;
  }
}
