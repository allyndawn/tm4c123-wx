// TM4C123-WX
// A Texas Instruments TM4C123 powered weather station with packet radio connectivity
//
// Allen Snook and Jake Dossett
// UW BEE 525 Final Project
// https://github.com/allendav/tm4c123-wx
//
// 23 February 2020

#include "stdint.h"
#include "stdio.h"

#include "tm4c123gh6pm.h"

#include "lcd.h"
#include "ds18b20.h"
#include "gps.h"
#include "rda1846.h"

char dateString[12];
char timeString[12];
char latString[12];
char latHem[12];
char longString[12];
char longHem[12];

uint8_t decimation = 0;

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

	char line1[17];
	line1[2] = '-';
	line1[5] = '-';
	line1[10] = ' ';
	line1[13] = ':';
	line1[16] = 0;

	char line2[17];
	line2[0] = 0;
	line2[16] = 0;

	PF2 ^= 0x04; // Toggle the LED

	// Every 10 seconds
	if ( 0 == decimation % 10 ) {
		if ( ! GPS_Is_Ready() ) {
			line1[0] = '#';
			line1[1] = '#';

			line1[3] = '#';
			line1[4] = '#';

			line1[6] = '#';
			line1[7] = '#';
			line1[8] = '#';
			line1[9] = '#';

			line1[11] = '#';
			line1[12] = '#';

			line1[14] = '#';
			line1[15] = '#';
		} else {
			GPS_Get_DateTime( dateString, timeString );
			GPS_Get_Location( latString, latHem, longString, longHem );
			//sprintf( "%s %s %s %s %s %s\n", dateString, timeString, latString, latHem, longString, longHem );
			line1[0] = dateString[2];
			line1[1] = dateString[3];

			line1[3] = dateString[0];
			line1[4] = dateString[1];

			line1[6] = '2';
			line1[7] = '0';
			line1[8] = dateString[4];
			line1[9] = dateString[5];

			line1[11] = timeString[0];
			line1[12] = timeString[1];

			line1[14] = timeString[2];
			line1[15] = timeString[3];
		}
		LCD_Write( line1, line2 );
	}

	// Every 60 seconds
	if ( 0 == decimation % 60 ) {
		DS18B20_InitiateMeasurement();
		// TODO - display last measurement
	}

	decimation++;
	if ( 59 < decimation ) {
		decimation = 0;
	}
}

int main( void ) {
	// Initialize the main application leds and polling timer
	Init();
	Timer1A_Init();

	// Initialize the LCD
	LCD_Init();
	LCD_Backlight_Full();

	// Initialize the thermometer
	DS18B20_Init();

	// Initialize the GPS
	GPS_Init();

	// Initialize the Radio
	RDA1846_Init();

	while ( 1 ) {
	}
}
