// Maxim/Dallas One Wire Interface for TM4C123
//
// Uses PB7
// Uses Timer0A
//
// Allen Snook
// 23 February 2020

#include "onewire.h"
#include "tm4c123gh6pm.h"

#include "stdio.h" // TODO REMOVE AFTER DEBUGGING

#define ONEWIRE_MAX_TASKS 500

#define ONEWIRE_MIN_WAIT_MICROSECONDS 5

#define ONEWIRE_BUS_LOW 0
#define ONEWIRE_RELEASE_BUS 0x1
#define ONEWIRE_SAMPLE_BUS 0x2
#define ONEWIRE_TRANSFER_BYTE 0x3
#define ONEWIRE_WAIT_BUS_HIGH 0x4

#define PB7 (*((volatile uint32_t *)0x40005200))

typedef struct OneWire_Tasks {
	uint16_t type;
	uint16_t holdTime;
	void (*callback)(uint8_t);
} OneWire_Task;

static OneWire_Task tasks[ONEWIRE_MAX_TASKS];
static uint16_t taskCount = 0;
static uint16_t currentTaskIndex = 0;

void _OneWire_AddTask( uint8_t type, uint16_t holdTime, void (*callback)(uint8_t data) ) {
	if ( taskCount >= ONEWIRE_MAX_TASKS ) {
		return;
	}

	tasks[taskCount].type = type;
	tasks[taskCount].holdTime = holdTime;
	tasks[taskCount].callback = callback;
	taskCount++;
}

void _OneWire_Wait( uint32_t microseconds ) {
	volatile unsigned long delay;

	if ( microseconds < ONEWIRE_MIN_WAIT_MICROSECONDS ) {
		microseconds = ONEWIRE_MIN_WAIT_MICROSECONDS;
	}

	// Activate timer0
	SYSCTL_RCGCTIMER_R |= 0x01;

	// Wait for the timer to settle
	// TODO Is there a better way to do this?
	delay = SYSCTL_RCGCTIMER_R;

	// Disable timer during setup
	TIMER0_CTL_R &= ~TIMER_CTL_TAEN;

	// Configure for 32-bit timer mode
	TIMER0_CFG_R = 0;

	// Configure for a one shot timer
	TIMER0_TAMR_R = TIMER_TAMR_TAMR_1_SHOT;

	// No prescaling
	TIMER0_TAPR_R = 0;

	// Assuming a 16 MHz clock, set the initial counting value
	TIMER0_TAILR_R = microseconds * 16;

	// Enable timeout (rollover) interrupt
	TIMER0_IMR_R |= TIMER_IMR_TATOIM;

	// Clear any lingering timer timeout flag
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;

	// Enable the countdown
	TIMER0_CTL_R |= TIMER_CTL_TAEN;

	// Set timer priority
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x00FFFFFF) | 0x40000000;

	// Timer 0A uses interrupt 19 - enable it in NVIC
	NVIC_EN0_R = 1 << 19;
}

void _OneWire_Bus_Low() {
	// Set PB7 as output
	GPIO_PORTB_DIR_R |= 0x80;

	// Pull PB7 low
	PB7 = 0;
}

void _OneWire_Release_Bus() {
	// Set PB7 as input
	GPIO_PORTB_DIR_R &= ~0x80;
}

uint8_t _OneWire_Sample_Bus() {
	////if ( PB7 ) {
	////	return 1;
	////}
	return 0;
}

void _OneWire_Write_0_Bit() {
	_OneWire_AddTask( ONEWIRE_BUS_LOW, 60, 0 );
	_OneWire_AddTask( ONEWIRE_RELEASE_BUS, 10, 0 );
}

void _OneWire_Write_1_Bit() {
	_OneWire_AddTask( ONEWIRE_BUS_LOW, 5, 0 );
	_OneWire_AddTask( ONEWIRE_RELEASE_BUS, 65, 0 );
}

void OneWire_Init( void ) {
	SYSCTL_RCGCGPIO_R |= 0x02;			// Activate Port B

	// Wait for clock to settle
	while ((SYSCTL_PRGPIO_R & 0x02) != 0x02)
	{
	}

	GPIO_PORTB_DIR_R &= ~0x80;			// Set PB7 as input
	GPIO_PORTB_DEN_R |= 0x80;			// Enable digital I/O for PB7
	GPIO_PORTB_PUR_R |= 0x80;			// Enable weak pull up for PB7

	// Start task queue processing
	_OneWire_Wait( 1000 );
}

void OneWire_Timer0A_Handler() {
	// Acknowledge the interrupt (Timer0A)
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;

	if ( 0 == taskCount ) {
		// If no tasks in the queue, go back to sleep for 1 ms
		_OneWire_Wait( 1000 );
		return;
	}

	// Get the next task to run
	OneWire_Task currentTask = tasks[ currentTaskIndex ];
	// Start the next one shot right away
	_OneWire_Wait( currentTask.holdTime );
	// Set up for next task now
	currentTaskIndex++;
	if ( currentTaskIndex >= taskCount ) {
		taskCount = 0;
		currentTaskIndex = 0;
	}


	static uint8_t shiftRegister = 0;
	uint8_t sample = 0;

	if ( currentTask.type == ONEWIRE_BUS_LOW ) {
		_OneWire_Bus_Low();
	}

	if ( currentTask.type == ONEWIRE_RELEASE_BUS ) {
		_OneWire_Release_Bus();
	}

	if ( currentTask.type == ONEWIRE_SAMPLE_BUS ) {
		shiftRegister = shiftRegister >> 1;
		sample = _OneWire_Sample_Bus();
		if ( sample ) {
			shiftRegister |= 0x80;
		}
		if ( currentTask.callback ) {
			currentTask.callback( sample );
		}
	}

	if ( currentTask.type == ONEWIRE_TRANSFER_BYTE ) {
		if ( currentTask.callback ) {
			currentTask.callback( shiftRegister );
		}
	}

	// TODO BUS HIGH
}

void OneWire_Reset( void (*callback)(uint8_t data) ) {
	_OneWire_AddTask( ONEWIRE_BUS_LOW, 480, 0 );
	_OneWire_AddTask( ONEWIRE_RELEASE_BUS, 70, 0 );
	_OneWire_AddTask( ONEWIRE_SAMPLE_BUS, 410, callback );
}

void OneWire_WriteByte( uint8_t data ) {
	for ( uint8_t i=0; i < 8; i++ ) {
		if ( data & 0x1 ) {
			_OneWire_Write_1_Bit();
		} else {
			_OneWire_Write_0_Bit();
		}
		data = data >> 1;
	}
}

void OneWire_ReadByte( void (*callback)(uint8_t data) ) {
	for ( uint8_t i=0; i < 8; i++ ) {
		_OneWire_AddTask( ONEWIRE_BUS_LOW, 5, 0 );
		_OneWire_AddTask( ONEWIRE_RELEASE_BUS, 10, 0 );
		_OneWire_AddTask( ONEWIRE_SAMPLE_BUS, 55, 0 );
	}
	_OneWire_AddTask( ONEWIRE_TRANSFER_BYTE, 0, callback );
}

void OneWire_WaitForHigh( void (*callback)(uint8_t data) ) {
	_OneWire_AddTask( ONEWIRE_WAIT_BUS_HIGH, 1000, callback );
}