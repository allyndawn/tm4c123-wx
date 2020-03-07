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

static uint8_t gpsDeviceDetected = 0;
static uint8_t gpsDataValid = 0;

static uint8_t gpsYear = 0;
static uint8_t gpsMonth = 0;
static uint8_t gpsDay = 0;

static uint8_t gpsHour = 0;
static uint8_t gpsMinute = 0;
static uint8_t gpsSeconds = 0;

static uint8_t gpsLatitudeDegrees = 0;
static uint8_t gpsLatitudeMinutes = 0;
static uint8_t gpsLatitudeSeconds = 0;

static uint8_t gpsLongitudeDegrees = 0;
static uint8_t gpsLongitudeMinutes = 0;
static uint8_t gpsLongitudeSeconds = 0;

uint8_t _GPS_Value_From_Scratchpad_Entry( uint8_t entry, uint8_t offset, uint8_t length ) {
	uint8_t value = 0;
	for ( uint8_t i=0; i < length; i++ ) {
		value *= 10;
		value += scratchpad[entry][offset + i] - 48; // Convert ASCII to number
	}
	return value;
}

void GPS_ProcessLine( char *data ) {

	gpsDeviceDetected = 1;

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

	gpsDataValid = ( 'A' == scratchpad[2][0] );

	gpsYear = _GPS_Value_From_Scratchpad_Entry( 9, 4, 2);
	gpsMonth = _GPS_Value_From_Scratchpad_Entry( 9, 2, 2);
	gpsDay = _GPS_Value_From_Scratchpad_Entry( 9, 0, 2);

	gpsHour = _GPS_Value_From_Scratchpad_Entry( 1, 0, 2);
	gpsMinute = _GPS_Value_From_Scratchpad_Entry( 1, 2, 2);
	gpsSeconds = _GPS_Value_From_Scratchpad_Entry( 1, 4, 2);

	gpsLatitudeDegrees = _GPS_Value_From_Scratchpad_Entry( 3, 0, 2);
	gpsLatitudeMinutes = _GPS_Value_From_Scratchpad_Entry( 3, 2, 2);
	gpsLatitudeSeconds = 0; // TODO

	gpsLongitudeDegrees = _GPS_Value_From_Scratchpad_Entry( 5, 0, 3);
	gpsLongitudeMinutes = _GPS_Value_From_Scratchpad_Entry( 5, 3, 2);
	gpsLongitudeSeconds = 0; // TODO
}

void GPS_Init() {
	UART_Init();
	UART_Register_Receive_Callback( GPS_ProcessLine );
}

uint8_t GPS_Device_Detected() {
	return gpsDeviceDetected;
}

uint8_t GPS_Data_Valid() {
	return gpsDataValid;
}

void GPS_Get_Date( uint16_t *year, uint8_t *month, uint8_t *day ) {
	if ( year ) {
		*year = gpsDataValid ? 2000 + gpsYear : 1970;
	}
	if ( month ) {
		*month = gpsDataValid ? gpsMonth : 1;
	}
	if ( day ) {
		*day = gpsDataValid ? gpsDay :1 ;
	}
}

void GPS_Get_Time( uint8_t *hour, uint8_t *minute, uint8_t *seconds ) {
	if ( hour ) {
		*hour = gpsDataValid ? gpsHour : 12;
	}
	if ( minute ) {
		*minute = gpsDataValid ? gpsMinute : 0;
	}
	if ( seconds ) {
		*seconds = gpsDataValid ? gpsSeconds : 0;
	}
}

void GPS_Get_Latitude( uint8_t *degrees, uint8_t *minutes, uint8_t *seconds, char *hemisphere ) {
	if ( degrees ) {
		*degrees = gpsDataValid ? gpsLatitudeDegrees : 12;
	}
	if ( minutes ) {
		*minutes = gpsDataValid ? gpsLatitudeMinutes : 0;
	}
	if ( seconds ) {
		*seconds = gpsDataValid ? gpsLatitudeSeconds: 0;
	}
	if ( hemisphere ) {
		*hemisphere = gpsDataValid ? scratchpad[4][0] : '-';
	}
}

void GPS_Get_Longitude( uint8_t *degrees, uint8_t *minutes, uint8_t *seconds, char *hemisphere ) {
	if ( degrees ) {
		*degrees = gpsDataValid ? gpsLongitudeDegrees : 12;
	}
	if ( minutes ) {
		*minutes = gpsDataValid ? gpsLongitudeMinutes : 0;
	}
	if ( seconds ) {
		*seconds = gpsDataValid ? gpsLongitudeSeconds: 0;
	}
	if ( hemisphere ) {
		*hemisphere = gpsDataValid ? scratchpad[6][0] : '-';
	}}

