// Interface for the Sparkfun LCD-14073 LCD via SSI / SPI
// 2020 February 15

#ifndef __LCD_H
#define __LCD_H

void LCD_Init();
void LCD_Backlight_Full();
void LCD_Write( char *line1, char *line2 );


#endif // __LCD_H