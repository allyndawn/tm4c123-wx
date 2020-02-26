// An interface to a Maxim DS18B20 digital thermometer
// Allen Snook
// 23 February 2020

#include "ds18b20.h"
#include "onewire.h"

#include "stdio.h" // TODO REMOVE AFTER DEBUGGING

#define DS18B20_SCRATCHPAD_LENGTH 9
static uint8_t scratchpad[DS18B20_SCRATCHPAD_LENGTH];
static uint16_t scratchpadByteCount = 0;

static int16_t temperature = DS18B20_NO_READING;

void _DS18B20_Devices_Present_CB( uint8_t data ) {
	// TODO
	printf( "_DS18B20_Devices_Present_CB: %hu\n", data );
}

void _DS18B20_Update_Scratchpad( uint8_t data ) {
	if ( scratchpadByteCount >= DS18B20_SCRATCHPAD_LENGTH ) {
		return;
	}

	scratchpad[scratchpadByteCount] = data;
	scratchpadByteCount++;

	if ( DS18B20_SCRATCHPAD_LENGTH == scratchpadByteCount ) {
		// TODO - validate the CRC
		// TODO Properly calculate the temperature from bytes 0 and 1
		temperature = scratchpad[1] << 8 | scratchpad[0];
	}
}

void DS18B20_Init() {
	OneWire_Init();
}

void DS18B20_InitiateMeasurement() {
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

	// Reset the bus
	OneWire_Reset( 0 );

	// Skip ROM
	OneWire_WriteByte( 0xCC );

	// Read the scratchpad
	OneWire_WriteByte( 0xBE );

	// Read the data
	for ( uint8_t i=0; i < DS18B20_SCRATCHPAD_LENGTH; i++ ) {
		OneWire_ReadByte( _DS18B20_Update_Scratchpad );
	}
}

int16_t DS18B20_GetTempTenths() {
	return temperature;
}

