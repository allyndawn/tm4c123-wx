// Interrupt driven combo PWM I2C interface TM4C123
// Based on work by Jonathan Valvano
//
// Useful for I2C devices that can also accept
// PWM inputs, like radios or other audio devices
//
// Uses Timer2A for one-shots
// Uses I2C0: PB2 (SCL), PB3 (SDA)
// Uses PB4 as nCS
// Uses PB6 as PWM
//
// Allen Snook
// 2 March 2020

#include "pwm-i2c.h"
#include "tm4c123gh6pm.h"

#define PB4 (*((volatile uint32_t *)0x40005040))

#define PWM_I2C_MAX_COMMANDS 100
#define PWM_I2C_MIN_WAIT_MILLISECONDS 1

typedef struct PWM_I2C_Commands {
	uint16_t address;
	uint16_t data;
	uint16_t mask;
	uint16_t waitMS;
} PWM_I2C_Command;

void (*pwm_i2c_callback)();

static PWM_I2C_Command commands[PWM_I2C_MAX_COMMANDS];
static uint16_t commandCount = 0;
static uint16_t currentCommandIndex = 0;

/*
 * Sets up Timer2A as a one-shot timer
 * and requests an interrupt when the time expires
 */
void _PWM_I2C_Sleep( uint16_t milliseconds ) {
	volatile unsigned long delay;

	if ( milliseconds < PWM_I2C_MIN_WAIT_MILLISECONDS ) {
		milliseconds = PWM_I2C_MIN_WAIT_MILLISECONDS;
	}

	// Activate Timer2
	SYSCTL_RCGCTIMER_R |= 0x04;

	// Wait for the timer to settle
	// TODO Is there a better way to do this?
	delay = SYSCTL_RCGCTIMER_R;

	// Disable timer during setup
	TIMER2_CTL_R &= ~TIMER_CTL_TAEN;

	// Configure for 32-bit timer mode
	TIMER2_CFG_R = 0;

	// Configure for a one shot timer
	TIMER2_TAMR_R = TIMER_TAMR_TAMR_1_SHOT;

	// No prescaling
	TIMER2_TAPR_R = 0;

	// Assuming a 16 MHz clock, set the initial counting value
	TIMER2_TAILR_R = milliseconds * 16000;

	// Enable timeout (rollover) interrupt
	TIMER2_IMR_R |= TIMER_IMR_TATOIM;

	// Clear any lingering timer timeout flag
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;

	// Enable the countdown
	TIMER2_CTL_R |= TIMER_CTL_TAEN;

	// Set timer priority
	// Timer2A is NVIC 5, b:31-29
	NVIC_PRI5_R = (NVIC_PRI5_R & 0x00FFFFFF) | 0x40000000;

	// Timer 2A uses interrupt 23
	NVIC_EN0_R = 1 << 23;
}

/*
 * Read a 16-b word from the I2C device at address
 * Based on Valvano p.376
 *
 */
uint16_t PWM_I2C_Read( uint8_t address ) {
	// TODO Handle errors, retries

	// Wait until ready
	while ( I2C0_MCS_R & 0x00000001 ) {};

	I2C0_MSA_R = ( address << 1 ) & 0xFE;
	I2C0_MSA_R |= 0x01;
	I2C0_MCS_R = ( I2C_MCS_ACK | I2C_MCS_START | I2C_MCS_RUN );

	// Wait until ready
	while ( I2C0_MCS_R & 0x00000001 ) {};

	uint16_t data = ( I2C0_MDR_R & 0xFF );
	data = data << 8;

	I2C0_MCS_R = ( I2C_MCS_STOP | I2C_MCS_RUN );

	// Wait until ready
	while ( I2C0_MCS_R & 0x00000001 ) {};

	data |= ( I2C0_MDR_R & 0xFF );

	return data;
}

/*
 * Write a 16-b word to the I2C device at address
 * Based on Valvano p.375
 */
void _PWM_I2C_Write( uint8_t address, uint16_t data, uint16_t mask ) {
	uint16_t newValue = data;

	// Has a mask? Read the register first
	if ( 0xFFFF != mask ) {
		uint16_t currentValue = PWM_I2C_Read( address );
		newValue = ( currentValue & mask ) | data;
	}

	// Wait until ready
	while ( I2C0_MCS_R & 0x00000001 ) {};

	// Transmit the first 8 bits
	I2C0_MSA_R = ( address << 1 ) & 0xFE;
	I2C0_MSA_R &= ~0x01;
	I2C0_MDR_R = ( newValue >> 8 ) & 0xFF;
	I2C0_MCS_R = ( I2C_MCS_START | I2C_MCS_RUN );

	// Wait until ready
	while ( I2C0_MCS_R & 0x00000001 ) {};

	// TODO Check, handle errors

	// Transmit the second 8
	I2C0_MDR_R = newValue & 0xFF;
	I2C0_MCS_R = ( I2C_MCS_STOP | I2C_MCS_RUN );

	// Wait until transmission done
	while ( I2C0_MCS_R & 0x00000001 ) {};

	// TODO Check, handle errors
}

/*
 * Grabs the next command in the queue, performs it
 */
void PWM_I2C_Timer2A_Handler() {
	// Acknowledge the interrupt
		TIMER2_ICR_R = TIMER_ICR_TATOCINT;

	// No commands? Just return
	if ( 0 == commandCount ) {
		return;
	}

	// Grab the next command in the queue
	// If no more commands, reset the queue and call the callback
	if ( currentCommandIndex >= commandCount ) {
		commandCount = 0;
		currentCommandIndex = 0;
		if ( pwm_i2c_callback ) {
			pwm_i2c_callback();
			return;
		}
	}

	PWM_I2C_Command currentCommand = commands[ currentCommandIndex ];

	// Otherwise, do the command
	_PWM_I2C_Write( currentCommand.address, currentCommand.data, currentCommand.mask );

	// Increment the command index
	currentCommandIndex++;

	// Arm the one-shot
	_PWM_I2C_Sleep( currentCommand.waitMS );
}

/*
 * Adds a command to the queue
 * If this is the first command added, also kicks off queue processing
 */
void PWM_I2C_Queue_Command( uint8_t address, uint16_t data, uint16_t mask, uint16_t waitMS ) {
	if ( commandCount >= PWM_I2C_MAX_COMMANDS ) {
		return;
	}

	commands[commandCount].address = address;
	commands[commandCount].data = data;
	commands[commandCount].mask = mask;
	commands[commandCount].waitMS = waitMS;
	commandCount++;

	_PWM_I2C_Sleep( 1 );
}

/*
 * Initialize the I2C interface
 * Based on Valvano p 374
 *
 */
void PWM_I2C_Init() {
	commandCount = 0;
	currentCommandIndex = 0;
	pwm_i2c_callback = 0;

	// Set up I2C
	SYSCTL_RCGCI2C_R |= 0x0001;			// Activate I2C0
	SYSCTL_RCGCGPIO_R |= 0x0002;		// Activate Port B

	while ( ( SYSCTL_PRGPIO_R & 0x0002 ) == 0 ) {};

	GPIO_PORTB_AFSEL_R |= 0x0C;			// Enable alt func on PB2, PB3
	GPIO_PORTB_ODR_R |= 0x08;			// Enable open drain on PB3

	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF00FF) | 0x0000300;
	GPIO_PORTB_DEN_R |= 0x0C;			// Enable digital I/O on PB2, PB3

	I2C0_MCR_R = 0x0010;				// TM4C123 is I2C master
	I2C0_MTPR_R = 0x8;					// 100 kHz clock (10000ns / ( 62.5ns - 1 ) )

	// Setup nCS on PB4
	GPIO_PORTB_DIR_R |= 0x10;			// Set PB4 for out
	GPIO_PORTB_DEN_R |= 0x10;			// Enable digital I/O on PB4
	PB4 = 0x0;							// Take PB4 low
}

/*
 * Accepts a callback that is called after
 * the transmit queue is empty (i.e. after all
 *waits are satisfied)
 */
void PWM_I2C_Set_Callback( void (*callback)() ) {
	pwm_i2c_callback = callback;
}

