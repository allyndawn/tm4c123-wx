// Interrupt driven UART interface with FIFO for TM4C123
// Based on work by Jonathan Valvano
//
// Uses UART1: PC4 (U1Rx) and PC5 (U1Tx)
//
// Allen Snook
// 23 February 2020


#include "uart.h"
#include "tm4c123gh6pm.h"

#define UART_MAX_BUFFER 200
static char buffer[UART_MAX_BUFFER];
static uint16_t bytesReceived;
static uint16_t full = 0;

static void (*UART_Receive_Callback)(char *data);

void _UART_Clear_Buffer() {
	buffer[0] = 0;
	bytesReceived = 0;
}

void _UART_AddToBuffer( char data ) {
	if ( 0x0A == data ) {
		// end of line
		if ( UART_Receive_Callback ) {
			buffer[bytesReceived] = 0;
			UART_Receive_Callback( &buffer[0] );
		}

		_UART_Clear_Buffer();
		return;
	}

	if ( bytesReceived < UART_MAX_BUFFER ) {
		buffer[bytesReceived] = data;
		bytesReceived++;
	}
}

// Initialize UART1
// Follows the steps recommended in the Tiva Datasheet, pg. 902, sec. 14.4
void UART_Init() {
	_UART_Clear_Buffer();

	SYSCTL_RCGCUART_R |= 0x0002;			// Enable UART1 (pg. 344)
	SYSCTL_RCGCGPIO_R |= 0x0004;			// Enable GPIO clocks on Port C (pg. 341)
	// TODO WAIT?

	GPIO_PORTC_AFSEL_R |= 0x30;				// Enable alt function on C4 and 5 (pg. 671)

	// Set the Port Mux Control for PC4 as U1RX (2) and PC5 as UITX (2) (pg. 1351)
	GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & 0xFF00FFFF) | 0x00220000;

	GPIO_PORTC_DEN_R |= 0x30;				// Enable digital on C4 and C5

	// Set the baud rate (9600) (assumes system clock of 16 MHz)
	UART1_CTL_R &= ~UART_CTL_UARTEN;		// Disable the UART
	UART1_IBRD_R = 104;						// IBRD = int(16,000,000 / (16 * 9600)) = int(104.1666)
	UART1_FBRD_R = 11;						// FBRD = int(0.1666 * 64 + 0.5) = 11

	UART1_LCRH_R = 0x70;					// 8N1 + FIFo (pg. 916)

	// We want an interrupt when the RX FIFO is > 1/4 full
	UART1_IFLS_R = (UART1_IFLS_R & 0xFFFFFFC0) | 0x0000009;
	UART1_IM_R = UART_IM_RXIM;

	// Enable the UART
	UART1_CTL_R |= UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN;

	// UART1 / IRQ6 / NVIC_PRI1_R / b23-21 / Priority 2
	NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF0FFFFFF) | 0x00400000;
	NVIC_EN0_R = 1 << 6;
}

void UART_Register_Receive_Callback( void (*callback)(char *data) ) {
	UART_Receive_Callback = callback;
}

void UART_OutChar( char data ) {
	// Spin if TX FIFO full
	while ( UART1_FR_R & UART_FR_TXFF ) {};
	UART1_DR_R = data;
}

// Outputs a line of data to the device
void UART_OutString( char *data ) {
	while ( *data ) {
		UART_OutChar( *data );
	}
}

// Handles UART interrupt events, IRQ6
void UART1_Handler() {
	if ( UART1_RIS_R & UART_RIS_RXRIS ) {
		UART1_ICR_R = UART_ICR_RXIC;	// Acknowledge the interrupt
		while (( UART1_FR_R & UART_FR_RXFE) == 0 ) {
			_UART_AddToBuffer( UART1_DR_R );
		}
	}
}
