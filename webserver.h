#ifndef _WEBSERVER_H
#define _WEBSERVER_H
/*==================================================================
  File Name: webserver.h
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This is the header-file for webserver.c
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
#include "EthernetServer.h"
#include "EthernetClient.h"
#include "Ethernet.h"
#include "w5500.h"
#include "socket.h"
    
void init_webserver(void);
void run_webserver(void);

#endif