// Interrupt driven UART interface with FIFO for TM4C123
// Based on work by Jonathan Valvano
//
// Uses UART1: PC4 (U1Rx) and PC5 (U1Tx)
//
// Allen Snook
// 23 February 2020

#ifndef __UART_H
#define __UART_H

#include "stdint.h"

void UART_Init();
void UART1_Handler();
void UART_Register_Receive_Callback( void (*callback)(char *data) );

#endif // __UART_H