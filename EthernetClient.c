#include "w5500.h"
#include "socket.h"
#include "Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dns.h"
#include "delay.h"

extern    uint16_t Ethernet_server_port[]; // defined in Ethernet.c
extern    uint8_t  dnsServerAddress[];	   // defined in Ethernet.c

uint16_t EthernetClient_srcport = 49152;        // Use IANA recommended ephemeral port range 49152-65535
uint8_t  EthernetClient_sock    = MAX_SOCK_NUM; // init. to no active socket

void EthernetClient_init(uint8_t sock)
{
	EthernetClient_sock = sock;
} // EthernetClient_init()

int EthernetClient_connectHost(const char* host, uint16_t port)
{
	// Look up the host first
	int ret = 0;
	uint8_t remote_addr[4];

	ipcpy(remote_addr, dnsServerAddress);
	dns_begin(remote_addr);
	ret = dns_getHostByName(host, remote_addr);
	if (ret == 1) 
	     return EthernetClient_connectIP(remote_addr, port);
	else return ret;
} // EthernetClient_connectHost()

int EthernetClient_connectIP(uint8_t *ip, uint16_t port) 
{
  if (EthernetClient_sock != MAX_SOCK_NUM)
    return 0;

  for (uint8_t i = 0; i < MAX_SOCK_NUM; i++)
  {
    uint8_t s = socketStatus(i);
    if (s == SnSR_CLOSED || s == SnSR_FIN_WAIT || s == SnSR_CLOSE_WAIT) 
    {
      EthernetClient_sock = i;
      break;
    } // if
  } // for i

  if (EthernetClient_sock == MAX_SOCK_NUM)
    return 0;

  EthernetClient_srcport++;
  if (EthernetClient_srcport == 0) EthernetClient_srcport = 49152;  // Use IANA recommended ephemeral port range 49152-65535
  socket(EthernetClient_sock, SnMR_TCP, EthernetClient_srcport, 0);

  if (!connect(EthernetClient_sock, ip, port))
  {
    EthernetClient_sock = MAX_SOCK_NUM;
    return 0;
  } // if

  while (EthernetClient_status() != SnSR_ESTABLISHED) 
  {
    delay_msec(1);
    if (EthernetClient_status() == SnSR_CLOSED) 
    {
      EthernetClient_sock = MAX_SOCK_NUM;
      return 0;
    } // if
  } // while
  return 1;
} // EthernetClient_connect()

size_t EthernetClient_write(const uint8_t *buf, size_t size) 
{
  if (EthernetClient_sock == MAX_SOCK_NUM) 
  {
    return 0;
  } // if
  if (!send(EthernetClient_sock, buf, size))
  {
    return 0;
  } // if
  return size;
} // EthernetClient_writeBuf()

int EthernetClient_available(void) 
{
  if (EthernetClient_sock != MAX_SOCK_NUM)
    return recvAvailable(EthernetClient_sock);
  return 0;
} // EthernetClient_available()

int EthernetClient_read(uint8_t *buf, size_t size)
{
  if (EthernetClient_sock == MAX_SOCK_NUM)
    return -1;
  return recv(EthernetClient_sock, buf, size);
} // EthernetClient_read()

int EthernetClient_peek(void) 
{
  uint8_t b;
  // Unlike recv, peek doesn't check to see if there's any data available, so we must
  if (!EthernetClient_available())
    return -1;
  peek(EthernetClient_sock, &b);
  return b;
} // EthernetClient_peek()

void EthernetClient_flush(void) 
{
  if (EthernetClient_sock != MAX_SOCK_NUM)
	flush(EthernetClient_sock);
} // EthernetClient_flush()

void EthernetClient_stop(void) 
{
  if (EthernetClient_sock == MAX_SOCK_NUM)
    return;

  // attempt to close the connection gracefully (send a FIN to other side)
  disconnect(EthernetClient_sock);
  uint32_t start = millis();

  // wait a second for the connection to close
  while (EthernetClient_status() != SnSR_CLOSED && millis() - start < 1000)
    delay_msec(1);

  // if it hasn't closed, close it forcefully
  if (EthernetClient_status() != SnSR_CLOSED)
    close(EthernetClient_sock);

  Ethernet_server_port[EthernetClient_sock] = 0;
  EthernetClient_sock = MAX_SOCK_NUM;
} // EthernetClient_stop()

uint8_t EthernetClient_connected(void) 
{
  if (EthernetClient_sock == MAX_SOCK_NUM) return 0;
  
  uint8_t s = EthernetClient_status();
  return !(s == SnSR_LISTEN || s == SnSR_CLOSED || s == SnSR_FIN_WAIT ||
          (s == SnSR_CLOSE_WAIT && !EthernetClient_available()));
} // EthernetClient_connected()

uint8_t EthernetClient_status(void) 
{
  if (EthernetClient_sock == MAX_SOCK_NUM) return SnSR_CLOSED;
  return socketStatus(EthernetClient_sock);
} // EthernetClient_status()

