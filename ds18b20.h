// An interface to a Maxim DS18B20 digital thermometer
// Allen Snook
// 23 February 2020

#ifndef __DS18B20_H
#define __DS18B20_H

#include "stdint.h"

#define DS18B20_NO_READING -9999

void DS18B20_Init( void );

void DS18B20_Initiate_Measurement();
void DS18B20_Read_Scratchpad();
int16_t DS18B20_Get_Temperature_F();

#endif // __DS18B20_H
