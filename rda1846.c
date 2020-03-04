// RDA1846 Radio interface for the TM4C123
// with packet radio transmission capability
//
// Based on work by InductiveTwig / HamShield
//
// Allen Snook
// 2 March 2020

#include "rda1846.h"
#include "pwm-i2c.h"

#define RDA1846_CLK_MODE_R 0x04
#define RDA1846_GPIO_MODE_R 0x1F
#define RDA1846_CTL_R 0x30
#define RDA1846_TX_VOICE_R 0x3A

// Private methods

void _RDA1846_Set_Narrow_Band() {
	// Set up for 12.5 kHz channel width
	PWM_I2C_Queue_Command( 0x11, 0x3D37, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x12, 0x0100, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x15, 0x1100, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x32, 0x4495, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x34, 0x2B8E, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x3A, 0x40C3, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x3C, 0x0F1E, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x3F, 0x28D0, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x48, 0x20BE, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x60, 0x1BB7, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x62, 0x0A10, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x65, 0x2494, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x66, 0xEB2E, 0xFFFF, 0 );

	// AGC table
	PWM_I2C_Queue_Command( 0x7F, 0x0001, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x05, 0x000C, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x06, 0x020C, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x07, 0x030C, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x08, 0x0324, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x09, 0x1344, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x0A, 0x3F44, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x0B, 0x3F44, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x0C, 0x3F44, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x0D, 0x3F44, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x0E, 0x3F44, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x0F, 0x3F44, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x12, 0xE0ED, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x13, 0xF2FE, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x14, 0x0A16, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x15, 0x2424, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x16, 0x2424, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x17, 0x2424, 0xFFFF, 0 );

	// Last command ends the AGC table and needs a 100 ms delay
	PWM_I2C_Queue_Command( 0x7F, 0x0, 0xFFFF, 100 );
}

void _RDA1846_Soft_Reset() {
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0x1, 0xFFFF, 100 );
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0x4, 0xFFFF, 0 );
}

void _RDA1846_Set_Clock_Mode( uint8_t mode ) {
	if ( 0 == mode ) {
		PWM_I2C_Queue_Command( RDA1846_CLK_MODE_R, 0x0FD0, 0xFFFF, 0 );
	} else {
		PWM_I2C_Queue_Command( RDA1846_CLK_MODE_R, 0x0FD1, 0xFFFF, 0 );
	}
}

void _RDA1846_Set_TX() {
	// TODO: Add 70cm support

	// Disable RX (Clear b:5)
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0, 0xFFDF, 0 );

	// Set GPIO 5 (Set b:11)
	PWM_I2C_Queue_Command( RDA1846_GPIO_MODE_R, 0x0800, 0xF7FF, 0 );

	// Set GPIO 4 (b:9) Low, sleep 50 ms
	PWM_I2C_Queue_Command( RDA1846_GPIO_MODE_R, 0, 0xFDFF, 50 );

	// Enable TX (Set b:6)
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0x0040, 0xFFBF, 0 );
}

void _RDA1846_Set_RX() {
	// Disable TX (Clear b:6)
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0, 0x0040, 0 );

	// Set GPIO 5 (b:11) Low
	PWM_I2C_Queue_Command( RDA1846_GPIO_MODE_R, 0, 0xF7FF, 0 );

	// Set GPIO 4 (b:9) Low, sleep 50 ms
	PWM_I2C_Queue_Command( RDA1846_GPIO_MODE_R, 0, 0xFDFF, 50);

	// Enable RX (Set b:5)
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0x0020, 0xFFDF, 0 );
}

void _RDA1846_Set_Transmit_Source_PWM_Mic() {
	PWM_I2C_Queue_Command( RDA1846_TX_VOICE_R, 0x4, 0xFFFF, 0 );
}

void _RDA1846_Set_Squelch( uint8_t on ) {
	// Turn squelch on (Set b: 3)
	PWM_I2C_Queue_Command( RDA1846_CTL_R, 0x0008, 0xFFF7, 0 );
}

void _RDA1846_Set_Squelch_Low_Threshhold( int16_t threshhold ) {
	// TODO
}

void _RDA1846_Get_RSSI() {
	// TODO
}

// Public methods

void RDA1846_Wait_For_Channel( /* callback */ ) {
}

void RDA1846_Set_Frequency_KHz( uint32_t freqKHZ ) {
}

void RDA1846_Set_Volume( uint16_t volume1, uint16_t volume2 ) {
}

void RDA1846_Set_Power( uint8_t power ) {
}


void RDA1846_Test_Connection( /* callback */ ) {
}

void RDA1846_Send_Packet( char *data ) {
}

void _RDA1846_Init_Complete_Callback() {
	PWM_I2C_Set_Callback( 0 );

	// Finish setting up the radio
	RDA1846_Set_Frequency_KHz( 144930 );
	_RDA1846_Set_RX();
	// TODO wait
	// TODO set up squelch
}

void RDA1846_Init() {
	PWM_I2C_Init();
	PWM_I2C_Set_Callback( _RDA1846_Init_Complete_Callback );

	_RDA1846_Soft_Reset();

	PWM_I2C_Queue_Command( 0x09, 0x03AC, 0xFFFF, 0 );		// Set GPIO voltage for 3.3V
	PWM_I2C_Queue_Command( 0x0A, 0x47E0, 0xFFFF, 0 );		// Set PGA Gain
	PWM_I2C_Queue_Command( 0x13, 0xA100, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x1F, 0x5001, 0xFFFF, 0 );		// GPIO7->VOX, GPIO0->CTC/DCS

	PWM_I2C_Queue_Command( 0x31, 0x0031, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x33, 0x0AF2, 0xFFFF, 0 );		// AGC

	PWM_I2C_Queue_Command( 0x41, 0x067F, 0xFFFF, 0 );		// Voice gain
	PWM_I2C_Queue_Command( 0x44, 0x02FF, 0xFFFF, 0 );		// TX gain
	PWM_I2C_Queue_Command( 0x47, 0x7F2F, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x4F, 0x2C62, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x53, 0x0094, 0xFFFF, 0 );		// Compressor update time
	PWM_I2C_Queue_Command( 0x54, 0x2A18, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x55, 0x0081, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x56, 0x0B22, 0xFFFF, 0 );		// Squelch detection time
	PWM_I2C_Queue_Command( 0x57, 0x1C00, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x58, 0x800D, 0xFFFF, 0 );
	PWM_I2C_Queue_Command( 0x5A, 0x0EDB, 0xFFFF, 0 );		// Noise detection time
	PWM_I2C_Queue_Command( 0x63, 0x3FFF, 0xFFFF, 0 );		// Pre-emphasis bypass

	// Calibration - wait 100 ms after each command
	PWM_I2C_Queue_Command( 0x30, 0x00A4, 0xFFFF, 100 );
	PWM_I2C_Queue_Command( 0x30, 0x00A6, 0xFFFF, 100 );
	PWM_I2C_Queue_Command( 0x30, 0x0006, 0xFFFF, 100 );

	_RDA1846_Set_Narrow_Band();
}
