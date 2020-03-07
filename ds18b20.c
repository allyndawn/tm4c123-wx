// An interface to a Maxim DS18B20 digital thermometer
// Allen Snook
// 23 February 2020

#include "ds18b20.h"
#include "onewire.h"

#include "stdio.h" // TODO REMOVE AFTER DEBUGGING

#define DS18B20_SCRATCHPAD_LENGTH 9
static uint8_t scratchpad[DS18B20_SCRATCHPAD_LENGTH];
static uint16_t scratchpadByteCount = 0;

void _DS18B20_Devices_Present_CB( uint8_t data ) {
	// TODO
	printf( "_DS18B20_Devices_Present_CB: %hu\n", data );
}

void _DS18B20_Read_Scratchpad_Callback( uint8_t data ) {
	if ( scratchpadByteCount >= DS18B20_SCRATCHPAD_LENGTH ) {
		return;
	}

	scratchpad[scratchpadByteCount] = data;
	scratchpadByteCount++;

	if ( DS18B20_SCRATCHPAD_LENGTH == scratchpadByteCount ) {
		// TODO - validate the CRC
	}
}

void DS18B20_Init() {
	OneWire_Init();

	for ( uint8_t i=0; i < DS18B20_SCRATCHPAD_LENGTH; i++ ) {
		scratchpad[i] = 0;
	}
}

void DS18B20_Initiate_Measurement() {
	// Reset which byte we are working on
	scratchpadByteCount = 0;

	// Reset the one-wire bus
	// TODO - detect no devices
	OneWire_Reset( 0 );

	// Skip ROM
	OneWire_WriteByte( 0xCC );

	// Initiate temperature conversion
	OneWire_WriteByte( 0x44 );

	// Wait for conversion to complete
	// TODO - detect timeout
	//OneWire_WaitForHigh( 0 );
}

void DS18B20_Read_Scratchpad() {
	// Reset the bus
	OneWire_Reset( 0 );

	// Skip ROM
	OneWire_WriteByte( 0xCC );

	// Read the scratchpad
	OneWire_WriteByte( 0xBE );

	// Read the data
	for ( uint8_t i=0; i < DS18B20_SCRATCHPAD_LENGTH; i++ ) {
		OneWire_ReadByte( _DS18B20_Read_Scratchpad_Callback );
	}
}

int16_t DS18B20_Get_Temperature_F() {
	int16_t rawTemp = ( scratchpad[1] << 8 ) | (scratchpad[0] & 0xFF );
	int16_t tempC = rawTemp / 16;
	int16_t tempF = ( tempC * 9 / 5 ) + 32;
	return tempF;
}

