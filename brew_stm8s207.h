#ifndef _BREW_STM8S207_H_
#define _BREW_STM8S207_H_
/*==================================================================
  File Name: brew_stm8s207.h
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This is the header-file for brew_stm8s207.c
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
#include <string.h>
#include "delay.h"         /* for delay_msec() */
#include "adc.h"
#include "i2c_bb.h"
#include "pwm.h"
#include "spi.h"
#include "uart.h"
#include "command_interpreter.h"
#include "misc.h"
#include "scheduler.h"
#include "eep.h"

#define ON1ST        (true) /* Create PWM signal by starting with 1 signal */
#define OFF1ST      (false) /* Create PWM signal by starting with 0 signal */

//------------------------------------
// Bit defines for elec_htrs variable
//------------------------------------
#define HTR_BK1     (0x01)
#define HTR_BK2     (0x02)
#define HTR_BK3     (0x04)
#define HTR_HLT1    (0x08)
#define HTR_HLT2    (0x10)
#define HTR_HLT3    (0x20)

typedef struct _pwmtime
{
    uint8_t std;      // STD state number
    uint8_t mask;     // bit-define for elec_htrs variable
    bool    on1st;    // true = make 1 first, false = make 0 first
} pwmtime;

//-------------------------------------------------------------------
// These defines are used by the B and H commands and indicate  
// which energy-sources are used.
// Note: These defines should be the same as in the PC-program!
//-------------------------------------------------------------------
#define GAS_MODU               (0x01) /* Modulating gas-valve */
#define GAS_ONOFF              (0x02) /* Non-modulating gas-valve */
#define ELEC_HTR1              (0x04) /* First electric heating-element */
#define ELEC_HTR2              (0x08) /* Second electric heating-element */
#define ELEC_HTR3              (0x10) /* Not implemented yet */

//-----------------------------
// Delayed Start STD modes
//-----------------------------
#define DEL_START_INIT   (0)
#define DEL_START_TMR    (1)
#define DEL_START_BURN   (2)
#define DEL_START_MAX_DELAY_TIME (54000) /* Max. time is 30 hours * 60 minutes * 30 * 2 seconds */
#define DEL_START_MAX_BURN_TIME   (3600) /* Max. time is 2 hours * 60 min. * 30 * 2 seconds */
#define DEL_START_ELEC_PWM         (100) /* PWM signal for HLT electric heaters during delayed start */

//-----------------------------
// pwm_2_time() States
//-----------------------------
#define EL_HTR_OFF (1)
#define EL_HTR_ON  (2)

//-----------------------------
// Defines for phases_present
//-----------------------------
#define PHASE_L1 (0x01) /* power-supply phase L1 */
#define PHASE_L2 (0x02) /* power-supply phase L2 */
#define PHASE_L3 (0x04) /* power-supply phase L3 */

//-------------------------------
// INIT_TEMP  20 °C
// LM35_CONV: 50000 E-2 °C / 1023
//-------------------------------
#define INIT_TEMP  (20)
#define LM35_CONV  (48.87585533)

//---------------------------------------------------
// Q = 5.5 Hz for 1 L, 11 pulses per L per sec.
// Per minute: 5.5 x 1 rising pulse x 60 sec. = 330
// An interrupt is generated for a rising AND
// falling edge, but only the rising edge is counted.
//---------------------------------------------------
#define FLOW_PER_L     (330)
#define FLOW_ROUND_OFF (FLOW_PER_L>>1)

void    print_ebrew_revision(char *ver);
uint8_t init_WIZ550IO_module(void);
void    print_IP_address(uint8_t *ip);

#endif /* _BREW_STM8S207_H_ */