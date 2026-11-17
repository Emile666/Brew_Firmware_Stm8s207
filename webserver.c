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
#include "webserver.h"
#include "delay.h"
#include "uart.h"

extern uint16_t Ethernet_server_port[]; // defined in Ethernet.c
extern uint8_t  enable_webserver;       // defined in brew_stm8s207.c

const char HTML_PAGE1[] =
    "<!DOCTYPE HTML><html><head>"
    "<title>Brew Controller</title>"
    "<style>body{font-family:Arial;text-align:center;margin-top:40px;}"
    ".on{color:green;font-weight:bold;}.off{color:red;font-weight:bold;}</style>";
const char HTML_PAGE2[] =
    "</head><body>"
    "<h1>Brew STM8S207 Webserver</h1>"
    "<p>Pump Status: <span id=\"p\" class=\"off\">--</span></p>"
    "<script>";
const char HTML_PAGE3[] =
    "function upd(){fetch('/status').then(r=>r.json()).then(d=>{"
    "let e=document.getElementById('p');"
    "e.innerText=d.pump?'ON':'OFF';"
    "e.className=d.pump?'on':'off';"
    "});}"
    "upd();setInterval(upd,1000);"
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
    for (uint8_t x = 0; x < MAX_SOCK_NUM; x++)
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

    // Check enable flag FIRST so we don't touch W5500 registers when disabled
    if (!enable_webserver || !EthernetServer_available())
        return;

    // Read the first 63 bytes into req_buf, and drain the rest of the headers
    while (EthernetClient_connected() && (millis() - start_time < 500))
    {
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
            {
                // Drain remaining header bytes from the W5500 RX buffer
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
    {
        // 1. JSON request for pump status
        if (strncmp(req_buf, "GET /status", 11) == 0)
        {
            char json_buf[32];
            uint8_t pump_state = PUMP_230Vb ? 1 : 0; // Read GPIO pin PE5

            snprintf(json_buf, sizeof(json_buf), "{\"pump\":%u}", pump_state);
            EthernetClient_write((const uint8_t *)json_header, strlen(json_header));
            EthernetClient_write((const uint8_t *)json_buf, strlen(json_buf));
        } // if
        // 2. Main webpage request
        else
        {
            EthernetClient_write((const uint8_t *)html_header, strlen(html_header));
            EthernetClient_write((const uint8_t *)HTML_PAGE1 , strlen(HTML_PAGE1));
            EthernetClient_write((const uint8_t *)HTML_PAGE2 , strlen(HTML_PAGE2));
            EthernetClient_write((const uint8_t *)HTML_PAGE3 , strlen(HTML_PAGE3));
        } // else
    } // if
    delay_msec(5);          // Give W5500 time to transmit the TCP packet
    EthernetClient_stop();  // Close socket cleanly
} // run_webserver()