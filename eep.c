/*==================================================================
  File Name: eep.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This files contains the EEPROM related functions 
             for the STM8 uC.
  ------------------------------------------------------------------
  This file is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
  ================================================================== */ 
#include "eep.h"

extern bool ethernet_WIZ550i;
extern bool delayed_start_enable; // true = delayed start is enabled

/*-----------------------------------------------------------------------------
  Purpose  : This function reads a (8-bit) value from the STM8 EEPROM.
  Variables: eep_address: the index number within the EEPROM. An index number
                          is the n-th 16-bit variable within the EEPROM.
  Returns  : the (8-bit value)
  ---------------------------------------------------------------------------*/
uint8_t eep_read8(uint8_t eep_address)
{
    uint8_t data;
    
    char    *address = (char *)EEP_BASE_ADDR; // EEPROM base address.
    address += eep_address;                   // add offset
    data     = *address;                      // read byte
    return data;
} // eep_read8()

/*-----------------------------------------------------------------------------
  Purpose  : This function reads a (16-bit) value from the STM8 EEPROM.
  Variables: eep_address: the index number within the EEPROM. An index number
                          is the n-th 16-bit variable within the EEPROM.
  Returns  : the (16-bit value)
  ---------------------------------------------------------------------------*/
uint16_t eep_read16(uint8_t eep_address)
{
    uint16_t data;
    
    char    *address = (char *)EEP_BASE_ADDR; // EEPROM base address.
    address += eep_address;                   // add offset
    data     = *address++;                    // read MSB first
    data   <<= 8;                             // SHL 8
    data    |= *address;                      // read LSB
    return data;                              // Return result
} // eep_read16()

/*-----------------------------------------------------------------------------
  Purpose  : This function writes a (8-bit) value to the STM8 EEPROM.
  Variables: eep_address: the index number within the EEPROM. An index number
                          is the n-th 16-bit variable within the EEPROM.
             data       : 8-bit value to write to the EEPROM
  Returns  : -
  ---------------------------------------------------------------------------*/
void eep_write8(uint8_t eep_address,uint8_t data)
{
    char *address = (char *)EEP_BASE_ADDR; //  EEPROM base address.

    // Avoid unnecessary EEPROM writes
    if (data == eep_read8(eep_address)) return;

    address += eep_address; // add offset
    FLASH_DUKR = 0xAE;      // unlock EEPROM
    FLASH_DUKR = 0x56;
    while (!FLASH_IAPSR_DUL) ; // wait until EEPROM is unlocked
    *address   = (uint8_t)(data & 0xFF); // write it
    FLASH_IAPSR_DUL = 0;    // write-protect EEPROM again
} // eep_write8()

/*-----------------------------------------------------------------------------
  Purpose  : This function writes a (16-bit) value to the STM8 EEPROM.
  Variables: eep_address: the index number within the EEPROM. An index number
                          is the n-th 16-bit variable within the EEPROM.
             data       : 16-bit value to write to the EEPROM
  Returns  : -
  ---------------------------------------------------------------------------*/
void eep_write16(uint8_t eep_address,uint16_t data)
{
    char *address = (char *)EEP_BASE_ADDR; //  EEPROM base address.

    // Avoid unnecessary EEPROM writes
    if (data == eep_read16(eep_address)) return;

    address += eep_address; // add offset
    FLASH_DUKR = 0xAE;      // unlock EEPROM
    FLASH_DUKR = 0x56;
    while (!FLASH_IAPSR_DUL) ; // wait until EEPROM is unlocked
    *address++ = (uint8_t)((data >> 8) & 0xFF); // write MSB
    *address   = (uint8_t)(data & 0xFF);        // write LSB
    FLASH_IAPSR_DUL = 0;    // write-protect EEPROM again
} // eep_write16()

void check_and_init_eeprom(void)
{
    uint8_t x;
    
    x = eep_read8(EEPARB_INIT);
    if (x == NO_INIT)
    {
        eep_write8(EEPARB_INIT  , 0x01);         // Eeprom init. flag
        eep_write8(EEPARB_ETHUSB, USE_USB);      // Default: Use USB
        eep_write8(EEPARB_HW_ID, 0x01);          // PCB HW ID number
        eep_write8(EEPARB_DEL_START_ENA ,false); // delayed-start enable
        eep_write16(EEPARB_DEL_START_TMR1,0);    // delayed-start timer 1
        eep_write16(EEPARB_DEL_START_TIME,0);    // delayed-start time
    } // if
} // check_and_init_eeprom()

void read_eeprom_parameters(void)
{
    if (eep_read8(EEPARB_ETHUSB) == USE_ETH)
         ethernet_WIZ550i = true;
    else ethernet_WIZ550i = false;
    
    if ((delayed_start_enable = eep_read8(EEPARB_DEL_START_ENA)) != true)
    {
        eep_write8(EEPARB_DEL_START_ENA ,false); // could also be 0xff the 1st time
        eep_write16(EEPARB_DEL_START_TMR1,0);    // reset timer1 in eeprom
        eep_write16(EEPARB_DEL_START_TIME,0);    // delayed-start time
    } // if
} // read_eeprom_parameters()

void write_eeprom_parameters(void)
{
    if (ethernet_WIZ550i)
         eep_write8(EEPARB_ETHUSB, USE_ETH);
    else eep_write8(EEPARB_ETHUSB, USE_USB);
} // write_eeprom_parameters()
