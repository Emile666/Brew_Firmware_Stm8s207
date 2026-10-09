/*==================================================================
  File Name: stm8_hw_init.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This files initializes all the hardware peripherals 
             of the STM8S207R8 uC.
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
  along with this software. If not, see <http://www.gnu.org/licenses/>.
  ================================================================== */ 
#include "stm8_hw_init.h"
#include "scheduler.h"

extern uint32_t t2_millis;    // needed for delay_msec()
extern uint32_t flow_hlt_mlt;
extern uint32_t flow_mlt_boil;
extern uint32_t flow_cfc_out; // Count from flow-sensor at output of CFC
extern uint32_t flow4;        // Count from HLT-CFC to MLT-top return

//------------------------------------------------
// Buzzer variables
//------------------------------------------------
bool     bz_on = false;    // true = buzzer is enabled
bool     bz_dbl;           // true = generate 2nd beep
uint8_t  bz_std = BZ_OFF;  // std number
uint8_t  bz_rpt;           // buzzer repeat counter
uint8_t  bz_rpt_max;       // number of beeps to make
uint16_t bz_tmr;           // buzzer msec counter
uint8_t  bz_freq = FREQ_4KHZ;

/*------------------------------------------------------------------
  Purpose  : This is the buzzer routine which runs every msec. 
             (f=1 kHz). It is used by 1 kHz interrupt routine.
  Variables: freq: [FREQ_1KHz, FREQ_2KHz, FREQ_4KHz]
  Returns  : -
  ------------------------------------------------------------------*/
void buzzer(void)
{
    switch (bz_std)
    {
        case BZ_OFF:   BEEP_CSR_BEEPEN  = 0;       //  Turn off the beep.
                       BEEP_CSR_BEEPSEL = bz_freq; //  0=1 kHz, 1=2 kHz, 2=4 kHz
                       BEEP_CSR_BEEPDIV = 14;      //  Set beep divider to 128/16=8 kHz
                       if (bz_on) 
                       {
                               bz_tmr = 0;
                               bz_rpt = 0;
                               bz_dbl = false;
                               bz_std = BZ_ON;
                       } // if
                       break;
        case BZ_ON:    BEEP_CSR_BEEPEN = 1;    //  Turn-on the beep.
                       if (++bz_tmr > 50) 
                       {
                               bz_tmr = 0;
                               if (!bz_dbl)
                                    bz_std = BZ_SHORT;
                               else bz_std = BZ_BURST;
                       } // if
                       break;
        case BZ_SHORT: BEEP_CSR_BEEPEN  = 0;   //  Turn off the beep.
                       bz_dbl = true;
                       if (++bz_tmr >= 50)
                       {
                               bz_tmr = 0;
                               bz_std = BZ_ON;
                       } // if
                       break;		
        case BZ_BURST: BEEP_CSR_BEEPEN  = 0;   //  Turn off the beep.
                       if (++bz_tmr > 500)
                       {
                             bz_tmr = 0;  
                             bz_dbl = false;
                             if (++bz_rpt >= bz_rpt_max) 
                             {
                                      bz_on  = false;
                                      bz_std = BZ_OFF;
                             } // if
                             else bz_std = BZ_ON;
                       } // if
                       break;					   						 
    } // switch
} // buzzer()

/*------------------------------------------------------------------
  Purpose  : This is the Timer-interrupt routine for the Timer 2 
             Overflow handler which runs every msec. (f=1 kHz). 
             It is used by the task-scheduler.
  Variables: -
  Returns  : -
  ------------------------------------------------------------------*/
#pragma vector = TIM2_OVR_UIF_vector
__interrupt void TIM2_UPD_OVF_IRQHandler(void)
{
    static uint16_t gc = 0;
    
    t2_millis++;       // update millisecond counter
    buzzer();          // sound alarm through buzzer
    scheduler_isr();   // call the ISR routine for the task-scheduler
    if (++gc > 499)
    {
        IRQ_LEDb ^= 1; // toggle IRQ led
        gc = 0;
    } // if
    TIM2_SR1_UIF = 0; // Reset the interrupt otherwise it will fire again straight away.
} // TIM2_UPD_OVF_IRQHandler()

/*-----------------------------------------------------------------------------
  Purpose  : This is the PORTA interrupt routine.
             It is used to count the falling-edges generated by flow sensor
	     FLOW1. It updates the global variable flow_hlt_mlt
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
#pragma vector = EXTI0_vector
__interrupt void PORTA_IRQHandler(void)
{
    flow_hlt_mlt++; 
} // PORTA_IRQHandler()

/*-----------------------------------------------------------------------------
  Purpose  : This is the PORTE interrupt routine.
             It is used to count the falling-edges generated by flow sensors
	     FLOW2, FLOW3 and FLOW4. 
             It uses the global variables flow_mlt_boil, flow_cfc_out and flow4.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
#pragma vector = EXTI4_vector
__interrupt void PORTE_IRQHandler(void)
{
    uint8_t x = ~(PE_IDR & (FLOW2 | FLOW3 | FLOW4));
    
    if (x & FLOW2) flow_mlt_boil++;
    if (x & FLOW3) flow_cfc_out++;
    if (x & FLOW4) flow4++;
} // PORTE_IRQHandler()

/*-----------------------------------------------------------------------------
  Purpose  : This routine initialises the system clock to run at 24 MHz.
             It uses the external HSE oscillator. 
  Variables: clk: which oscillator to use: HSI (0xE1), HSE (0xB4) or LSI (0xD2)
  Returns  : which oscillator is active: HSI (0xE1), HSE (0xB4) or LSI (0xD2)
  ---------------------------------------------------------------------------*/
uint8_t initialise_system_clock(uint8_t clk)
{
    CLK_ECKR       = 0;           //  Reset the External Clock Register.
    CLK_ECKR_HSEEN = 1;           //  Enable the HSE.
    while (CLK_ECKR_HSERDY == 0); //  Wait for the HSE to be ready for use.
    CLK_CKDIVR     = 0;           //  Ensure the clocks are running at full speed.
 
    // The datasheet lists that the max. ADC clock is equal to 6 MHz (4 MHz when on 3.3V).
    // Because fMASTER is now at 24 MHz, we need to set the ADC-prescaler to 6.
    ADC_CR1_SPSEL  = 0x03;        //  Set prescaler to 6, fADC = 4 MHz
    CLK_SWIMCCR    = 0;           //  Set SWIM to run at clock / 2.
    while (CLK_SWCR_SWBSY != 0);  //  Pause while the clock switch is busy.

    //  Enable switching for 24 MHz
    if (clk == HSE) CLK_SWCR_SWEN  = 1;
    CLK_SWR        = 0xB4;        //  Use HSE as the clock source.
    while (CLK_SWCR_SWIF == 0);   //  Target clock source not ready  
    CLK_SWCR       = 0;           //  Reset the clock switch control register.
    CLK_SWCR_SWEN  = 1;           //  Enable switching.
    CLK_ICKR_LSIEN = 1;           //  Enable LSI clock for BEEP function
    while (CLK_SWCR_SWBSY != 0);  //  Pause while the clock switch is busy.
    return CLK_CMSR;              //  Return which oscillator is active
} // initialise_system_clock()

/*-----------------------------------------------------------------------------
  Purpose  : This routine initialises Timer 2 to generate a 1 kHz interrupt.
             16 MHz: 16 MHz / (  16 *  1000) = 1000 Hz (1000 = 0x03E8)
             24 MHz: 24 MHz / (  16 *  1500) = 1000 Hz (1500 = 0x05DC)
  Variables: clk: HSE (24 MHz), HSI (16 MHz)
  Returns  : -
  ---------------------------------------------------------------------------*/
void setup_timers(uint8_t clk)
{
    // Set Timer 2 for an interrupt frequency of 1 kHz
    // 16 MHz: TIM2_ARRH=0x03, TIM2_ARRL=0xE8 (0x03E8 = 1000)
    // 24 MHz: TIM2_ARRH=0x05, TIM2_ARRL=0xDC (0x05DC = 1500)
    TIM2_PSCR    = 0x04;  //  Prescaler = 16
    if (clk == HSE)
    {   // external HSE oscillator
        TIM2_ARRH    = 0x05;  //  High byte of 1500 for 24 MHz
        TIM2_ARRL    = 0xDC;  //  Low  byte of 1500 for 24 MHz
    } // if
    else
    {   // internal HSI oscillator
        TIM2_ARRH    = 0x03;  //  High byte of 1000 for 16 MHz
        TIM2_ARRL    = 0xE8;  //  Low  byte of 1000 for 16 MHz
    } // else
    TIM2_IER_UIE = 1;     //  Enable the update interrupts
    TIM2_CR1_CEN = 1;     //  Finally enable the timer
    
    // Set Timer 1 for a frequency of 24 kHz, used for generating PWM signals
    TIM1_PSCRH = 0x00;    //  Prescaler = 1
    TIM1_PSCRL = 0x00;    //  Prescaler = 1
    TIM1_ARRH  = 0x03;    //  High byte of 1000 (24 MHz / 1000 = 24 kHz)
    TIM1_ARRL  = 0xE8;    //  Low  byte of 1000
    TIM1_CCR1H = 0x00;    //  Test: duty-cycle of 20%
    TIM1_CCR1L = 0xC8;    //        1000 * 0.20 = 200 = 0xC8
    TIM1_CCR2H = 0x00;    //  Test: duty-cycle of 25%
    TIM1_CCR2L = 0xFA;    //        1000 * 0.25 = 250 = 0xFA
    TIM1_CCER1_CC1P = 0;  //  Active high channel 1
    TIM1_CCER1_CC2P = 0;  //  Active high channel 2
    TIM1_CCER1_CC1E = 1;  //  Capture compare channel 1 output enable
    TIM1_CCER1_CC2E = 1;  //  Capture compare channel 2 output enable
    TIM1_CCMR1_OC1M = 6;  //  PWM Mode 1 - active if counter < CCR1, inactive otherwise.
    TIM1_CCMR2_OC2M = 6;  //  PWM Mode 1 - active if counter < CCR2, inactive otherwise.
    TIM1_BKR_MOE    = 1;  //  Main Output enable
    TIM1_CR1_CEN    = 1;  //  Finally enable the timer
    
    // Set Timer 3 for a frequency of approx. 1 Hz, used for generating SSR signals
    TIM3_PSCR  = 0x0F;    //  Prescaler = 16384
    TIM3_ARRH  = 0x03;    //  High byte of 1000 (24 MHz / (2^15 * 1000) = 0.73 Hz)
    TIM3_ARRL  = 0xE8;    //  Low  byte of 1000
    TIM3_CCR1H = 0x01;    //  Test: duty-cycle of 30%
    TIM3_CCR1L = 0x2C;    //        1000 * 0.30 = 300 = 0x12C
    TIM3_CCR2H = 0x01;    //  Test: duty-cycle of 35%
    TIM3_CCR2L = 0x5E;    //        1000 * 0.35 = 350 = 0x15E
    TIM3_CCER1_CC1P = 0;  //  Active high channel 1
    TIM3_CCER1_CC2P = 0;  //  Active high channel 2
    TIM3_CCER1_CC1E = 1;  //  Capture compare channel 1 output enable
    TIM3_CCER1_CC2E = 1;  //  Capture compare channel 2 output enable
    TIM3_CCMR1_OC1M = 6;  //  PWM Mode 1 - active if counter < CCR1, inactive otherwise.
    TIM3_CCMR2_OC2M = 6;  //  PWM Mode 1 - active if counter < CCR2, inactive otherwise.
    TIM3_CR1_CEN    = 1;  //  Finally enable the timer
} // setup_timers()

/*-----------------------------------------------------------------------------
  Purpose  : This routine initialises all the GPIO pins of the STM8 uC.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
void setup_gpio_ports(void)
{
    // UART1 (PA5,PA4) is controlled by UART device, PA3 is free
    PA_DDR     &= ~FLOW1; // Set as inputs
    PA_CR1     |=  FLOW1; // Enable pull-up
    PA_CR2     |=  FLOW1; // Enable interrupt 
    EXTI_CR1_PAIS = 0x02; // PORTA external interrupt to falling edge only
    
    // PORTB: VALVE8..VALVE1
    PB_DDR     |=   VALVES; // Set as output
    PB_CR1     |=   VALVES; // Set to push-pull
    PB_ODR     &=  ~VALVES; // All valves off at power-up

    // SPI (PC7=MISO, PC6=MOSI, PC5=CLK) is controlled by SPI device
    // PWM-outputs (PC2=BK_PWM, PC1=HLT_PWM) are controlled by Timer outputs
    PC_DDR     |=  (BOIL_230V | HLT_230V); // Set as outputs
    PC_CR1     |=  (BOIL_230V | HLT_230V); // Set to Push-Pull
    PC_ODR     &= ~(BOIL_230V | HLT_230V); // Outputs are OFF

    // UART3 (PD6, PD5) is controlled by UART device
    // Buzzer (PD4) is enabled by setting AFR7 bit in OPT2 option byte,
    // Set ST-LINK->Option Bytes...->AFR7 to 'Alternate Active'
    // PWM-outputs (PD2=HLT_SSR, PD0=BK_SSR) are controlled by Timer outputs
    // SSR-outputs control 3-phase electric heating elements
    PD_ODR     |=  SPI_CS_LEDS;                                          // MAX7219 CS is disabled
    PD_DDR     |=  (SPI_CS_LEDS | BK_SSR3 | BK_SSR2 | BK_SSR1 | BUZZER); // SSR outputs
    PD_CR1     |=  (SPI_CS_LEDS | BK_SSR3 | BK_SSR2 | BK_SSR1 | BUZZER); // Set to Push-Pull
    PD_ODR     &= ~(BK_SSR3 | BK_SSR2 | BK_SSR1 | BUZZER);               // SSR Outputs are off
    
    PE_ODR     |=  (SCL1 | SDA1 | SCL0 | SDA0 | SPI_SS); // Must be set here, or I2C will not work
    PE_DDR     |=  (SCL1 | SDA1 | SCL0 | SDA0 | SPI_SS); // Set as outputs
    PE_CR1     |=  (SCL1 | SDA1 | SCL0 | SDA0 | SPI_SS); // Set to push-pull
    PE_DDR     &= ~(FLOW2 | FLOW3 | FLOW4);     // Set as inputs
    PE_CR1     &= ~(FLOW2 | FLOW3 | FLOW4);     // Enable pull-up
    PE_CR2     |=  (FLOW2 | FLOW3 | FLOW4);     // Enable Interrupt 
    EXTI_CR2_PEIS = 0x02;                       // PORTE external interrupt to falling edge only
    
    // LM35 (PF0) is controlled by ADC device, PF7-PF5 are free
    // SSR-outputs control 3-phase electric heating elements
    PF_DDR     |=  (PUMP_230V | PUMP2_230V | HLT_SSR_ALL); // Set as outputs
    PF_CR1     |=  (PUMP_230V | PUMP2_230V | HLT_SSR_ALL); // Set to push-pull
    PF_ODR     &= ~(PUMP_230V | PUMP2_230V | HLT_SSR_ALL); // Disable pumps
    
    // PG5 is free
    PG_ODR     |=  (SCL2 | SDA2); // Must be set here, or I2C will not work
    PG_DDR     |=  (SCL2 | SDA2 | IRQ_LED | BG_LED | ALIVE_LEDS | SPI_NRESET); // Set as outputs
    PG_CR1     |=  (SCL2 | SDA2 | IRQ_LED | BG_LED | ALIVE_LEDS | SPI_NRESET); // Set to push-pull
    PG_ODR     |=  SPI_NRESET; // disable active-low reset for Wiz550io device
    PG_ODR     &= ~(IRQ_LED | BG_LED | ALIVE_LEDS); // disable leds
} // setup_gpio_ports()
