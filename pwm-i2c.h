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

#ifndef __PWM_I2C_H
#define __PWM_I2C_H

#include "stdint.h"

void PWM_I2C_Init();
void PWM_I2C_Set_Callback( void (*callback)() );
void PWM_I2C_Queue_Command( uint8_t address, uint16_t data, uint16_t mask, uint16_t waitMS );
uint16_t PWM_I2C_Read( uint8_t address );

#endif // __PWM_I2C_H