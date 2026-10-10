/*==================================================================
  File Name: stm8_hw_init.c
  Author   : Soohwan Kim (suhwan@wiznet.co.kr), mod. 12 Aug 2013,
             C-version by Emile
  ------------------------------------------------------------------
  Purpose  : This files contains the Ethernet routines for the Wiz550io
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
#include "Ethernet.h"
#include "w5500.h"
#include "socket.h"
#include "uart.h"
#include "eep.h"

extern uint8_t dhcpLocalIp[4];
extern uint8_t dhcpGatewayIp[4];
extern uint8_t dhcpSubnetMask[4];
extern uint8_t dhcpDnsServerIp[4];

// XXX: don't make assumptions about the value of MAX_SOCK_NUM.
uint8_t  Ethernet_state[MAX_SOCK_NUM] = { 0, };
uint16_t Ethernet_server_port[MAX_SOCK_NUM] = { 0, };

uint8_t dnsServerAddress[4];

int Ethernet_begin(void)
{
  uint8_t mac_address[6] = {0,};
  uint8_t tmp[4] = {0,0,0,0};
  int ret = 0;
  char s[35];
  uint16_t ver = 0x00;
  
  // Initialize the basic info
  uart_printf("w5500_init():");
  w5500_init();
  w5500_write_common_register(SIPR,tmp);
  ver = w5500_read_common_register(VERSIONR,tmp);
  sprintf(s," ver. 0x%02x\nread MAC: ",ver);          
  uart_printf(s);
  w5500_read_common_register(SHAR ,mac_address);
  sprintf(s,"%02x:%02x:%02x:%02x:%02x:%02x ",mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]);
  uart_printf(s);
  if (!(mac_address[0] | mac_address[1] | mac_address[2] | mac_address[3] | mac_address[4] | mac_address[5]))
  {
       if (ver == 0x04)
       {  // W5500_Lite found, set MAC address manually
          mac_address[0] = 0x02; mac_address[1] = 0x08;
          mac_address[2] = 0xDC; mac_address[3] = 0x1D;
          mac_address[4] = 0x2B; 
          mac_address[5] = eep_read8(EEPARB_HW_ID); // read HW ID
          w5500_write_common_register(SHAR, mac_address); // write 6 bytes
          uart_printf("\nW5500_Lite found, MAC-address set to ");
          sprintf(s,"02:08:DC:1D:2B:%02x\n",mac_address[5]);
          uart_printf(s);
       } // if
       else
       {
          uart_printf(" : no W5500 found\n");
          return ret; // 0: error, no WIZ550IO found
       } // else
  } // if
  else uart_printf("\n");
  uart_printf("dhcp_begin():");
  
  // Now try to get our config info from a DHCP server
  ret = dhcp_begin(mac_address);
  if (ret == 1)
  {
    // We have successfully found a DHCP server and got our configuration info, 
    // so set things accordingly
    w5500_write_common_register(SIPR,dhcpLocalIp);
    w5500_write_common_register(GAR ,dhcpGatewayIp);
    w5500_write_common_register(SUBR,dhcpSubnetMask); 
    ipcpy(dnsServerAddress, dhcpDnsServerIp); // save result
    uart_printf("ok\n");
  } // if
  else uart_printf("no DHCP\n");
  return ret;
} // Ethernet_begin()

void Ethernet_begin_ip(uint8_t *local_ip)
{
  uint8_t tmp_ip[4];
  // Assume the DNS server and Gateway will be the machine on the same network 
  // as the local IP but with last octet being '1'
  ipcpy(tmp_ip, local_ip);
  tmp_ip[3] = 1;
  w5500_init(); // init. W5500 chipset
  w5500_write_common_register(SIPR,local_ip); // init. local IP address
  ipcpy(dnsServerAddress, tmp_ip);            // init. DNS-server address
  w5500_write_common_register(GAR ,tmp_ip);   // init. default Gateway address
  tmp_ip[0] = tmp_ip[1] = tmp_ip[2] = 255;
  tmp_ip[3] = 0;							  // subnet-mask = 255.255.255.0
  w5500_write_common_register(SUBR,tmp_ip);   // init. subnet-mask
} // Ethernet_begin_ip()

int Ethernet_maintain(void)
{
  int rc = DHCP_CHECK_NONE;
  rc = dhcp_checkLease();
  switch ( rc )
  {
      case DHCP_CHECK_NONE:
        //nothing done
        break;
      case DHCP_CHECK_RENEW_OK:
      case DHCP_CHECK_REBIND_OK:
            // we might have gotten a new IP.
            w5500_write_common_register(SIPR,dhcpLocalIp);
            w5500_write_common_register(GAR ,dhcpGatewayIp);
            w5500_write_common_register(SUBR,dhcpSubnetMask);
            ipcpy(dnsServerAddress, dhcpDnsServerIp); // save result
        break;
      default:
        // this is actually a error, it will retry though
        break;
    } // switch
  return rc;
} // Ethernet_maintain()
