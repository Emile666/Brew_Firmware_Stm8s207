#ifndef ETHERNETCLIENT_H
#define ETHERNETCLIENT_H
#include <stdint.h>
#include <stddef.h>

void    EthernetClient_init(uint8_t sock);
int     EthernetClient_connectHost(const char* host, uint16_t port);
int     EthernetClient_connectIP(uint8_t *ip, uint16_t port); 
size_t  EthernetClient_write(const uint8_t *buf, size_t size); 
int     EthernetClient_available(void); 
int     EthernetClient_read(uint8_t *buf, size_t size);
int     EthernetClient_peek(void); 
void    EthernetClient_flush(void); 
void    EthernetClient_stop(void);
uint8_t EthernetClient_connected(void);
uint8_t EthernetClient_status(void);

#endif
