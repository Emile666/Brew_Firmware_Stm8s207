/*==================================================================
  File Name: web_server.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This is the web-server for brew_stm8s207.c
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
#include <string.h>
#include <stdlib.h>
#include "webserver.h"
#include "delay.h"
#include "uart.h"
#include "max7219.h"

extern uint16_t Ethernet_server_port[]; // defined in Ethernet.c
extern uint8_t  enable_webserver;       // defined in brew_stm8s207.c
extern uint8_t  hlt_elec1_pwm;
extern uint8_t  hlt_elec2_pwm;
extern uint8_t  hlt_elec3_pwm;
extern uint8_t  bk_elec1_pwm;
extern uint8_t  bk_elec2_pwm;
extern uint8_t  bk_elec3_pwm;
extern uint8_t  hlt_gas_pwm;          // duty-cycle for gas-burner HLT
extern uint8_t  bk_gas_pwm;           // duty-cycle for gas-burner Boil-kettle
extern bool     delayed_start_enable; // true = delayed start is enabled
extern uint8_t  delayed_start_std;    // std number, start in INIT state
extern uint16_t delayed_start_timer1; // actual counter (in 2-sec ticks -> /30 = min)
extern uint16_t delayed_start_time;   // total time (in 2-sec ticks -> /30 = min)

extern int16_t    thlt_temp_87;       // THLT Temperature in °C * 128
extern int16_t    tmlt_temp_87;       // TMLT Temperature in °C * 128
extern int16_t    thlt_ow_87;         // THLT Temperature in °C * 128
extern int16_t    thlt_ow2_87;        // THLT Temperature from 2nd DS18B20 in °C * 128
extern int16_t    tmlt_ow_87;         // TMLT Temperature in °C * 128
extern int16_t    tmlt_ow2_87;        // TMLT Temperature from 2nd DS18B20 in °C * 128
extern int16_t    tcfc_temp_87;       // TCFC Temperature in °C * 128
extern int16_t    tboil_temp_87;      // TBOIL Temperature in °C * 128
extern uint8_t    max7219dig2;        // Frontpanel LEDs MAX7219 DIG2
extern const char *ebrew_revision;    // ebrew revision number

static inline uint8_t max_u8(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}

const char HTML_PAGE1[] =
    "<!DOCTYPE HTML><html><head>"
    "<title>Brew Controller</title>"
    "<style>"
    "body{font-family:Arial;text-align:center;margin-top:20px;}"
    ".box{border:1px solid #ccc;border-radius:8px;max-width:420px;margin:12px auto;padding:10px;}"
    ".on{color:green;font-weight:bold;}"
    ".off{color:red;font-weight:bold;}"
    ".wait{color:orange;font-weight:bold;}"
    "p{margin:6px 0;}"
    ".vgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px 16px;}"
    ".vrow{display:flex;justify-content:center;align-items:center;gap:6px;font-size:14px;}"
    ".vrow span{min-width:48px;text-align:center;}"
    "input[type=range]{width:68px;cursor:pointer;}"
    "input[type=number]{width:58px;text-align:center;font-size:14px;padding:2px;}"
    "</style>"
    "</head><body>"
    "<h1>Brew STM8S207 Webserver rev.";

const char HTML_PAGE2[] =
    "</h1>"
    "<div class=\"box\"><h3>Delayed Start</h3>"
     "<div class=\"vrow\"><b>Status:</b><span id=\"dse\" class=\"off\">--</span>"
    "<input type=\"range\" min=\"0\" max=\"2\" value=\"1\" onchange=\"sd(this)\"></div>"
    "<p>Time: <b><span id=\"dst1\">--</span>/<span id=\"dst2\">--</span> min</b> &nbsp;"
    "<input type=\"number\" id=\"dmin\" min=\"1\" max=\"1440\" value=\"60\"> min</p>"
    "</div>"

    "<div class=\"box\"><h3>Temperatures</h3>"
    "<p>THLT: <b><span id=\"thlt\">--</span> &deg;C, <span id=\"thlt1\">--</span> &deg;C, <span id=\"thlt2\">--</span> &deg;C</b></p>"
    "<p>TMLT: <b><span id=\"tmlt\">--</span> &deg;C, <span id=\"tmlt1\">--</span> &deg;C, <span id=\"tmlt2\">--</span> &deg;C</b></p>"
    "<p>TBOIL: <b><span id=\"tbk\">--</span> &deg;C</b></p>"
    "<p>TCFC: <b><span id=\"tcfc\">--</span> &deg;C</b></p>"
    "</div>"
      
    "<div class=\"box\"><h3>Pumps</h3>"
    "<div class=\"vgrid\">"
    "<div class=\"vrow\"><b>P1</b><span id=\"p1\" class=\"off\">--</span><input type=\"range\" min=\"0\" max=\"2\" value=\"1\" onchange=\"sp(1,this)\"></div>"
    "<div class=\"vrow\"><b>P2</b><span id=\"p2\" class=\"off\">--</span><input type=\"range\" min=\"0\" max=\"2\" value=\"1\" onchange=\"sp(2,this)\"></div>"
    "</div>"
    "</div>"

    "<div class=\"box\"><h3>HLT</h3>"
    "<p>Gas: <span id=\"hg\" class=\"off\">--</span></p>"
    "<p>E1: <span id=\"h1\" class=\"off\">--</span></p>"
    "<p>E2: <span id=\"h2\" class=\"off\">--</span></p>"
    "<p>E3: <span id=\"h3\" class=\"off\">--</span></p>"
    "<p>PWR: <b><span id=\"hp\">--</span> %</b></p>"
    "</div>"

    "<div class=\"box\"><h3>Boil Kettle</h3>"
    "<p>Gas: <span id=\"bg\" class=\"off\">--</span></p>"
    "<p>E1: <span id=\"b1\" class=\"off\">--</span></p>"
    "<p>E2: <span id=\"b2\" class=\"off\">--</span></p>"
    "<p>E3: <span id=\"b3\" class=\"off\">--</span></p>"
    "<p>PWR: <b><span id=\"bp\">--</span> %</b></p>"
    "</div>"

    "<div class=\"box\"><h3>Valves</h3>"
    "<div class=\"vgrid\" id=\"vlist\"></div>"
    "</div>"
      
    "<script>"
    "let vh='';"
    "for(let i=1;i<=8;i++){"
    "vh+='<div class=\"vrow\"><b>V'+i+'</b>'"
    "+'<span id=\"v'+i+'\" class=\"off\">--</span>'"
    "+'<input type=\"range\" min=\"0\" max=\"2\" value=\"1\" onchange=\"sw('+i+',this)\"></div>';}"
    "document.getElementById('vlist').innerHTML=vh;"
    "function sw(v,el){"
    "if(el.value==2)fetch('/vcmd?v='+v+'&s=1');"
    "else if(el.value==0)fetch('/vcmd?v='+v+'&s=0');"
    "setTimeout(()=>{el.value=1;},150);}"
    "function sp(p,el){"
    "if(el.value==2)fetch('/pcmd?p='+p+'&s=1');"
    "else if(el.value==0)fetch('/pcmd?p='+p+'&s=0');"
    "setTimeout(()=>{el.value=1;},150);}"
    "function sd(el){"
    "let m=parseInt(document.getElementById('dmin').value)||0;"
    "if(el.value==2)fetch('/dcmd?s=1&m='+m);"
    "else if(el.value==0)fetch('/dcmd?s=0&m=0');"
    "setTimeout(()=>{el.value=1;},150);}"      
    "function setSt(id,v){let e=document.getElementById(id);"
    "e.innerText=v?'ON':'OFF';e.className=v?'on':'off';}"
    "function setDs(v){let e=document.getElementById('dse');"
    "if(v==1){e.innerText='Delay';e.className='wait';}"
    "else if(v==2){e.innerText='Started';e.className='on';}"
    "else{e.innerText='OFF';e.className='off';}}"
    "function upd(){fetch('/status').then(r=>r.json()).then(d=>{"
    "setDs(d.dse);"
    "document.getElementById('thlt').innerText=d.thlt;"
    "document.getElementById('thlt1').innerText=d.thlt1;"
    "document.getElementById('thlt2').innerText=d.thlt2;"
    "document.getElementById('tmlt').innerText=d.tmlt;"
    "document.getElementById('tmlt1').innerText=d.tmlt1;"
    "document.getElementById('tmlt2').innerText=d.tmlt2;"
    "document.getElementById('tbk').innerText=d.tbk;"
    "document.getElementById('tcfc').innerText=d.tcfc;"
    "document.getElementById('dst1').innerText=d.dst1;"
    "document.getElementById('dst2').innerText=d.dst2;"
    "setSt('p1',d.p1);setSt('p2',d.p2);"
    "setSt('hg',d.hg);setSt('h1',d.h1);setSt('h2',d.h2);setSt('h3',d.h3);"
    "document.getElementById('hp').innerText=d.hp;"
    "setSt('bg',d.bg);setSt('b1',d.b1);setSt('b2',d.b2);setSt('b3',d.b3);"
    "document.getElementById('bp').innerText=d.bp;"
    "for(let i=0;i<8;i++){let e=document.getElementById('v'+(i+1));"
    "let st=(d.vv>>i)&1;e.innerText=st?'OPEN':'CLOSE';e.className=st?'on':'off';}"
    "});}"
    "setTimeout(upd,500);setInterval(upd,1000);"
    "</script></body></html>";

const char *html_header =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n\r\n";

const char *json_header =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n\r\n";

/*-----------------------------------------------------------------------
  Purpose  : This function sends a big string in parts of 250 bytes
  Variables: -
  Returns  : -
  -----------------------------------------------------------------------*/
void send_chunked(const char *str)
{
    size_t total_len = strlen(str);
    size_t offset = 0;

    while (offset < total_len)
    {
        size_t chunk = total_len - offset;
        if (chunk > 250)
        {
            chunk = 250;
        }
        EthernetClient_write((const uint8_t *)(str + offset), chunk);
        offset += chunk;
    } // while
} // send_chunked()

void temp87tos(char *name, int16_t val_87)
{
   uint16_t temp, frac_16, v87;
   char     s2[12];
   
   name[0] = '\0';
   if (val_87 < 0) 
   {
       v87 = (uint16_t)(-val_87); // make positive if negative
       strcat(name,"-");
   } // if
   else v87 =  (uint16_t)val_87;
   
   temp     = v87 >> 7;           // The integer part of the sensor value
   frac_16  = v87 & 0x007f;       // The fractional part of the sensor value
   frac_16 *= 25;                 // 100 / 128 = 25 / 32
   frac_16 += 16;                 // 0.5 for rounding
   frac_16 >>= 5;                 // SHR 5 = divide by 32
   if (frac_16 >= 100)            // Voorkom .100 bij afronden van .996
   {
       temp++;
       frac_16 = 0;
   } // if
   sprintf(s2,"%u.%02u" ,temp,frac_16);
   strcat(name,s2);               // store result back in *name
} // temp87tos()

/*-----------------------------------------------------------------------
  Purpose  : This function inits the the web-server on port 80
  Variables: -
  Returns  : -
  -----------------------------------------------------------------------*/
void init_webserver(void)
{
    char     s[30];   // Needed for uart_printf() and sprintf()
    uint8_t  bufr[8]; // Needed for w5500_read_common_register()
    uint8_t  x;
    uint16_t y;
    uint8_t  web_sock = MAX_SOCK_NUM;

    uart_printf("Webserver: ");
    EthernetServer_init(80);
    EthernetServer_begin();

    // Find which socket was claimed by EthernetServer_begin() for port 80
    for (x = 0; x < MAX_SOCK_NUM; x++)
    {
        if (Ethernet_server_port[x] == 80)
        {
            web_sock = x;
            break;
        } // if
    } // for 

    if (web_sock < MAX_SOCK_NUM)
    {
        sprintf(s, "ok, sock=%d\n", web_sock);        
        uart_printf(s);
        x = w5500_read_socket_register(web_sock, Sn_MR, bufr);
        sprintf(s, "MR=%d, ", x);                              
        uart_printf(s);
        y = w5500_read_socket_register(web_sock, Sn_PORT, bufr);
        sprintf(s, "PORT=%d, ", y);                            
        uart_printf(s);
        x = w5500_read_socket_register(web_sock, Sn_SR, bufr);
        sprintf(s, "SR=0x%02X, ", x);                          
        uart_printf(s);
        y = w5500_getTXFreeSize(web_sock);
        sprintf(s, "TXfree=%d, ", y);                          
        uart_printf(s);
        y = w5500_getRXReceivedSize(web_sock);
        sprintf(s,"RXrecv=%d\n",y);			  
        uart_printf(s);
    } // if
    else
    {
        uart_printf("error, no free socket\n");
    }  // else
} // init_webserver()

/*-----------------------------------------------------------------------
  Purpose  : This function runs a simple web-server at port 80
  Variables: -
  Returns  : -
  -----------------------------------------------------------------------*/
void run_webserver(void)
{
    char req_buf[64];
    int len = 0;
    bool got_1st_line = false;
    uint32_t start_time = millis();
    uint8_t  state;
    
    // Check enable flag FIRST so we don't touch W5500 registers when disabled
    if (!enable_webserver || !EthernetServer_available())
        return;
    
    while (EthernetClient_connected() && (millis() - start_time < 500))
    {   // Read the first 63 bytes into req_buf, and drain the rest of the headers
        int avail = EthernetClient_available();
        if (avail > 0)
        {
            if (!got_1st_line)
            {
                int to_read = (avail < (int)(sizeof(req_buf) - 1)) ? avail : (int)(sizeof(req_buf) - 1);
                len = EthernetClient_read((uint8_t *)req_buf, to_read);
                if (len > 0)
                {
                    req_buf[len] = '\0';
                    got_1st_line = true; // Lock req_buf so subsequent reads don't overwrite it!
                } // if
            } // if
            else
            {   // Drain remaining header bytes from the W5500 RX buffer
                uint8_t dummy[64];
                int to_read = (avail < (int)sizeof(dummy)) ? avail : (int)sizeof(dummy);
                int dlen = EthernetClient_read(dummy, to_read);
                if (dlen > 0 && dlen < (int)sizeof(dummy))
                {
                    dummy[dlen] = '\0';
                    if (strstr((char *)dummy, "\r\n\r\n") != NULL)
                        break;
                } // if
            } // else
            if (got_1st_line && strstr(req_buf, "\r\n\r\n") != NULL)
                break;
        } // if
    } // while
    if (got_1st_line)
    {   // 1. JSON request for pump status
        if (strncmp(req_buf, "GET /status", 11) == 0)
        {
            char json_buf[255];
            char sthlt[10] , sthlt1[10], sthlt2[10], stmlt[10];
            char stmlt1[10], stmlt2[10], stbk[10]  , stcfc[10];
            uint8_t  dse  = delayed_start_std;
            uint16_t dst1 = delayed_start_timer1 / 30;
            uint16_t dst2 = delayed_start_time / 30;
            uint8_t p1    = PUMP_230Vb  ? 1 : 0; // Read GPIO pin PF5
            uint8_t p2    = PUMP2_230Vb ? 1 : 0; // Read GPIO pin PF4
            uint8_t hg    = HLT_230Vb   ? 1 : 0; // Read GPIO pin PC3
            uint8_t h1    = hlt_elec1_pwm > 0 ? 1 : 0;
            uint8_t h2    = hlt_elec2_pwm > 0 ? 1 : 0;
            uint8_t h3    = hlt_elec3_pwm > 0 ? 1 : 0; 
            uint8_t hp    = max_u8(max_u8(hlt_elec1_pwm,hlt_elec2_pwm),
                                   max_u8(hlt_elec3_pwm,hlt_gas_pwm));
            uint8_t bg    = BOIL_230Vb   ? 1 : 0; // Read GPIO pin PC4
            uint8_t b1    = bk_elec1_pwm > 0 ? 1 : 0;
            uint8_t b2    = bk_elec2_pwm > 0 ? 1 : 0;
            uint8_t b3    = bk_elec3_pwm > 0 ? 1 : 0; 
            uint8_t bp    = max_u8(max_u8(bk_elec1_pwm,bk_elec2_pwm),
                                   max_u8(bk_elec3_pwm,bk_gas_pwm));
            uint8_t vv    = PB_ODR; // read status of all valves
            temp87tos(sthlt ,thlt_temp_87);
            temp87tos(stmlt ,tmlt_temp_87);
            temp87tos(sthlt1,thlt_ow_87);
            temp87tos(sthlt2,thlt_ow2_87);
            temp87tos(stmlt1,tmlt_ow_87);
            temp87tos(stmlt2,tmlt_ow2_87);
            temp87tos(stbk  ,tboil_temp_87);
            temp87tos(stcfc ,tcfc_temp_87);

            snprintf(json_buf, sizeof(json_buf),
             "{\"dse\":%u,\"dst1\":%u,\"dst2\":%u,"
              "\"thlt\":\"%s\",\"thlt1\":\"%s\",\"thlt2\":\"%s\","
              "\"tmlt\":\"%s\",\"tmlt1\":\"%s\",\"tmlt2\":\"%s\","
              "\"tbk\":\"%s\",\"tcfc\":\"%s\","
              "\"p1\":%u,\"p2\":%u,\"hg\":%u,\"h1\":%u,\"h2\":%u,\"h3\":%u,\"hp\":%u,"
              "\"bg\":%u,\"b1\":%u,\"b2\":%u,\"b3\":%u,\"bp\":%u,\"vv\":%u}",
              dse, dst1, dst2, 
              sthlt, sthlt1, sthlt2, stmlt, stmlt1, stmlt2, stbk, stcfc,
              p1, p2, hg, h1, h2, h3, hp, bg, b1, b2, b3, bp, vv);
            EthernetClient_write((const uint8_t *)json_header, strlen(json_header));
            EthernetClient_write((const uint8_t *)json_buf, strlen(json_buf));
        } // if
        else if (strncmp(req_buf, "GET /vcmd?", 10) == 0)
        {   // switch valve with "GET /vcmd?v=1&s=1" (or s=0)
            // Position 12 = valve number ('1'..'8'), position 16 = valve state ('0' or '1')
            uint8_t v_nr  = (uint8_t)(req_buf[12] - '0');
            
            state = (uint8_t)(req_buf[16] - '0');
            if (v_nr >= 1 && v_nr <= 8)
            {
                uint8_t mask = (uint8_t)(1U << (v_nr - 1));
                if (state == 1)
                {
                    PB_ODR |= mask;  // Zet bit (V1 = bit 0 .. V8 = bit 7)
                } // if
                else if (state == 0)
                {
                    PB_ODR &= ~mask; // Wis bit
                } // else if
                max7219_write(MAX7219_DIG1 | PB_ODR); // write valve bits to frontpanel LEDs
            } // if
            const char *ok_resp = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
            EthernetClient_write((const uint8_t *)ok_resp, strlen(ok_resp));
        } // else if
        else if (strncmp(req_buf, "GET /pcmd?", 10) == 0)
        {   // Switch Pump with "GET /pcmd?p=1&s=1" (or s=0)
            // Position 12 = pump number ('1' of '2'), position 16 = pump state ('0' or '1')
            uint8_t p_nr  = (uint8_t)(req_buf[12] - '0');
            
            state = (uint8_t)(req_buf[16] - '0');
            if (p_nr == 1)
            {
                if (state == 1)
                {
                    PUMP_230Vb = 1; 
                    max7219dig2 |=  MAX7219D2P1;
                } // if
                else
                {
                    PUMP_230Vb = 0;
                    max7219dig2 &= ~MAX7219D2P1;
                } // else    
            } // if
            else if (p_nr == 2)
            {
                if (state == 1)
                {
                    PUMP2_230Vb = 1; 
                    max7219dig2 |=  MAX7219D2P2;
                } // if
                else
                {
                    PUMP2_230Vb = 0;
                    max7219dig2 &= ~MAX7219D2P2;
                } // else    
            } // else if
            const char *ok_resp = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
            EthernetClient_write((const uint8_t *)ok_resp, strlen(ok_resp));
        } // else if
        else if (strncmp(req_buf, "GET /dcmd?", 10) == 0)
        {   // Delayed start with "GET /dcmd?s=1&m=120" (or s=0&m=0)
            // Position 12 = delayed start state ('0' of '1'), position 16 = timer value in minutes
            uint16_t mins = (uint16_t)atoi(&req_buf[16]); 

            state = (uint8_t)(req_buf[12] - '0');
            if (state == 1 && mins > 0)
            {
                if (mins > 1440) mins = 1440;     // Max 24 hours: 1440 * 30 = 43200 ticks
                delayed_start_time   = mins * 30; // Convert from minutes to 2 sec. intervals
                delayed_start_enable = 1;         // Enable delayed start function
            } // if
            else if (state == 0)
            {
                delayed_start_enable = 0;         // Disable delayed start
            } // if
            const char *ok_resp = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
            EthernetClient_write((const uint8_t *)ok_resp, strlen(ok_resp));
        } // else if
        else
        {   // 2. Main webpage request
            EthernetClient_write((const uint8_t *)html_header, strlen(html_header));
            send_chunked(HTML_PAGE1);
            EthernetClient_write((const uint8_t *)&ebrew_revision[11], 4);
            send_chunked(HTML_PAGE2);
        } // else
    } // if
    delay_msec(5);          // Give W5500 time to transmit the TCP packet
    EthernetClient_stop();  // Close socket cleanly
} // run_webserver()