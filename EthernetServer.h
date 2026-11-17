#ifndef ETHERNET_SERVER_H
#define ETHERNET_SERVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Global variable defined in EthernetServer.c
extern uint16_t EthernetServer_port;

// Function prototypes matching EthernetServer.c
void EthernetServer_init(uint16_t port);
void EthernetServer_begin(void);
void EthernetServer_accept(void);
bool EthernetServer_available(void);
size_t EthernetServer_write(const uint8_t *buffer, size_t size);

#endif // ETHERNET_SERVER_H