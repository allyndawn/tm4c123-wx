// An interface to a Maxim DS18B20 digital thermometer
// Allen Snook
// 23 February 2020

#include "ds18b20.h"
#include "tm4c123gh6pm.h"

#define DS18B20_MAX_TASKS 250

#define OW_BUS_LOW 0
#define OW_RELEASE_BUS 1
#define OW_SAMPLE_BUS 2
#define OW_WAIT 3
#define OW_CLEAR_SHIFT_REG 4
#define OW_TRANSFER_BYTE 5
#define OW_WAIT_BUS_HIGH 6

#define DS18B20_SCRATCHPAD_LENGTH 9

#define PB7 (*((volatile uint32_t *)0x40005200))

typedef struct Tasks {
	uint16_t type;
	uint16_t time;
	void (*callback)(uint8_t);
} Task;

static Task tasks[DS18B20_MAX_TASKS];
static uint16_t taskCount = 0;

static uint16_t timeInTask = 0;

static uint8_t shiftRegister = 0;

static uint8_t scratchpad[DS18B20_SCRATCHPAD_LENGTH];
static uint16_t scratchpadByteCount = 0;

static int16_t temperature;

void _shiftTasksArray() {
	if ( 0 == taskCount ) {
		return;
	}

	if ( 1 < taskCount ) {
		for (uint16_t i=0; i < taskCount - 1; i++ ) {
			tasks[i] = tasks[i+1];
		}
	}

	taskCount--;
}

void _OW_AddTask( uint8_t type, uint16_t time, void (*callback)(uint8_t data) ) {
	if ( taskCount >= DS18B20_MAX_TASKS ) {
		return;
	}

	tasks[taskCount].type = type;
	tasks[taskCount].time = time;
	tasks[taskCount].callback = callback;
	taskCount++;
}

void devicesPresentCallback( uint8_t data ) {
}

void DS18B20_Init( void ) {
	SYSCTL_RCGCGPIO_R |= 0x02;			// Activate Port B

	// Wait for clock to settle
	while ((SYSCTL_PRGPIO_R & 0x02) != 0x02)
	{
	}

	GPIO_PORTB_DIR_R &= ~0x80;			// Set PB7 as input
	GPIO_PORTB_DEN_R |= 0x80;			// Enable digital I/O for PB7
	GPIO_PORTB_PUR_R |= 0x11;			// Enable weak pull up for PB7 // TODO check hex here

	temperature = DS18B20_NO_READING;
}

void _OW_Bus_Low() {
	GPIO_PORTB_DIR_R &= ~0x80;			// Set PB7 as output
	PB7 = 0;
}

void _OW_Release_Bus() {
	GPIO_PORTB_DIR_R &= ~0x80;			// Set PB7 as input
}

uint8_t _OW_Sample_Bus() {
	if ( PB7 ) {
		return 1;
	}
	return 0;
}

void DS18B20_TickHandler( void ) {
	if ( 0 == taskCount ) {
		return;
	}

	// We always work on task 0

	// If this is our first time working on this task
	// do its thing, if any
	if ( 0 == timeInTask ) {
		if ( OW_CLEAR_SHIFT_REG == tasks[0].type ) {
			shiftRegister = 0x0;
		}

		if ( OW_BUS_LOW == tasks[0].type ) {
			_OW_Bus_Low();
		}

		if ( OW_RELEASE_BUS == tasks[0].type ) {
			_OW_Release_Bus();
		}

		if ( OW_SAMPLE_BUS == tasks[0].type ) {
			shiftRegister = shiftRegister >> 1;
			if ( _OW_Sample_Bus() ) {
				shiftRegister |= 0x80;
			}
		}

		if ( OW_WAIT == tasks[0].type ) {
			// NO-OP
		}

		if ( OW_TRANSFER_BYTE == tasks[0].type ) {
			tasks[0].callback( shiftRegister );
		}

		if ( OW_WAIT_BUS_HIGH == tasks[0].type ) {
			// TODO
		}
	}

	timeInTask++;

	// Have we spent long enough?
	if ( tasks[0].time <= timeInTask ) {
		timeInTask = 0;
		_shiftTasksArray();
	}
}

void DS18B20_Device_Present_Callback( uint8_t data ) {
	// TODO
}

void DS18B20_Conversion_Timeout_Callback( uint8_t data ) {
	// TODO
}

void OW_Reset() {
	_OW_AddTask( OW_BUS_LOW, 480, 0 );
	_OW_AddTask( OW_RELEASE_BUS, 70, 0 );
	_OW_AddTask( OW_SAMPLE_BUS, 0, DS18B20_Device_Present_Callback );
	_OW_AddTask( OW_WAIT, 410, 0 );
}

void OW_Write_0_Bit() {
	_OW_AddTask( OW_BUS_LOW, 60, 0 );
	_OW_AddTask( OW_RELEASE_BUS, 10, 0 );
}

void OW_Write_1_Bit() {
	_OW_AddTask( OW_BUS_LOW, 6, 0 );
	_OW_AddTask( OW_RELEASE_BUS, 64, 0 );
}

void OW_WriteByte( uint8_t data ) {
	for ( uint8_t i=0; i < 8; i++ ) {
		if ( data & 0x1 ) {
			OW_Write_1_Bit();
		} else {
			OW_Write_0_Bit();
		}
		data = data >> 1;
	}
}

void OW_ReadByte( void (*callback)(uint8_t data) ) {
	for ( uint8_t i=0; i < 8; i++ ) {
		_OW_AddTask( OW_BUS_LOW, 6, 0 );
		_OW_AddTask( OW_RELEASE_BUS, 9, 0 );
		_OW_AddTask( OW_SAMPLE_BUS, 0, 0 ); // samples it and shifts it into a temporary shift register
		_OW_AddTask( OW_WAIT, 55, 0 );
	}
	_OW_AddTask( OW_TRANSFER_BYTE, 0, callback );
}

void OW_WaitForHigh() {
	_OW_AddTask( OW_WAIT_BUS_HIGH, 1000, DS18B20_Conversion_Timeout_Callback );
}

void updateScratchpad( uint8_t data ) {
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

void DS18B20_InitiateMeasurement( void ) {
	// Reset which byte we are working on
	scratchpadByteCount = 0;

	// Reset the one-wire bus
	OW_Reset();

	// Skip ROM
	OW_WriteByte( 0xCC );

	// Initiate temperature conversion
	OW_WriteByte( 0x44 );

	// Wait for conversion to complete
	OW_WaitForHigh();

	// Reset the bus
	OW_Reset();

	// Skip ROM
	OW_WriteByte( 0xCC );

	// Read the scratchpad
	OW_WriteByte( 0xBE );

	// Read the data
	for ( uint8_t i=0; i < DS18B20_SCRATCHPAD_LENGTH; i++ ) {
		OW_ReadByte( updateScratchpad );
	}
}

int16_t DS18B20_GetTempTenths() {
	return temperature;
}

