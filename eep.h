#ifndef _STM8_EEP_H
#define _STM8_EEP_H
/*==================================================================
  File Name: eep.h
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This is the header-file for eep.c
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
#include "stm8_hw_init.h"

// EEPROM base address within STM8 uC
#define EEP_BASE_ADDR (0x4000)

#define EEPARB_INIT           (0x08) /* 8-bit */
#define EEPARB_ETHUSB         (0x0A) /* 8-bit, 0x00 = ETH, 0xff = USB */
#define EEPARB_IP0            (0x0C) /* 16-bit */
#define EEPARB_IP1            (0x0E) /* 16-bit */
#define EEPARB_IP2            (0x10) /* 16-bit */
#define EEPARB_IP3            (0x12) /* 16-bit */
#define EEPARB_HW_ID          (0x14) /* 16-bit */
#define EEPARB_DEL_START_ENA  (0x16) /* 8-bit */
#define EEPARB_DEL_START_TMR1 (0x18) /* 16-bit */
#define EEPARB_DEL_START_TIME (0x1A) /* 16-bit */

#define NO_INIT               (0xFF)
#define USE_ETH               (0x00)
#define USE_USB               (0xFF)

// Function prototypes
uint8_t  eep_read8(uint8_t eep_address);
uint16_t eep_read16(uint8_t eep_address);
void     eep_write8(uint8_t eep_address, uint8_t data);
void     eep_write16(uint8_t eep_address, uint16_t data);
void     check_and_init_eeprom(void);
void     read_eeprom_parameters(void);
void     write_eeprom_parameters(void);

#endif
