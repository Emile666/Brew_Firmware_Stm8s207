#include "w5500.h"
#include "socket.h"
#include "Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"

extern    uint16_t Ethernet_server_port[]; // defined in Ethernet.c

uint16_t EthernetServer_port;

void EthernetServer_init(uint16_t port)
{
  EthernetServer_port = port;
} // EthernetServer_init()

void EthernetServer_begin(void)
{
  for (uint8_t sock = 0; sock < MAX_SOCK_NUM; sock++) 
  {
    EthernetClient_init(sock);
    if (EthernetClient_status() == SnSR_CLOSED) 
    {
      socket(sock, SnMR_TCP, EthernetServer_port, 0);
      listen(sock);
      Ethernet_server_port[sock] = EthernetServer_port;
      break;
    } // if
  } // for 
} // EthernetServer_begin()

void EthernetServer_accept(void)
{
  bool listening = false;

  for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
  {
    EthernetClient_init(sock);

    if (Ethernet_server_port[sock] == EthernetServer_port) 
    {
      if (EthernetClient_status() == SnSR_LISTEN)
      {
        listening = true;
      } // if 
      else if (EthernetClient_status() == SnSR_CLOSE_WAIT && !EthernetClient_available())
      {
        EthernetClient_stop();
      } // else if
    } // if 
  } // for sock

  if (!listening) 
  {
    EthernetServer_begin();
  } // if
} // EthernetServer_accept()

bool EthernetServer_available(void)
{
  EthernetServer_accept();

  for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
  {
    EthernetClient_init(sock);
    if (Ethernet_server_port[sock] == EthernetServer_port &&
        (EthernetClient_status() == SnSR_ESTABLISHED ||
         EthernetClient_status() == SnSR_CLOSE_WAIT)) 
    {
      if (EthernetClient_available()) 
      {
        // Global EthernetClient is now already pointing to 'sock'
        return true;
      } // if
    } // if
  } // for 
  // No client ready: reset global EthernetClient to invalid socket
  EthernetClient_init(MAX_SOCK_NUM);
  return false;
} // EthernetServer_available()

size_t EthernetServer_write(const uint8_t *buffer, size_t size) 
{
  size_t n = 0;
  
  EthernetServer_accept();

  for (uint8_t sock = 0; sock < MAX_SOCK_NUM; sock++)
  {
    EthernetClient_init(sock);

    if (Ethernet_server_port[sock] == EthernetServer_port &&
      EthernetClient_status() == SnSR_ESTABLISHED) 
    {
      n += EthernetClient_write(buffer, size);
    } // if
  } // for
  
  return n;
} // EthernetServer_write()
