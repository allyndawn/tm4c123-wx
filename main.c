// TM4C123-WX
// A Texas Instruments TM4C123 powered weather station with packet radio connectivity
//
// Allen Snook and Jake Dossett
// UW BEE 525 Final Project
// https://github.com/allendav/tm4c123-wx
//
// 23 February 2020

#include "stdint.h"
#include "tm4c123gh6pm.h"
#include "ds18b20.h"

#define PF2 (*((volatile uint32_t *)0x40025010))

void Init() {
	// Activate clock for Port F
	SYSCTL_RCGCGPIO_R |= 0x00000020;

	// Allow time for clock to stabilize
	while ((SYSCTL_PRGPIO_R & 0x20) == 0)
	{
	}

	// Set PF2 for out
	GPIO_PORTF_DIR_R |= 0x04;

	// Enable digital I/O on PF2
	GPIO_PORTF_DEN_R |= 0x04;

	// Turn the LED on
	PF2 = 0x04;
}

// Temporary until we hook this up properly
// Request an updated temperature once per second
// When we do this for reals, we won't need to
// sample nearly this quickly.
void Timer1A_Init() {
	volatile unsigned long delay;

	// Activate timer 1
	SYSCTL_RCGCTIMER_R |= 0x02;

	// Wait for the timer to settle
	// TODO Is there a better way to do this?
	delay = SYSCTL_RCGCTIMER_R;

	// Disable timer during setup
	TIMER1_CTL_R &= ~TIMER_CTL_TAEN;

	// Configure for 32-bit timer mode
	TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;

	// Configure for a periodic timer
	TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;

	// No prescaling
	TIMER1_TAPR_R = 0;

	// Assuming a 16 MHz clock, set the initial counting value
	// (8000000 -> 0x7A1200 -> 2 Hz
	TIMER1_TAILR_R = 0x7A1200;

	// Enable timeout (rollover) interrupt
	TIMER1_IMR_R |= TIMER_IMR_TATOIM;

	// Clear any lingering timer timeout flag
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;

	// Set timer priority to 3
	NVIC_PRI5_R = (NVIC_PRI5_R & 0xFFFF00FF) | 0x00006000;

	// Timer 1A uses interrupt 21 - enable it in NVIC
	NVIC_EN0_R = 1 << 21;

	// Enable the timer
	TIMER1_CTL_R |= TIMER_CTL_TAEN;
}

void Timer1A_Handler() {
	// Acknowledge interrupt
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;

	DS18B20_InitiateMeasurement();

	PF2 ^= 0x04; // Toggle the LED
}

int main( void ) {
	Init();
	Timer1A_Init();

	DS18B20_Init();

	while ( 1 ) {
	}
}
