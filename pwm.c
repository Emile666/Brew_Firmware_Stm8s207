/*==================================================================
  File Name: pwm.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : STM8 PWM routines
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
#include "pwm.h"

uint8_t hlt_gas_pwm; // duty-cycle for gas-burner HLT
uint8_t bk_gas_pwm;  // duty-cycle for gas-burner Boil-kettle

/*------------------------------------------------------------------
  Purpose  : This function writes a duty-cycle to a timer, so that a
             PWM signal is generated.
             There are two timers used, each with two channels:
             Timer 1, f = 16 kHz, used for the modulating gas-valves
             - channel 1: HLT_PWM
             - channel 2: BK_PWM
             The timer expect a duty-cycle in E-1 %.
  Variables:
       pwm_ch: [PWM_HLT, PWM_BK], the PWM timer to write to
   duty_cycle: The duty_cycle for the timer, value between [0%,100%]
  Returns  : -
  ------------------------------------------------------------------*/
void pwm_write(uint8_t pwm_ch, uint8_t duty_cycle) // duty_cyle varies 0 % and 100 %
{
    uint8_t lsb,msb;
    uint16_t temp;

    // The LM393 with the BCP53 PNP transistor behave as normal logic.
    // PWM signal = 1 -> LM393 output low -> BCP53 on -> +24V output
    temp = 10 * (uint16_t)duty_cycle; // timers expect a duty-cycle in E-1 %
    msb  = (temp >> 8);
    lsb  = temp & 0xFF;
    if (pwm_ch == PWM_HLT)
    {   // HLT
        hlt_gas_pwm = duty_cycle; // save for webserver
        TIM1_CCR1H  = msb;        // msb, write this first
        TIM1_CCR1L  = lsb;        // lsb
    } // if
    else
    {   // Boil-kettle
        bk_gas_pwm = duty_cycle; // save for webserver
        TIM1_CCR2H = msb;        //  msb, write this first
        TIM1_CCR2L = lsb;        //  lsb
    } // else
} // pwm_write()

