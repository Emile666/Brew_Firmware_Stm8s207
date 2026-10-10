/*==================================================================
  File Name: brew_stm8s207.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This is the main file for brew_stm8s207.c
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
  ------------------------------------------------------------------
  Revision 2.04  2023/10/03 Emile
  - Revision history before rev. 2.00 removed
  - Code clean-up
  - 3 phase electric heating enabled
  - Delayed-start now set to 100%

  Revision 2.03  2023/10/02  Emile
  - MAX7219 PCB and Ethernet now work together 

  Revision 2.00  2021/03/31 12:20  Emile
  - Ported from Brew_Arduino rev. r1.39 for e-brew PCB v4.01 
  ================================================================== */ 
#include "brew_stm8s207.h"
#include "stm8_hw_init.h"
#include "w5500.h"
#include "Ethernet.h"
#include "Udp.h"
#include "one_wire.h"
#include "max7219.h"
      
extern char rs232_inbuf[];

// Global variables
uint8_t      local_ip[4]      = {0,0,0,0}; // local IP address, gets a value from init_WIZ550IO_module() -> dhcp_begin()
uint16_t     local_port;                   // local port number read back from wiz550i module
const char  *ebrew_revision   = "$Revision: 2.09 $"; // ebrew revision number
bool         ethernet_WIZ550i = true;		     // true = start WIZ550i at power-up

// The following variables are defined in Udp.c
extern uint8_t  remoteIP[4]; // remote IP address for the incoming packet whilst it's being processed 
extern uint16_t remotePort;  // remote port for the incoming packet whilst it's being processed 
extern uint8_t  _sock;       // socket ID for Wiz5100
unsigned char   udp_rcv_buf[UDP_TX_PACKET_MAX_SIZE]; // buffer to hold incoming packet]

// The following variables are needed for the buzzer
extern bool     bz_on;      // true = buzzer is enabled
extern uint8_t  bz_rpt_max; // number of beeps to make
extern uint8_t  bz_freq;    // Buzzer frequency

//-----------------------------------
// Electric Heating variables
//-----------------------------------
uint8_t  hlt_elec1_pwm = 0;       // PWM signal (0-100 %) for HLT Electric Heating 1
uint8_t  hlt_elec2_pwm = 0;       // PWM signal (0-100 %) for HLT Electric Heating 2
uint8_t  hlt_elec3_pwm = 0;       // PWM signal (0-100 %) for HLT Electric Heating 3
pwmtime  pwmhlt1;                 // Struct for HLT Electric Heater 1 Slow SSR signal
pwmtime  pwmhlt2;                 // Struct for HLT Electric Heater 2 Slow SSR signal
pwmtime  pwmhlt3;                 // Struct for HLT Electric Heater 3 Slow SSR signal
uint8_t  bk_elec1_pwm = 0;        // PWM signal (0-100 %) for Boil-kettle Electric Heating 1
uint8_t  bk_elec2_pwm = 0;        // PWM signal (0-100 %) for Boil-kettle Electric Heating 2
uint8_t  bk_elec3_pwm = 0;        // PWM signal (0-100 %) for Boil-kettle Electric Heating 3
pwmtime  pwmbk1;                  // Struct for Boil-kettle Electric Heater 1 Slow SSR signal
pwmtime  pwmbk2;                  // Struct for Boil-kettle Electric Heater 2 Slow SSR signal
pwmtime  pwmbk3;                  // Struct for Boil-kettle Electric Heater 3 Slow SSR signal
uint8_t  elec_htrs = 0x00;        // Bit-define for every electrical phase of HLT and BK

//-----------------------------------
// LM35 parameters and variables
//-----------------------------------
ma       lm35_ma;                   // struct for LM35 moving_average filter
uint16_t lm35_temp;                 // LM35 Temperature in E-2 °C

//------------------------------------------------
// THLT parameters and variables (I2C)
//------------------------------------------------
int16_t  temp_slope_87  = 512;      // Temperature slope-limiter is 4 °C/ 2 sec. * 128
ma       thlt_ma;                   // struct for THLT moving_average filter
int16_t  thlt_old_87;               // Previous value of thlt_temp_87
int16_t  thlt_temp_87;              // THLT Temperature from LM92 in °C * 128
uint8_t  thlt_err = 0;              // 1 = Read error from LM92
uint16_t hlt_dist;                  // HLT distance in mm from ultrasonic sensor

//-----------------------------------------------------
// THLT parameters and variables (One-Wire). Used for:
// 1) Backup for THLT-I2C
// 2) Can be used as extra temp. sensor
//-----------------------------------------------------
ma       thlt_ow_ma;                // struct for THLT_OW moving_average filter
int16_t  thlt_ow_old_87;            // Previous value of thlt_ow_87
int16_t  thlt_ow_87;                // THLT Temperature from DS18B20 in °C * 128
uint8_t  thlt_ow_err = 0;	    // 1 = Read error from DS18B20
uint8_t  thlt_ow_num = 0;           // Number of DS18B20 THLT sensors connected
int16_t  thlt_ow2_87;               // THLT Temperature from 2nd DS18B20 in °C * 128
uint8_t  thlt_ow2_err = 0;	    // 1 = Read error from DS18B20

//------------------------------------------------
// TMLT parameters and variables (I2C)
//------------------------------------------------
ma       tmlt_ma;                   // struct for TMLT moving_average filter
int16_t  tmlt_old_87;               // Previous value of tmlt_temp_87
int16_t  tmlt_temp_87;              // TMLT Temperature from LM92 in °C * 128
uint8_t  tmlt_err = 0;              // 1 = Read error from LM92

//-----------------------------------------------------
// TMLT parameters and variables (One-Wire). Used for:
// 1) Backup for TMLT-I2C
// 2) Can be used as extra temp. sensor
//-----------------------------------------------------
ma       tmlt_ow_ma;                // struct for TMLT_OW moving_average filter
int16_t  tmlt_ow_old_87;            // Previous value of tmlt_ow_87
int16_t  tmlt_ow_87;                // TMLT Temperature from DS18B20 in °C * 128
uint8_t  tmlt_ow_err = 0;	    // 1 = Read error from DS18B20
uint8_t  tmlt_ow_num = 0;           // Number of DS18B20 TMLT sensors connected
int16_t  tmlt_ow2_87;               // TMLT Temperature from 2nd DS18B20 in °C * 128
uint8_t  tmlt_ow2_err = 0;	    // 1 = Read error from DS18B20

//------------------------------------------------
// TCFC parameters and variables (One-Wire only)
//------------------------------------------------
ma       tcfc_ma;                   // struct for TCFC moving_average filter
int16_t  tcfc_old_87;               // Previous value of tcfc_temp_87
int16_t  tcfc_temp_87;              // TCFC Temperature in °C * 128
uint8_t  tcfc_err = 0;              // 1 = Read error from DS18B20  
uint8_t  tcfc_ow_num = 0;           // Number of DS18B20 TCFC sensors connected

//------------------------------------------------
// TBOIL parameters and variables (One-Wire only)
//------------------------------------------------
ma       tboil_ma;                  // struct for TBOIL moving_average filter
int16_t  tboil_old_87;              // Previous value of tboil_temp_87
int16_t  tboil_temp_87;             // TBOIL Temperature in °C * 128
uint8_t  tboil_err = 0;             // 1 = Read error from DS18B20
uint8_t  tboil_ow_num = 0;          // Number of DS18B20 TBOIL sensors connected

uint32_t flow_hlt_mlt  = 0UL; // Flow 1
uint32_t flow_mlt_boil = 0UL; // Flow 2
uint32_t flow_cfc_out  = 0UL; // Flow 3 Count from flow-sensor at output of CFC
uint32_t flow4         = 0UL; // Flow 4 Count from HLT-CFC to MLT-top return
uint32_t flow_old1     = 0UL;
uint32_t flow_old2     = 0UL;
uint32_t flow_old3     = 0UL;
uint32_t flow_old4     = 0UL;

//------------------------------------------------
// Delayed-start variables
//------------------------------------------------
bool     delayed_start_enable  = false;          // true = delayed start is enabled
uint16_t delayed_start_time    = 0;              // delayed start time in 2 sec. counts 
uint16_t delayed_start_timer1;                   // timer to countdown until delayed start
uint8_t  delayed_start_std     = DEL_START_INIT; // std number, start in INIT state

//------------------------------------------------
// Frontpanel LEDs are controlled by the MAX7219
//------------------------------------------------
uint8_t max7219dig2 = 0; // LEDs: Alive, Delayed-Start, Pump2, Pump1, F4, F3, F2, F1
uint8_t max7219dig0 = 0; // LEDs: HLT gas, HLT E3, HLT E2, HLT E1, BK gas, BK E3, BK E2, BK E1

/*------------------------------------------------------------------
  Purpose  : This is the delayed-start routine which is called by 
             lm35_task() every 2 seconds. 
  Variables: -
  Returns  : -
  ------------------------------------------------------------------*/
void process_delayed_start(void)
{
    uint8_t         led_tmr_max = 0; // blink every x seconds  
    static uint8_t  led_tmr     = 0; // Timer for blinking of RED led
    static uint16_t eep_tmr;         // minute counter for eeprom write
    static uint16_t timer2;          // timer for max. burn in minutes (max. 120 minutes)
    
    switch (delayed_start_std)
    {
        case DEL_START_INIT: // Delayed-start is not enabled
            if (delayed_start_enable)
            {
                delayed_start_timer1 = eep_read16(EEPARB_DEL_START_TMR1); // read value from eeprom
                delayed_start_time   = eep_read16(EEPARB_DEL_START_TIME); // read value from eeprom
                eep_tmr              = 0;                      // reset eeprom minute timer
                delayed_start_std    = DEL_START_TMR;          // start countdown timer
            } // if				 
            led_tmr_max = 0; // no blinking
            break;
            
        case DEL_START_TMR: // The delayed-start timer is running, no time-out yet
            if (!delayed_start_enable)
            {
                eep_write8(EEPARB_DEL_START_ENA,false); // reset enable in eeprom
                delayed_start_std = DEL_START_INIT;
            } // if				 
            else if (++delayed_start_timer1 >= delayed_start_time)
            {   // delayed-start counted down
                timer2            = 0;              // init. burn-timer 
                delayed_start_std = DEL_START_BURN; // start HLT burner
            } // if
            if (++eep_tmr >= 30)
            {   // save in eeprom every minute
                eep_tmr = 0;
                eep_write16(EEPARB_DEL_START_TMR1,delayed_start_timer1);
            } // if
            led_tmr_max = 5; // blink once every 10 seconds
            break;
            
        case DEL_START_BURN: // Delayed-start time-out, HLT-burner is burning
            if (!delayed_start_enable)
            {
                eep_write8(EEPARB_DEL_START_ENA,false); // reset enable in eeprom
                delayed_start_std = DEL_START_INIT;
                process_pwm_signal(PWM_HLT,0,ELEC_HTR1|ELEC_HTR2|ELEC_HTR3); // disable heaters
            } // if
            else 
            {	// For delayed start: only enable HLT electric heaters with fixed percentage	 
                process_pwm_signal(PWM_HLT,DEL_START_ELEC_PWM,ELEC_HTR1|ELEC_HTR2|ELEC_HTR3);
                if (++timer2 >= DEL_START_MAX_BURN_TIME) 
                {   // Safety feature, set burn-time to max. of 2 hours
                    delayed_start_enable = false; // prevent another burn
                    delayed_start_std    = DEL_START_INIT;
                    eep_write8(EEPARB_DEL_START_ENA,false); // reset enable in eeprom
                } // if
            } // else				 
            led_tmr_max = 1; // blink once every 4 seconds
            break;
            
        default: delayed_start_std = DEL_START_INIT;
        break;
    } // switch
    if ((led_tmr_max > 0) && (++led_tmr > led_tmr_max))
    {    // blink once every led_tmr_max seconds
        led_tmr      = 0;
        ALIVE_LED_Rb = 1; // red LED on
        max7219dig2 |= MAX7219D2DS; // Frontpanel LED on
    } // if
    else 
    {
        ALIVE_LED_Rb = 0; // red LED off
        max7219dig2 &= ~MAX7219D2DS; // Frontpanel LED off
    } // else
} // process_delayed_start()

/*------------------------------------------------------------------
  Purpose  : This function initializes the pwm_2_time structs that
             are used for the HLT electric heating-elements.
  Variables: 
          p: pointer to struct to initialize
	   mask: bit-defines of electric heaters in elec_htrs variable.
	  on1st: phase of PWM signal, true = 1 first, false = 0 first
  Returns  : -
  ------------------------------------------------------------------*/
void init_pwm_time(pwmtime *p, uint8_t mask, bool on1st)
{
	p->std    = EL_HTR_OFF; // default state
	p->mask   = mask;       // bit-define for elec_htrs
	p->on1st  = on1st;      // true = high first, then low
} // init_pwm_time()

/*-----------------------------------------------------------------------------
  Purpose  : Converts a PWM signal into a time-division signal of 100 * 100 msec.
             This is used to control the electric heating-elements.
             This routine should be called from pwm_task() every 100 msec.
	     It uses the pwm values which are set by process_pwm_signal().
  Variables: p: pointer to pwmtime struct. There is a separate struct for each
                electric heating-element.
	  cntr: time-division counter, counts from 1 to 100 and back again.
           pwm: PWM percentage.
  Returns  : -
  ---------------------------------------------------------------------------*/
void pwm_2_time(pwmtime *p, uint8_t cntr, uint8_t pwm)
{
	int8_t  bt,et;
	
	bt  = p->on1st ? 25 : 75; // time center-point
	bt -= (pwm>>1);           // start-time for a 1
	et  = bt + pwm;           // end-time for a 1
	if      (bt < 0)   { bt = 0        ; et = pwm; }
	else if (et > 100) { bt = 100 - pwm; et = 100; }

	switch (p->std)
	{
		case EL_HTR_OFF:
			//------------------------------------------------------------
			// Goto ON-state if cntr in [bt,et] && pwm>0
			//------------------------------------------------------------
			elec_htrs &= ~p->mask;
			if ((pwm > 0) && (cntr >= bt) && (cntr < et))
			{  
				p->std = EL_HTR_ON; // go to ON-state
			} // if
			// else: pwm=0 or outside 1-time, continue in this state
			break;

		case EL_HTR_ON:
			//---------------------------------------------------------
			// Goto OFF-state if pwm=0 or cntr>et && pwm<100
			//---------------------------------------------------------
			elec_htrs |= p->mask;       // set electric heater ON
			if ((pwm == 0) || ((cntr >= et) && (pwm < 100)))
			{   
				p->std = EL_HTR_OFF;   // go to 'OFF' state
			} // if
			// else: pwm>0 && ((cntr < et) || (pwm>=100))
			break;

		default: 
		    p->std = EL_HTR_OFF;
		    break;	
	} // switch 
} // pwm_2_time()

/*-----------------------------------------------------------------------------
  Purpose  : This task converts a PWM signal into a time-division signal of 
             100 * 100 msec. This routine should be called from the scheduler 
	     every 100 msec. It uses the hlt_elecx_pwm and bk_elecx_pwm signals 
	     which were set by the process_pwm_signal(). This variable will only 
	     get a non-zero number when the corresponding electrical 
	     heating-element is enabled.
			 
	     Priority: the HLT and BK heating-elements can be sourced from the
	     same outlet. If the BK starts to heat, it may be possible
	     for the HLT to draw current too. In order to prevent this,
	     the BK has priority over the HLT. In other words, if the
	     BK heating-element is energized, the HLT heating-element
	     on the same outlet is disabled.
  Variables: - 
  Returns  : -
  ---------------------------------------------------------------------------*/
void pwm_task(void)
{
   static uint8_t cntr = 1; // time-division counter
   
   pwm_2_time(&pwmbk1 ,cntr,bk_elec1_pwm);   // BK heating-element 1
   pwm_2_time(&pwmbk2 ,cntr,bk_elec2_pwm);   // BK heating-element 2
   pwm_2_time(&pwmbk3 ,cntr,bk_elec3_pwm);   // BK heating-element 3
   
   pwm_2_time(&pwmhlt1,cntr,hlt_elec1_pwm);  // HLT heating-element 1
   pwm_2_time(&pwmhlt2,cntr,hlt_elec2_pwm);  // HLT heating-element 2
   pwm_2_time(&pwmhlt3,cntr,hlt_elec3_pwm);  // HLT heating-element 3
   
   // If both heaters of the same phase are on, disable the HLT phase
   if ((elec_htrs & (HTR_BK1 | HTR_HLT1)) == (HTR_BK1 | HTR_HLT1)) elec_htrs &= ~HTR_HLT1;
   if ((elec_htrs & (HTR_BK2 | HTR_HLT2)) == (HTR_BK2 | HTR_HLT2)) elec_htrs &= ~HTR_HLT2;
   if ((elec_htrs & (HTR_BK3 | HTR_HLT3)) == (HTR_BK3 | HTR_HLT3)) elec_htrs &= ~HTR_HLT3;
   
   if (elec_htrs & pwmhlt1.mask)
        { HLT_SSR1b = true;  max7219dig0 |=  MAX7219D0H1; } // Enable  HLT heater 1 
   else { HLT_SSR1b = false; max7219dig0 &= ~MAX7219D0H1; } // Disable HLT heater 1
   if (elec_htrs & pwmhlt2.mask)
        { HLT_SSR2b = true;  max7219dig0 |=  MAX7219D0H2; } // Enable  HLT heater 2
   else { HLT_SSR2b = false; max7219dig0 &= ~MAX7219D0H2; } // Disable HLT heater 2
   if (elec_htrs & pwmhlt3.mask)
        { HLT_SSR3b = true;  max7219dig0 |=  MAX7219D0H3; } // Enable  HLT heater 3
   else { HLT_SSR3b = false; max7219dig0 &= ~MAX7219D0H3; } // Disable HLT heater 3
   
   if (elec_htrs & pwmbk1.mask)
        { BK_SSR1b = true;  max7219dig0 |=  MAX7219D0B1; } // Enable  BK heater 1
   else { BK_SSR1b = false; max7219dig0 &= ~MAX7219D0B1; } // Disable BK heater 1
   if (elec_htrs & pwmbk2.mask)
        { BK_SSR2b = true;  max7219dig0 |=  MAX7219D0B2; } // Enable  BK heater 2
   else { BK_SSR2b = false; max7219dig0 &= ~MAX7219D0B2; } // Disable BK heater 2
   if (elec_htrs & pwmbk3.mask)
        { BK_SSR3b = true;  max7219dig0 |=  MAX7219D0B3; } // Enable  BK heater 3
   else { BK_SSR3b = false; max7219dig0 &= ~MAX7219D0B3; } // Disable BK heater 3

   // Update frontpanel LEDs
   if (flow_hlt_mlt != flow_old1)
   {
        max7219dig2 ^=  MAX7219D2F1; // toggle LED
        flow_old1 = flow_hlt_mlt;
   } // if
   else max7219dig2 &= ~MAX7219D2F1; // LED off
   if (flow_mlt_boil != flow_old2)
   {    
        max7219dig2 ^=  MAX7219D2F2; // toggle LED
        flow_old2 = flow_mlt_boil;
   } // if
   else max7219dig2 &= ~MAX7219D2F2; // LED off
   if (flow_cfc_out != flow_old3)
   {
        max7219dig2 ^=  MAX7219D2F3; // toggle LED
        flow_old3 = flow_cfc_out;
   } // if
   else max7219dig2 &= ~MAX7219D2F3; // LED off
   if (flow4 != flow_old4)
   {
        max7219dig2 ^=  MAX7219D2F4; // toggle LED
        flow_old4 = flow4;
   } // if
   else max7219dig2 &= ~MAX7219D2F4; // LED off
   max7219_write(MAX7219_DIG0 | max7219dig0);
   max7219_write(MAX7219_DIG2 | max7219dig2);
   
   uart3_command_handler(); // get ultrasonic sensor info
   
   if (++cntr > 100) cntr = 1; // reset time-division counter
} // pwm_task()

/*--------------------------------------------------------------------
   Purpose  : This task is called every 2 seconds and does the following:
             1) processes the temperature from the Hot-Liquid Tun (HLT). 
	        The first sensor to read is the LM92 (I2C sensor). If 
		that one is not present, the DS18B20 (One-Wire sensor) 
		is tried. It uses the output from ow_task(), which delivers
		a new DS18B20 reading every 2 seconds.
		Since both sensors have 4 fractional bits (1/2, 1/4, 1/8, 1/16), 
		a signed Q8.4 format would be sufficient. However all variables 
		are stored in a Q8.7 format for accuracy reasons when filtering. 
		All variables with this format have the extension _87. The 
		HLT temperature is both filtered and slope-limited.
	     2) processes the temperature from the LM35 temperature sensor. 
		This sensor is connected to ADC0. The LM35 outputs 10 mV/°C,
		VREF=1.1 V, therefore => Max. Temp. = 11000 E-2 °C
		Conversion constant = 11000 / 1023 = 10.7526882
             3) executes the delayed-start function, which will fire the
		HLT burner after a number of minutes set by the D0 command.

  Variables: thlt_old_87   : previous value of thlt_temp_87
             temp_slope_87 : the slope-limit: 2 °C/sec.
             thlt_err      : 1 = error reading from LM92 (I2C)
             thlt_ow_err   : 1 = error reading from DS18B20 (One-Wire)
             thlt_ow_87    : Temperature value from DS18B20 (One-Wire)
             thlt_ma       : the moving-average filter struct for THLT
             thlt_temp_87  : the processed HLT temperature in °C
             lm35_temp     : contains temperature in E-2 °C
             lm35_frac     : contains fractional temperature in E-2 °C
             lm35_ma       : moving-average filter struct for lm35_temp
  Returns  : -
  --------------------------------------------------------------------*/
void thlt_task(void)
{
    int16_t tmp;  // temporary variable (signed Q8.7 format)
    float   tmpf; // temporary variable
    
    thlt_old_87 = thlt_temp_87; // copy previous value of thlt_temp
    tmp         = lm92_read(I2C_CH1, &thlt_err); // returns a signed Q8.7 format
    if (!thlt_err) 
    {	// filter if I2C-sensor has been read successfully
        slope_limiter(temp_slope_87, thlt_old_87, &tmp);
        thlt_temp_87 = (int16_t)(moving_average(&thlt_ma, (float)tmp) + 0.5);
    } // if
    
    tmpf      = read_adc(AD_LM35) * LM35_CONV; // Read LM35 temperature sensor
    lm35_temp = (uint16_t)moving_average(&lm35_ma, tmpf);
    process_delayed_start();                   // Process delayed-start function
} // thlt_task()

/*--------------------------------------------------------------------
  Purpose  : This is the task that processes the temperature from
	     the Mash-Lauter Tun (MLT). The first sensor to read is the
	     LM92 (I2C sensor). If that one is not present, the DS18B20
	     (One-Wire sensor) is tried. Since both sensors have 4
	     fractional bits (1/2, 1/4, 1/8, 1/16), a signed Q8.4 format
	     would be sufficient. However all variables are stored in a
	     Q8.7 format for accuracy reasons when filtering. All variables
	     with this format have the extension _87.
	     The MLT temperature is both filtered and slope-limited.
	     This task is called every 2 seconds and uses the output from
	     ow_task(), which delivers a new DS18B20 reading every 2 seconds.
  Variables: tmlt_old_87   : previous value of tmlt_temp_87
	     temp_slope_87 : the slope-limit: 2 °C/sec.
	     tmlt_err      : 1 = error reading from LM92 (I2C)
	     tmlt_ow_err   : 1 = error reading from DS18B20 (One-Wire)
	     tmlt_ow_87    : Temperature value from DS18B20 (One-Wire)
	     tmlt_ma       : the moving-average filter struct for TMLT
	     tmlt_temp_87  : the processed MLT temperature in °C
  Returns  : -
  --------------------------------------------------------------------*/
void tmlt_task(void)
{
    int16_t tmp; // temporary variable (signed Q8.7 format)
    
    tmlt_old_87 = tmlt_temp_87; // copy previous value of tmlt_temp
    tmp         = lm92_read(I2C_CH2, &tmlt_err); // returns a signed Q8.7 format
    if (!tmlt_err)
    {	// filter if I2C-sensor has been read successfully
        slope_limiter(temp_slope_87, tmlt_old_87, &tmp);
        tmlt_temp_87 = (int16_t)(moving_average(&tmlt_ma, (float)tmp) + 0.5);
    } // else
} // tmlt_task()

/*--------------------------------------------------------------------
  Purpose  : This is the task that processes the temperatures from
             the HLT One-Wire sensors. A maximum of two sensors are
             supported. The sensor has its own DS2482 I2C-to-One-Wire 
             bridge. Since this sensor has 4 fractional bits (1/2, 1/4, 
             1/8, 1/16), a signed Q8.4 format would be sufficient. 
             However all variables are stored in a
             Q8.7 format for accuracy reasons when filtering. All variables
             with this format have the extension _87.
             This task is called every second so that every 2 seconds a new
             temperature is present.
  Variables: thlt_ow_87 : HLT temperature read from sensor in Q8.7 format
	     thlt_ow_err: 1=error
             thlt_ow2_87 : HLT temperature read from 2nd sensor in Q8.7 format
	     thlt_ow2_err: 1=error
  Returns  : -
  --------------------------------------------------------------------*/
void owh_task(void)
{
    static int owh_std = 0; // internal state
    int16_t    tmp;         // temporary variable (signed Q8.7 format)
    
    switch (owh_std)
    {   
    case 0: // Start Conversion for all sensors
        if (ds18b20_start_conversion(DS2482_THLT_BASE))
        {
            owh_std = 1; // Only read OW device if DS2482 is found
        } // if
        else
        {   // DS2482 not found on I2C bus
            thlt_ow_err = thlt_ow2_err = 1; // set both error flags to 1
        } // else
        break;
    case 1: // Read Thlt_ow device
        thlt_ow_old_87 = thlt_ow_87; // copy previous value of thlt_ow
        tmp = ds18b20_read(DS2482_THLT_BASE, 0, &thlt_ow_err,1);
        if (!thlt_ow_err)
        {
            slope_limiter(temp_slope_87, thlt_ow_old_87, &tmp);
            thlt_ow_87 = (int16_t)(moving_average(&thlt_ow_ma, (float)tmp) + 0.5);
        } // if
        if (thlt_ow_num > 1)
        {   // More than 1 sensor?
            thlt_ow2_87 = ds18b20_read(DS2482_THLT_BASE, 1, &thlt_ow2_err,1);
        } // if
        else thlt_ow2_err = 1;
        owh_std = 0;
        break;
    } // switch
    BG_LEDb = !BG_LEDb; // alive-led for background-processes
} // owh_task()

/*--------------------------------------------------------------------
  Purpose  : This is the task that processes the temperatures from
             the MLT One-Wire sensors. A maximum of two sensors are
             supported. The sensor has its own DS2482 I2C-to-One-Wire 
             bridge. Since this sensor has 4 fractional bits (1/2, 1/4, 
             1/8, 1/16), a signed Q8.4 format would be sufficient. 
             However all variables are stored in a
             Q8.7 format for accuracy reasons when filtering. All variables
             with this format have the extension _87.
             This task is called every second so that every 2 seconds a new
             temperature is present.
  Variables: tmlt_ow_87 : MLT temperature read from sensor in Q8.7 format
	     tmlt_ow_err: 1=error
             tmlt_ow2_87 : HLT temperature read from 2nd sensor in Q8.7 format
	     tmlt_ow2_err: 1=error
Returns  : -
  --------------------------------------------------------------------*/
void owm_task(void)
{
    static int owm_std = 0; // internal state
    int16_t    tmp;         // temporary variable (signed Q8.7 format)
    
    switch (owm_std)
    {   
    case 0: // Start conversion
        if (ds18b20_start_conversion(DS2482_TMLT_BASE))
        {
            owm_std = 1; // Only read OW device if DS2482 is found
        } // if
        else
        {   // DS2482 not found on I2C bus
            tmlt_ow_err = tmlt_ow2_err = 1; // set both error flags to 1
        } // else
        break;
    case 1: // Read Tmlt_ow device
        tmlt_ow_old_87 = tmlt_ow_87; // copy previous value of tmlt_ow
        tmp = ds18b20_read(DS2482_TMLT_BASE,0,&tmlt_ow_err,1);
        if (!tmlt_ow_err)
        {
            slope_limiter(temp_slope_87, tmlt_ow_old_87, &tmp);
            tmlt_ow_87 = (int16_t)(moving_average(&tmlt_ow_ma, (float)tmp) + 0.5);
        } // if
        if (tmlt_ow_num > 1)
        {   // More than 1 sensor?
            tmlt_ow2_87 = ds18b20_read(DS2482_TMLT_BASE, 1, &tmlt_ow2_err,1);
        } // if
        else tmlt_ow2_err = 1; // 1 = error
        owm_std = 0;
        break;
    } // switch
} // owm_task()

/*--------------------------------------------------------------------
  Purpose  : This is the task that processes the temperatures from
             the Boil-kettle One-Wire sensor. The sensor has its
             own DS2482 I2C-to-One-Wire bridge. Since this sensor has 4
             fractional bits (1/2, 1/4, 1/8, 1/16), a signed Q8.4 format
             would be sufficient. However all variables are stored in a
             Q8.7 format for accuracy reasons when filtering. All variables
             with this format have the extension _87.
             This task is called every second so that every 2 seconds a new
             temperature is present.
  Variables: tboil_temp_87 : Boil-kettle temperature read from sensor in Q8.7 format
	     tboil_err     : 1=error
  Returns  : -
  --------------------------------------------------------------------*/
void owb_task(void)
{
    static int owb_std = 0; // internal state
    int16_t    tmp;         // temporary variable (signed Q8.7 format)
    
    switch (owb_std)
    {   
    case 0: // Start conversion
        if (ds18b20_start_conversion(DS2482_TBOIL_BASE))
        {
            owb_std = 1; // Only read OW device if DS2482 is found
        } // if
        break;
    case 1: // Read Tboil device
        tboil_old_87 = tboil_temp_87; // copy previous value of tboil_temp
        tmp = ds18b20_read(DS2482_TBOIL_BASE,0,&tboil_err,1);
        if (!tboil_err)
        {
            slope_limiter(temp_slope_87, tboil_old_87, &tmp);
            tboil_temp_87 = (int16_t)(moving_average(&tboil_ma, (float)tmp) + 0.5);
        } // if
        owb_std = 0; // only 1 sensor is currently supported
        break;
    } // switch
} // owb_task()

/*--------------------------------------------------------------------
  Purpose  : This is the task that processes the temperatures from
             the Counterflow Chiller (CFC) One-Wire sensor. The sensor has
             its own DS2482 I2C-to-One-Wire bridge. Since this sensor has 4
             fractional bits (1/2, 1/4, 1/8, 1/16), a signed Q8.4 format
             would be sufficient. However all variables are stored in a
             Q8.7 format for accuracy reasons when filtering. All variables
             with this format have the extension _87.
             This task is called every second so that every 2 seconds a new
             temperature is present.
  Variables: tcfc_temp_87 : CFC temperature read from sensor in Q8.7 format
	     tcfc_err     : 1=error
  Returns  : -
  --------------------------------------------------------------------*/
void owc_task(void)
{
    static int owc_std = 0; // internal state
    int16_t    tmp;         // temporary variable (signed Q8.7 format)
    
    switch (owc_std)
    {   
    case 0: // Start conversion
        if (ds18b20_start_conversion(DS2482_TCFC_BASE))
        {
            owc_std = 1; // Only read OW device if DS2482 is found
        } // if
        break;
    case 1: // Read Tcfc device
        tcfc_old_87 = tcfc_temp_87; // copy previous value of tcfc_temp
        tmp = ds18b20_read(DS2482_TCFC_BASE,0,&tcfc_err,1);
        if (!tcfc_err)
        {
            slope_limiter(temp_slope_87, tcfc_old_87, &tmp);
            tcfc_temp_87 = (int16_t)(moving_average(&tcfc_ma, (float)tmp) + 0.5);
        } // if
        owc_std = 0;  // only 1 sensor is currently supported
        break;
    } // switch
} // owc_task()

/*------------------------------------------------------------------
  Purpose  : This function prints a welcome message to the serial
             port together with the current CVS revision number.
  Variables: 
        ver: string with version info
  Returns  : -
  ------------------------------------------------------------------*/
void print_ebrew_revision(char *ver)
{
    uint8_t len;
    char s[30];
    
    strcpy(ver, "E-Brew stm8s207 rev.");  // welcome message, assure that all is well!
    len = strlen(ebrew_revision) - 13;  // just get the rev. number
    strncpy(s,&ebrew_revision[11],len); // example: " 1.3"
    s[len]   = '\n';
    s[len+1] = '\0';
    strcat(ver,s);
} // print_ebrew_revision()

/*------------------------------------------------------------------
  Purpose  : This function prints the IP address to the serial port.
  Variables: The IP-address to print
  Returns  : -
  ------------------------------------------------------------------*/
void print_IP_address(uint8_t *ip)
{
    char s[30];
    uint8_t i;
    
    for (i = 0; i < 4; i++)
    {
        sprintf(s,"%d",ip[i]);
        uart_printf(s);
        if (i < 3) uart_printf(".");
    } // for
} // print_IP_address()

/*-----------------------------------------------------------------------
  Purpose  : This function inits the WIZ550i Ethernet module by
             calling w5500_init() and spi_init(). If a valid MAC address
	     is detected, dhcp_begin() is started.
  Variables: The IP-address to print
  Returns  : 1: success, 0: WIZ550IO not present
  -----------------------------------------------------------------------*/
uint8_t init_WIZ550IO_module(void)
{
    char     s[30];   // Needed for uart_printf() and sprintf()
    uint8_t  bufr[8]; // Needed for w5500_read_common_register()
    uint8_t  ret,x;
    uint16_t y;
    
    //---------------------------------------------------------------
    // Reset WIZ550IO Ethernet module
    //---------------------------------------------------------------
    SPI_NRESETb = 0;           // Hardware reset for WIZ550io
    delay_msec(2);             // At least 500 usec according to datasheet
    SPI_NRESETb = 1;           // Disable hardware reset for WIZ550io
    delay_msec(300);           // Giver W5500 time to configure itself (at least 150 msec)

    ret = Ethernet_begin();    // includes w5500_init() & dhcp_begin()
    if (ret == 0)              // Error, no WIZ550IO module found
    {
        ethernet_WIZ550i = false; // No ETH mode, switch back to USB
        return 0;
    } // if	
    
    x = udp_begin(EBREW_PORT_NR);           // init. UDP protocol
    sprintf(s,"udp_begin():%s, sock=%d\n",x ? "ok":"err",_sock);   uart_printf(s);
    x = w5500_read_socket_register(_sock, Sn_MR, bufr);
    sprintf(s,"Sn_MR=%d, ",x);                        uart_printf(s);
    local_port = w5500_read_socket_register(_sock, Sn_PORT, bufr);
    sprintf(s,"Sn_PORT=%d, ",local_port);		  uart_printf(s);
    y = w5500_getTXFreeSize(_sock);
    sprintf(s,"Sn_TXfree=%d, ",y);			  uart_printf(s);
    y = w5500_getRXReceivedSize(_sock);
    sprintf(s,"Sn_RXrecv=%d\n",y);			  uart_printf(s);
    w5500_read_common_register(GAR, bufr);
    uart_printf("Gateway IP : "); print_IP_address(bufr);
    w5500_read_common_register(SUBR, bufr);
    uart_printf("\nSubnet Mask: "); print_IP_address(bufr);	
    w5500_read_common_register(SIPR, bufr);
    uart_printf("\nLocal IP   : "); print_IP_address(bufr);
    for (x = 0; x < 4; x++) local_ip[x] = bufr[x]; // copy IP address
    sprintf(s,":%d\n",local_port); // add local port as read back from wiz550io module
    uart_printf(s);
    return 1; // success
} // init_WIZ550IO_module()

/*------------------------------------------------------------------
  Purpose  : This is the main() function for the E-brew hardware.
  Variables: -
  Returns  : should never return
  ------------------------------------------------------------------*/
int main(void)
{
    char    s[30];     // Needed for uart_printf() and sprintf()
    int	    udp_packet_size;
    uint8_t clk;       // which clock is active
    uint8_t x0,x1,x2;
    
    __disable_interrupt();
    //-------------------------------------------------------------------
    // For 24 MHz, set ST-LINK->Option Bytes...->Flash_Wait_states to 1 !
    //-------------------------------------------------------------------
    clk = initialise_system_clock(HSE); // Set system-clock to 24 MHz

    uart_init(clk);            // UART init. to BAUDRATE (uart.h),8,N,1
    uart3_init(clk);           // UART3 init. to BAUDRATE3 (uart.h),8,N,1
    setup_timers(clk);         // Set Timer 2 to 1 kHz and timer1 and timer3 for PWM output
    setup_gpio_ports();        // Init. needed output-ports for LED and keys
    x0 = i2c_init_bb(I2C_CH0); // Init. I2C bus 0 for bit-banging
    x1 = i2c_init_bb(I2C_CH1); // Init. I2C bus 1 for bit-banging
    x2 = i2c_init_bb(I2C_CH2); // Init. I2C bus 2 for bit-banging
    //pwm_write(PWM_BK ,0);    // Start with 0 % duty-cycle for Boil-kettle
    //pwm_write(PWM_HLT,0);    // Start with 0 % duty-cycle for HLT
    spi_init();                // Init. SPI module: 3 MHz clock, Master mode, SPI mode 1
    max7219_init();            // Init. MAX7219 on SPI-bus
    
    check_and_init_eeprom();   // EEPROM init.
    read_eeprom_parameters();  // Read EEPROM value for ETHUSB and delayed-start
	
    //---------------------------------------------------------------
    // Init. Moving Average Filters for Measurements
    //---------------------------------------------------------------
    init_moving_average(&lm35_ma   ,MAX_MA, (float)INIT_TEMP * 100.0); // Init. MA-filter with 20 °C
    init_moving_average(&thlt_ma   ,MAX_MA, (float)INIT_TEMP * 128.0); // Init. MA-filter with 20 °C
    init_moving_average(&tmlt_ma   ,MAX_MA, (float)INIT_TEMP * 128.0); // Init. MA-filter with 20 °C
    init_moving_average(&tcfc_ma   ,MAX_MA, (float)INIT_TEMP * 128.0); // Init. MA-filter with 20 °C
    init_moving_average(&tboil_ma  ,MAX_MA, (float)INIT_TEMP * 128.0); // Init. MA-filter with 20 °C
    init_moving_average(&thlt_ow_ma,MAX_MA, (float)INIT_TEMP * 128.0); // Init. MA-filter with 20 °C
    init_moving_average(&tmlt_ow_ma,MAX_MA, (float)INIT_TEMP * 128.0); // Init. MA-filter with 20 °C
    lm35_temp     = INIT_TEMP * 100;
    thlt_temp_87  = INIT_TEMP << 7;
    tmlt_temp_87  = INIT_TEMP << 7;
    tcfc_temp_87  = INIT_TEMP << 7;
    tboil_temp_87 = INIT_TEMP << 7;
    thlt_ow_87    = INIT_TEMP << 7;
    tmlt_ow_87    = INIT_TEMP << 7;

    //----------------------------------------------------------------------------
    // Initialize Electric Heating Elements
    // HLT1 and BK1 are on the same 230 V outlet, same for HLT2/BK2 and HLT3/BK3.
    //----------------------------------------------------------------------------
    init_pwm_time(&pwmhlt1,HTR_HLT1,ON1ST);  // HLT Electric heating element 1
    init_pwm_time(&pwmhlt2,HTR_HLT2,OFF1ST); // HLT Electric heating element 2
    init_pwm_time(&pwmhlt3,HTR_HLT3,ON1ST);  // HLT Electric heating element 3
    init_pwm_time(&pwmbk1 ,HTR_BK1 ,OFF1ST); // BK  Electric heating element 1
    init_pwm_time(&pwmbk2 ,HTR_BK2 ,ON1ST);  // BK  Electric heating element 2
    init_pwm_time(&pwmbk3 ,HTR_BK3 ,OFF1ST); // BK  Electric heating element 3

    //---------------------------------------------
    // Initialize all tasks for the Brew Hardware
    //---------------------------------------------
    add_task(pwm_task  ,"pwm_task"  , 10,  100); // Electrical Heating Time-Division every 100 msec.
    add_task(owh_task  ,"owh_task"  ,120, 1000); // Process Temperature from DS18B20 HLT sensor
    add_task(owm_task  ,"owm_task"  ,220, 1000); // Process Temperature from DS18B20 MLT sensor
    add_task(owb_task  ,"owb_task"  ,320, 1000); // Process Temperature from DS18B20 Boil-kettle sensor
    add_task(owc_task  ,"owc_task"  ,420, 1000); // Process Temperature from DS18B20 CFC-output sensor
    add_task(thlt_task ,"thlt_task" ,520, 2000); // Process Temperature from THLT sensor (I2C and/or OW)
    add_task(tmlt_task ,"tmlt_task" ,620, 2000); // Process Temperature from TMLT sensor (I2C and/or OW)
	
    __enable_interrupt();       // set global interrupt enable, start task-scheduler
    uart_printf("\n");
    print_ebrew_revision(s);    // print revision number
    uart_printf(s);		// Output to COM-port for debugging
    if (ethernet_WIZ550i)       // Initialize Ethernet adapter
    {
        if (init_WIZ550IO_module())
        {   // 1 = ok, DHCP-server found
            bz_rpt_max = 1; // Sound buzzer once to indicate ethernet connection ready
            bz_on      = true;
        } // if
    } // if
    sprintf(s,"CLK: 0x%X ",clk);
    uart_printf(s);
    if      (clk == HSI) uart_printf("HSI\n");
    else if (clk == LSI) uart_printf("LSI\n");
    else if (clk == HSE) uart_printf("HSE\n");
    sprintf(s,"I2C0: %s, ", !x0 ? "reset ok" : "NOT reset"); 
    uart_printf(s);
    i2c_scan(I2C_CH0, RS232_USB); // List all devices found
    sprintf(s,"I2C1: %s, ", !x1 ? "reset ok" : "NOT reset"); 
    uart_printf(s);
    i2c_scan(I2C_CH1, RS232_USB); // List all devices found
    sprintf(s,"I2C2: %s, ", !x2 ? "reset ok" : "NOT reset"); 
    uart_printf(s);
    i2c_scan(I2C_CH2, RS232_USB); // List all devices found
  
    find_OW_devices(RS232_USB); // Find and Print all One-Wire devices to UART
   
    uart_printf("Starting main()\n");

    while(1)
    {
        dispatch_tasks();                // run the task-scheduler
        switch (rs232_command_handler()) // run command handler continuously
        {
            case ERR_CMD: uart_printf("Cmd Err\n"); 
                          break;
            case ERR_NUM: sprintf(s,"Num Err (%s)\n",rs232_inbuf);
                          uart_printf(s);  
                          break;
            default     : break;
        } // switch
        if (ethernet_WIZ550i) // only true after an E1 command
        {
            udp_packet_size = udp_parsePacket();
            if ((udp_packet_size) && (udp_packet_size < UDP_TX_PACKET_MAX_SIZE))
            {
                udp_read(udp_rcv_buf, UDP_TX_PACKET_MAX_SIZE);
                udp_rcv_buf[udp_packet_size] = '\0';
                ethernet_command_handler((char *)udp_rcv_buf);
            } // if	
        } // if
    } // while()
} // main()