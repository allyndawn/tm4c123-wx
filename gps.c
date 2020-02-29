// Neo6 GPS Interface for TM4C123
//
// Uses PC4 (U1Rx) and PC5 (U1Tx)
//
// Allen Snook
// 23 February 2020

#include "gps.h"
#include "uart.h"
#include "string.h"

#define GPS_GPRMC_TOKENS 12
#define GPS_GPRMC_MAX_TOKEN_LENGTH 12
char scratchpad[GPS_GPRMC_TOKENS][GPS_GPRMC_MAX_TOKEN_LENGTH];

void GPS_ProcessLine( char *data ) {

	// Is it at least 16  but not more than 66 characters long?
	uint16_t length = strlen( data );
	if ( ( length < 16 ) | ( length > 66 ) ) {
		return;
	}

	// Does it start with $GPRMC
	if ( ( '$' != data[0] ) | ( 'G' != data[1] ) | ( 'P' != data[2] ) |
		( 'R' != data[3] ) | ( 'M' != data[4] ) | ( 'C' != data[5] ) ) {
		return;
	}

	// Does it have exactly 12 tokens?
	uint16_t commaCount = 0;
	for ( uint16_t i=0; i < length; i++ ) {
		if ( ',' == data[i] ) {
			commaCount++;
		}
	}
	if ( GPS_GPRMC_TOKENS != commaCount ) {
		return;
	}

	// OK, tokenize it
	// First, clear the scratchpad
	for ( uint16_t i=0; i < GPS_GPRMC_TOKENS; i++ ) {
		scratchpad[i][0] = 0;
	}

	uint8_t tokenIndex = 0;

	// Now, tokenize into the scratchpad
	char dataChar[2];
	dataChar[0] = 0;
	dataChar[1] = 0;

	for ( uint16_t i=0; i < length; i++ ) {
		if ( ',' == data[i] ) {
			tokenIndex++;
		} else if (strlen( scratchpad[tokenIndex] ) < GPS_GPRMC_MAX_TOKEN_LENGTH - 1 ) {
			dataChar[0] = data[i];
			strcat( scratchpad[tokenIndex], dataChar );
		}
	}
}

void GPS_Init() {
	UART_Init();
	UART_Register_Receive_Callback( GPS_ProcessLine );
}

uint8_t GPS_Is_Ready() {
	return ( 'A' == scratchpad[2][0] );
}

void GPS_Get_DateTime( char *dateString, char *timeString ) {
	strcpy( dateString, scratchpad[9] );
	strcpy( timeString, scratchpad[1] );
}

void GPS_Get_Location( char *latString, char *latHem, char *longString, char *longHem ) {
	strcpy( latString, scratchpad[3] );
	strcpy( latHem, scratchpad[4] );
	strcpy( longString, scratchpad[5] );
	strcpy( longHem, scratchpad[6] );
}