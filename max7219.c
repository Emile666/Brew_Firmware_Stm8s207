/*==================================================================
  File Name    : max7219.c
  Author       : Emile
  ------------------------------------------------------------------
  Purpose : This files contains the routines for the MAX7219
            8 x 7-segment display driver
  ------------------------------------------------------------------
  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this file. If not, see <http://www.gnu.org/licenses/>.
  ================================================================== */
#include "max7219.h"
#include "stm8_hw_init.h"
#include "spi.h"

/*------------------------------------------------------------------
  Purpose  : This function initialises the MAX7219 7-segment display
             driver to the following:
	     - No decode for digits 7-0
	     - Set intensity to 15/32
	     - Display digit 3-0
	     - Set normal operation
  Variables: -
  Returns  : -
  ------------------------------------------------------------------*/
void max7219_init(void)
{
    max7219_write(MAX7219_TEST);         // Normal operation
    max7219_write(MAX7219_DECD);         // No decode for digits 7-0
    max7219_write(MAX7219_INTNS | 0x05); // Set intensity to 7/32
    max7219_write(MAX7219_SCANL | 0x03); // Display digits 0, 1, 2 and 3
    max7219_write(MAX7219_SHUTD | 0x01); // Normal operation
    
    max7219_write(MAX7219_DIG0);         // reset digit 0
    max7219_write(MAX7219_DIG1);         // reset digit 1
    max7219_write(MAX7219_DIG2 | 0x80);  // reset digit 2, Alive LED on
    max7219_write(MAX7219_DIG3);         // reset digit 3
} // max7219_init()

/*------------------------------------------------------------------
  Purpose  : This function writes 1 16-bit data-byte to the MAX7219.
  Variables: dat: the data-byte to write to the MAX7219
  Returns  : -
  ------------------------------------------------------------------*/
void max7219_write(uint16_t dat)
{
    SPI_CS_LEDSb = 0;      // Enable CS pin of MAX7219
    SPI_CR1_SPE = 1;       // Enable SPI
    spi_write(dat >> 8);   // write bits D15-D08
    //spi_read();
    spi_write(dat & 0xFF); // write bits D07-D00
    //spi_read();
    SPI_CS_LEDSb = 1;      // Disable CS pin of MAX7219
    SPI_CR1_SPE  = 0;      // Disable SPI
} // max7219_write()
