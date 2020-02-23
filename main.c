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

#define PF2 (*((volatile uint32_t *)0x40025010))

int main(void)
{
	// Activate clock for Port F
	SYSCTL_RCGCGPIO_R |= 0x00000020;

	// Allow time for clock to stabilize
	while ((SYSCTL_PRGPIO_R&0x20) == 0)
	{
	}

	// Set PF2 for out
	GPIO_PORTF_DIR_R |= 0x04;

	// Enable digital I/O on PF2
	GPIO_PORTF_DEN_R |= 0x04;

	while ( 1 )
	{
		// Toggle PF2 - the blue LED
		PF2 ^= 0x04;
	}
}
