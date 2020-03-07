// Neo6 GPS Interface for TM4C123
//
// Uses PC4 (U1Rx) and PC5 (U1Tx)
//
// Allen Snook
// 23 February 2020

#ifndef __GPS_H
#define __GPS_H

#include "stdint.h"

void GPS_Init();

uint8_t GPS_Device_Detected();
uint8_t GPS_Data_Valid();

void GPS_Get_Date( uint16_t *year, uint8_t *month, uint8_t *day );
void GPS_Get_Time( uint8_t *hour, uint8_t *minute, uint8_t *seconds );
void GPS_Get_Latitude( uint8_t *degrees, uint8_t *minutes, uint8_t *seconds, char *hemisphere );
void GPS_Get_Longitude( uint8_t *degrees, uint8_t *minutes, uint8_t *seconds, char *hemisphere );


#endif // __GPS_H