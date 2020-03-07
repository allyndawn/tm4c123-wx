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
#include "string.h"

#include "tm4c123gh6pm.h"

#include "lcd.h"
#include "ds18b20.h"
#include "gps.h"
#include "rda1846.h"

uint8_t cycleCount = 0; // 0 to 119

uint8_t gpsDataValid = 0;
uint8_t gpsDeviceDetected = 0;

uint8_t thermometerDataValid = 0;

uint16_t year;
uint8_t month, day, hour, minute, seconds, latDeg, longDeg;
char latHem, longHem;

int16_t temperatureDegreesF = 0;

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
	// Set PF3 for out
	GPIO_PORTF_DIR_R |= 0x08;

	// Enable digital I/O on PF2
	GPIO_PORTF_DEN_R |= 0x04;
	// Enable digital I/O on PF3
	GPIO_PORTF_DEN_R |= 0x08;

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

/*
 * Fires twice each second
 */
void Timer1A_Handler() {
	// Acknowledge interrupt
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;

	// Toggle the LED on every cycle
	PF2 ^= 0x04;

	// TODO read thermometer

	// Every 10th cycle (5 seconds) update GPS and therm state
	if ( 0 == cycleCount % 10 ) {
		gpsDeviceDetected = GPS_Device_Detected();
		gpsDataValid = GPS_Data_Valid();
		if ( gpsDataValid ) {
			GPS_Get_Date( &year, &month, &day );
			GPS_Get_Time( &hour, &minute, &seconds );
			GPS_Get_Latitude( &latDeg, 0, 0, &latHem );
			GPS_Get_Longitude( &longDeg, 0, 0, &longHem );
		}

		thermometerDataValid = DS18B20_Data_Valid();
		if ( thermometerDataValid ) {
			temperatureDegreesF = DS18B20_Get_Temperature_F();
		}
	}

	// Update the display every 10 seconds
	if ( 0 == cycleCount % 20 ) {
		char line1[17];
		char line2[17];

		if ( ! gpsDeviceDetected ) {
			strcpy( line1, "Looking for GPS" );
			strcpy( line2, "");
		} else if ( ! gpsDataValid ) {
			strcpy( line1, "Acquiring Sats" );
			strcpy( line2, "");
		} else {
			GPS_Get_Date( &year, &month, &day );
			GPS_Get_Time( &hour, &minute, &seconds );
			GPS_Get_Latitude( &latDeg, 0, 0, &latHem );
			GPS_Get_Longitude( &longDeg, 0, 0, &longHem );
			sprintf( line1, "%02hu/%02hu/%04u %02u:%02u", month, day, year, hour, minute );
			if ( DS18B20_Data_Valid() ) {
				sprintf( line2, "%02hu %c %03hu %c %3d F", latDeg, latHem, longDeg, longHem, temperatureDegreesF );
			} else {
				sprintf( line2, "%02hu %c %03hu %c --- F", latDeg, latHem, longDeg, longHem );
			}
		}

		LCD_Write( line1, line2 );
	}

	// On six cycles (3 seconds) in, start a new temperature measurement
	if ( cycleCount % 6 ) {
		DS18B20_Initiate_Measurement();
	}

	// On 12 cycles in (6 seconds), fetch the most recent measurement
	if ( 12 == cycleCount ) {
		DS18B20_Read_Scratchpad();
	}

	cycleCount++;
	// Every 120 cycles (60 seconds) start over again
	if ( 29 < cycleCount ) {
		cycleCount = 0;
	}
}

int main( void ) {
	// Initialize the LCD
	LCD_Init();
	LCD_Backlight_Full();

	// Initialize the thermometer
	DS18B20_Init();

	// Initialize the GPS
	GPS_Init();

	// Initialize the Radio
	RDA1846_Init();

	// Initialize the main application leds and polling timer
	Init();
	Timer1A_Init();

	while ( 1 ) {
	}
}
