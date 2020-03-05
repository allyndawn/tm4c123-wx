// RDA1846 Radio interface for the TM4C123
//
// Allen Snook
// 2 March 2020

#ifndef __RDA1846_H
#define __RDA1846_H

#include "stdint.h"

void RDA1846_Init();
void RDA1846_Set_Squelch( uint8_t on );
void RDA1846_Set_Frequency_KHz( uint32_t freqKHZ );
void RDA1846_Set_Volume( uint16_t volume1, uint16_t volume2 );


#endif // __RDA1846_H