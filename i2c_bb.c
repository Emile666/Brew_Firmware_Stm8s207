/*==================================================================
  File Name: i2c_bb.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This files contains the bit-banging I2C functions 
             for the STM8 uC. It is needed for every I2C device
             connected to the STM8 uC. The I2C hardware unit inside
             the STM8 is not used, since this is too complicated and
             could cause bus lock-ups.
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
#include "i2c_bb.h"
    
/*-----------------------------------------------------------------------------
  Purpose  : This function resets the I2C-bus after a lock-up. See also:
             http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
  Variables: --
  Returns  : 0 if bus cleared
             1 if SCL held low, 
             2 if SDA held low by slave clock strecht for > 2 seconds
	     3 if SDA held low after 20 clocks
  ---------------------------------------------------------------------------*/
uint8_t i2c_reset_bus(enum I2C_CH ch)
{
    int8_t   scl_cnt,sda_cnt; // must be a signed int!
    uint8_t  scl_low,sda_low;
    
    scl_in(ch); // SCL input with external pull-ups
    sda_in(ch); // SDA input with external pull-ups
    i2c_delay_5usec(400); // delay 2 msec.
    
    if (!scl_read(ch)) // check if SCL is low
    {	// if it is held low, the uC cannot become the I2C master
            return 1; // I2C bus-error. Could not clear SCL clock line held low
    } // if
    sda_low = !sda_read(ch);
    sda_cnt = 20; // > 2x9 clock
    while (sda_low && (sda_cnt-- > 0))
    {	// Note: I2C bus is open collector so do NOT drive SCL or SDA high.
            scl_out(ch);             // SCL is Push-Pull output
            scl_0(ch);               // Set SCL low
            i2c_delay_5usec(1);      // extra delay, so that even the slowest I2C devices are handled
            scl_in(ch);              // SCL is input
            i2c_delay_5usec(2);      // for > 5 us, so that even the slowest I2C devices are handled
            scl_low = !scl_read(ch); // check if SCL is low
            scl_cnt = 20;
            while (scl_low && (scl_cnt-- > 0))
            {
                    i2c_delay_5usec(20000); // delay 100 msec.
                    scl_low = !scl_read(ch);  // check if SCL is low
            } // while
            if (scl_low)
            {	// still low after 2 sec error
                    return 2; // I2C error, could not clear, SCL held low by slave clock stretch for > 2 sec.
            } // if
            sda_low = !sda_read(ch);
    } // while
    if (sda_low)
    {	// still low
            return 3; // I2C bus error, could not clear, SDA held low
    } // if
    // else pull SDA line low for Start or Repeated Start
    sda_1(ch);          // Set SDA to 1
    sda_out(ch);        // Set SDA to output
    sda_0(ch);          // Then make it low i.e. send an I2C Start or Repeated Start
    // When there's only one I2C master a Start or Repeat Start has the same function as a Stop
    // and clears the bus. A repeat Start is a Start occurring after a Start with no intervening Stop.
    i2c_delay_5usec(2); // delay 10 usec.
    sda_1(ch);          // Make SDA line high i.e. send I2C STOP control
    i2c_delay_5usec(2); // delay 10 usec.
    scl_1(ch);          // Set SCL to 1
    scl_out(ch);        // SCL is Push-Pull output
    return 0;           // all is good, both SCL and SDA are high
} // i2c_reset_bus()

/*-----------------------------------------------------------------------------
  Purpose  : This function initializes the I2C bit-banging routines
  Variables: ch: [0,1,2] I2C channel
  Returns  : --
  ---------------------------------------------------------------------------*/
uint8_t i2c_init_bb(enum I2C_CH ch)
{
      uint8_t x = i2c_reset_bus(ch); // 0 = I2C bus is reset

      sda_1(ch);         // Set SDA to 1
      scl_1(ch);         // Set SCL to 1
      scl_out(ch);       // SCL is Push-Pull output
      sda_out(ch);       // SDA line is Push-Pull output
      return x;
} // i2c_init_bb()

/*-----------------------------------------------------------------------------
  Purpose  : This function generates an I2C start condition.
             This is defined as SDA 1 -> 0 while SCL = 1
  Variables: ch: [0,1,2] I2C channel
             address: I2C address of device
  Returns  : ack bit from I2C device: ack (0) or nack (1)
  ---------------------------------------------------------------------------*/
uint8_t i2c_start_bb(enum I2C_CH ch, uint8_t address)
{   // Pre-condition : SDA = 1
    
    scl_1(ch);          // SCL = 1
    sda_0(ch);          // SDA = 0
    i2c_clk_delay();    // small delay
    scl_0(ch);          // SCL = 0
    return i2c_write_bb(ch,address); // Post-condition: SCL = 0, SDA = 0
} // i2c_start_bb;

/*-----------------------------------------------------------------------------
  Purpose  : This function generates an I2C repeated-start condition
  Variables: ch: [0,1,2] I2C channel
             address: I2C address of device
  Returns  : ack bit from I2C device: ack (0) or nack (1)
  ---------------------------------------------------------------------------*/
uint8_t i2c_rep_start_bb(enum I2C_CH ch, uint8_t address)
{   
    sda_1(ch);          // SDA = 1
    i2c_clk_delay();    // small delay
    return i2c_start_bb(ch,address);
} // i2c_start_bb;

/*-----------------------------------------------------------------------------
  Purpose  : This function generates an I2C stop condition
  Variables: ch: [0,1,2] I2C channel
  Returns  : -
  ---------------------------------------------------------------------------*/
void i2c_stop_bb(enum I2C_CH ch)
{   // Pre-condition : SDA = 0
    scl_1(ch);          // SCL = 1
    sda_1(ch);          // SDA = 1
    i2c_clk_delay();    // small delay
} // i2c_stop_bb;

/*-----------------------------------------------------------------------------
  Purpose  : This function writes a byte to the I2C bus
  Variables: ch: [0,1,2] I2C channel
  Returns  : ack bit from I2C device: ack (0) or nack (1)
  ---------------------------------------------------------------------------*/
uint8_t i2c_write_bb(enum I2C_CH ch, uint8_t data)
{
    uint8_t i   = 0x80;
    uint8_t ack = I2C_ACK;
    
    scl_0(ch); // clock low
    while (i > 0)
    {   // write bits to I2C bus
        if (data & i) sda_1(ch);
        else          sda_0(ch);
        scl_1(ch); // SCL = 1
        scl_0(ch); // SCL = 0
        i >>= 1;   // next bit
    } // while
    sda_in(ch);         // set as input
    i2c_clk_delay();    // small delay
    scl_1(ch);
    if (sda_read(ch)) ack = I2C_NACK; // ack (0), nack (1) 
    scl_0(ch);   // SCL = 0
    sda_out(ch); // set to output again
    sda_0(ch);   // SDA = 0
    return ack;
} // i2c_write_bb()

/*-----------------------------------------------------------------------------
  Purpose  : This function reads a byte from the I2C bus
  Variables: ack: send ack (0) or nack (1) to I2C device
  Returns  : result: byte read from I2C device
  ---------------------------------------------------------------------------*/
uint8_t i2c_read_bb(enum I2C_CH ch, uint8_t ack)
{
    uint8_t result = 0x00;
    uint8_t i      = 0x80;
    
    scl_0(ch);  // clock low
    sda_in(ch); // set as input
    while (i > 0)
    {   // read bits from I2C bus
        result <<= 1; // make room for new bit
        scl_1(ch);        // SCL = 1
        if (sda_read(ch)) result |=  0x01;
        scl_0(ch);       // SCL = 0
        i >>= 1;
    } // while
    sda_out(ch); // set to output again
    if (ack == I2C_ACK) 
         sda_0(ch); // output ACK
    else sda_1(ch); // output NACK
    scl_1(ch);      // SCL = 1
    scl_0(ch);      // SCL = 0
    sda_0(ch);      // SDA = 0
    return result;
} // i2c_read_bb()

int16_t lm92_read(enum I2C_CH ch, uint8_t *err)
/*------------------------------------------------------------------
  Purpose  : This function reads the LM92 13-bit Temp. Sensor and
             returns the temperature. The first LM92 device found in the
             address range of 0x90..0x96, is read.
             Reading Register 0 of the LM92 results in the following bits:
              15   14  13 12      3   2    1   0
             Sign MSB B10 B9 ... B0 Crit High Low
  Variables:
       dvc : ch: [I2C_CH0,I2C_CH1,I2C_CH2], the I2C channel where the LM92
                 is connected to.
  Returns  : The temperature from the LM92 in a signed Q8.7 format.
             Q8.7 is chosen here for accuracy reasons when filtering.
  ------------------------------------------------------------------*/
{
    uint8_t  buffer[2]; // array to store data from i2c_read()
    uint16_t temp_int;  // the temp. from the LM92 as an integer
    uint8_t  sign;      // sign of temperature
    int16_t  temp = 0;  // the temp. from the LM92 as an int
    uint8_t  adr;       // i2c address
    
    *err = 1;                      // assume no I2C device found 
    adr  = LM92_0_BASE | I2C_READ; // First possible LM92 address
    while (*err && (adr <= LM92_3_BASE+1))
    {
        *err = (i2c_start_bb(ch,adr) == I2C_NACK); // generate I2C start + output address to I2C bus
        if (*err) adr += 2;                       // no device found, try next possible I2C address 
    }; // while
    // adr contains address of LM92 found or *err is true (no LM92 present)     
    if (!*err)	
    {
        buffer[0] = i2c_read_bb(ch, I2C_ACK);  // Read 1st byte, request for more
        buffer[1] = i2c_read_bb(ch, I2C_NACK); // Read 2nd byte, generate I2C stop condition
        temp_int = buffer[0] & 0x00FF;         // store {Sign, MSB, bit 10..5} at bits temp_int bits 7..0
        temp_int <<= 8;                        // SHL 8, Sign now at bit 15
        temp_int &= 0xFF00;                    // Clear bits 7..0
        temp_int |= (buffer[1] & 0x00FF);      // Add bits D4..D0 to temp_int bits 7..3
        temp_int &= 0xFFF8;                    // Clear Crit High & Low bits
        sign = ((temp_int & LM92_SIGNb) == LM92_SIGNb);
        if (sign)
        {
            temp_int &= ~LM92_SIGNb;        // Clear sign bit
            temp_int  = LM92_FS - temp_int; // Convert two complement number
        } // if
        temp = temp_int; // without shifting! Returns Q8.7 format
        if (sign)
        {
            temp = -temp; // negate number
        } // if
    } // else
    i2c_stop_bb(ch);
    return temp;  // Return value now in °C << 7
} // lm92_read()

//--------------------------------------------------------------------------
// Perform a device reset on the DS2482, always connected to I2C_CH0.
//
// Device Reset
//   S AD,0 [A] DRST [A] Sr AD,1 [A] [SS] A\ P
//  [] indicates from slave
//  SS status byte to read to verify state
//
// Input: addr: the I2C address of the DS2482 to reset
// Returns: true if device was reset
//          false device not detected or failure to perform reset
//--------------------------------------------------------------------------
bool ds2482_reset(uint8_t addr)
{
    uint8_t err, ret = 0;
    
    // generate I2C start + output address to I2C bus
    err = (i2c_start_bb(I2C_CH0, addr | I2C_WRITE) == I2C_NACK);
    if (!err)
    {
        err  = (i2c_write_bb(I2C_CH0, CMD_DRST)  == I2C_NACK); // write register address
        i2c_rep_start_bb(I2C_CH0, addr | I2C_READ);
        ret = i2c_read_bb(I2C_CH0, I2C_NACK); // Read byte, generate I2C stop condition
        i2c_stop_bb(I2C_CH0);
    } // if
    // check for failure due to incorrect read back of status
    if (!err && ((ret & 0xF7) == 0x10))
         return true;
    else return false;	
} // ds2482_reset()

//--------------------------------------------------------------------------
// Write the configuration register in the DS2482. The configuration 
// options are provided in the lower nibble of the provided config byte. 
// The uppper nibble in bitwise inverted when written to the DS2482.
//  
// Write configuration (Case A)
//   S AD,0 [A] WCFG [A] CF [A] Sr AD,1 [A] [CF] A\ P
//  [] indicates from slave
//  CF configuration byte to write
//
// Input: addr: the I2C address of the DS2482 to reset
// Returns:  true: config written and response correct
//           false: response incorrect
//--------------------------------------------------------------------------
bool ds2482_write_config(uint8_t addr)
{
    uint8_t err, read_config = 0;
    
    // generate I2C start + output address to I2C bus
    err = (i2c_start_bb(I2C_CH0, addr | I2C_WRITE) == I2C_NACK);
    if (!err)
    {
        err  = (i2c_write_bb(I2C_CH0, CMD_WCFG)  == I2C_NACK); // write register address
        err |= (i2c_write_bb(I2C_CH0, DS2482_CONFIG)  == I2C_NACK); // write register address
        i2c_rep_start_bb(I2C_CH0, addr | I2C_READ);
        read_config = i2c_read_bb(I2C_CH0, I2C_NACK); // Read byte, generate I2C stop condition
        i2c_stop_bb(I2C_CH0);
    } // if
    // check for failure due to incorrect read back
    if (err || (read_config != DS2482_CONFIG))
    {
        ds2482_reset(addr); // handle error
        return false;
    } // if
    return true;
} // ds2482_write_config()

//--------------------------------------------------------------------------
// DS2428 Detect routine that performs a device reset followed by writing 
// the default configuration settings (active pullup enabled)
//
// Input: addr: the I2C address of the DS2482 to reset
// Returns: true if device was detected and written
//          false device not detected or failure to write configuration byte
//--------------------------------------------------------------------------
bool ds2482_detect(uint8_t addr)
{
   if (!ds2482_reset(addr)) // reset the DS2482
      return false;

   if (!ds2482_write_config(addr)) // write default configuration settings
        return false;
   else return true;
} // ds2482_detect()

//--------------------------------------------------------------------------
// Use the DS2482 help command '1-Wire triplet' to perform one bit of a 1-Wire
// search. This command does two read bits and one write bit. The write bit
// is either the default direction (all device have same bit) or in case of 
// a discrepancy, the 'search_direction' parameter is used. 
//
// Returns: The DS2482 status byte result from the triplet command
//--------------------------------------------------------------------------
uint8_t ds2482_search_triplet(uint8_t search_direction, uint8_t addr)
{
    uint8_t err, status;
    int poll_count = 0;
    
    // 1-Wire Triplet (Case B)
    //   S AD,0 [A] 1WT [A] SS [A] Sr AD,1 [A] [Status] A [Status] A\ P
    //                                         \--------/        
    //                           Repeat until 1WB bit has changed to 0
    //  [] indicates from slave
    //  SS indicates byte containing search direction bit value in msbit
    //
    // generate I2C start + output address to I2C bus
    err = (i2c_start_bb(I2C_CH0, addr | I2C_WRITE) == I2C_NACK);
    if (!err)
    {
        err  = (i2c_write_bb(I2C_CH0, CMD_1WT) == I2C_NACK); // write register address
        err |= (i2c_write_bb(I2C_CH0, search_direction ? 0x80 : 0x00) == I2C_NACK);
        i2c_rep_start_bb(I2C_CH0, addr | I2C_READ);
        // loop checking 1WB bit for completion of 1-Wire operation 
        // abort if poll limit reached
        status = i2c_read_bb(I2C_CH0, I2C_ACK); // Read byte
        do
        {
            if (status & STATUS_1WB) status = i2c_read_bb(I2C_CH0, I2C_ACK);
        }
        while ((status & STATUS_1WB) && (poll_count++ < DS2482_OW_POLL_LIMIT));
        status = i2c_read_bb(I2C_CH0, I2C_NACK);
        i2c_stop_bb(I2C_CH0);
        // check for failure due to poll limit reached
        if (poll_count >= DS2482_OW_POLL_LIMIT)
        {
            ds2482_reset(addr); // handle error
            return false;
        } // if
        return status;
    } // if
    else return false;
} // ds2482_search_triplet()