// Neo6 GPS Interface for TM4C123
//
// Uses PC4 (U1Rx) and PC5 (U1Tx)
//
// Allen Snook
// 23 February 2020

#include "gps.h"
#include "uart.h"
#include "stdio.h"

void GPS_ProcessLine( char *data ) {
	printf( "%s\n", data );
}

void GPS_Init() {
	UART_Init();
	UART_Register_Receive_Callback( GPS_ProcessLine );
}