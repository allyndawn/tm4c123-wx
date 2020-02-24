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

volatile int16_t tempTenths;

void Init( void ) {
	tempTenths = DS18B20_NO_READING;
}

void Timer0A_Init( void ) {
	// TODO set up a 1 us timer to service the thermometer queue
}

void Timer0A_Handler( void ) {
	// TODO set up a 15 second timer to update the temperature measurement
	// TODO Acknowledge the interrupt
	DS18B20_TickHandler();
}

void Timer0B_Init( void ) {
}

void Timer0B_Handler( void ) {
	// TODO Acknowledge the interrupt
	tempTenths = DS18B20_GetTempTenths();
}

int main( void ) {
	Init();

	DS18B20_Init();

	Timer0A_Init();
	Timer0B_Init();

	DS18B20_InitiateMeasurement();

	while ( 1 ) {
	}
}
