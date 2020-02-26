// Maxim/Dallas One Wire Interface for TM4C123
// Allen Snook
// 23 February 2020

#ifndef __ONEWIRE_H
#define __ONEWIRE_H

#include "stdint.h"

void OneWire_Init();
void OneWire_Timer0A_Handler();
void OneWire_Reset( void (*callback)(uint8_t data) );
void OneWire_WriteByte( uint8_t data );
void OneWire_ReadByte( void (*callback)(uint8_t data) );
void OneWire_WaitForHigh( void (*callback)(uint8_t data) );

#endif // __ONEWIRE_H