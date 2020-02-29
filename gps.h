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
uint8_t GPS_Is_Ready();
void GPS_Get_DateTime( char *dateString, char *timeString );
void GPS_Get_Location( char *latString, char *latHem, char *longString, char *longHem );

#endif // __GPS_H