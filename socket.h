#ifndef	_SOCKET_H_
#define	_SOCKET_H_
/*==================================================================
  File Name: socket.h
  Author   : ?, C-version by Emile
  ------------------------------------------------------------------
  Purpose  : Header file for socket.c
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
#include "w5500.h"

void     ipcpy(uint8_t *dest, uint8_t *src);
bool     ipequ(uint8_t *str1, uint8_t *str2);

uint8_t  socket(SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag); // Opens a socket(TCP or UDP or IP_RAW mode)
uint8_t  socketStatus(SOCKET s);
void     close(SOCKET s);       // Close socket
uint8_t  listen(SOCKET s);	// Establish TCP connection (Passive connection)
uint8_t  connect(SOCKET s, uint8_t *addr, uint16_t port); // Establish TCP connection (Active connection)
void     disconnect(SOCKET s);  // disconnect the connection
uint16_t send(SOCKET s, const uint8_t *buf, uint16_t len); // Send data (TCP)
int16_t  recv(SOCKET s, uint8_t *buf, int16_t len);	   // Receive data (TCP)
int16_t  recvAvailable(SOCKET s);
uint16_t peek(SOCKET s, uint8_t *buf);
uint16_t sendto(SOCKET s, const uint8_t *buf, uint16_t len, uint8_t* addr, uint16_t port); // Send data (UDP/IP RAW)
uint16_t recvfrom(SOCKET s, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t *port);    // Receive data (UDP/IP RAW)
void     flush(SOCKET s);       // Wait for transmission to complete
uint16_t igmpsend(SOCKET s, const uint8_t *buf, uint16_t len);
uint16_t bufferData(SOCKET s, uint16_t offset, const uint8_t *buf, uint16_t len);
int      startUDP(SOCKET s, uint8_t *addr, uint16_t port);
int      sendUDP(SOCKET s);

#endif
/* _SOCKET_H_ */
